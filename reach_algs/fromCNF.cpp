#include <iostream>
#include <fstream>
#include <sstream>
#include <set>
#include <sys/time.h>
#include <sylvan_obj.hpp>

#include "bdd_reach_algs.h"


typedef std::set<int> Clause;
typedef std::set<Clause> Cnf;

/**
 * Obtain current wallclock time
 */
static double
wctime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec + 1E-6 * tv.tv_usec);
}

static double t_start;
#define INFO(s, ...) fprintf(stdout, "[% 8.2f] " s, wctime()-t_start, ##__VA_ARGS__)
#define Abort(...) { fprintf(stderr, __VA_ARGS__); fprintf(stderr, "Abort at line %d!\n", __LINE__); exit(-1); }


using namespace sylvan;


BDD S, R, T; // initial states, relation, (optional) target states
BDDSET vars; // state vars (i.e. vars of S) (even vars starting at 0)


void print_lit(int lit)
{
    if (lit > 0) {
        std::cout << "x" << lit;
    } else if (lit < 0) {
        std::cout << "~x" << std::abs(lit);
    } else {
        std::cerr << "literal 0 shouldn't occur in CNF formula" << std::endl;
    }
}

void print_clause(Clause clause)
{
    int lit_prev = INT32_MAX;
    std::cout << "(";
    for (int lit : clause) {
        if (lit_prev != INT32_MAX) {
            print_lit(lit_prev);
            std::cout << " v ";
        }
        lit_prev = lit;
    }
    // print last lit
    print_lit(lit_prev);
    std::cout << ")";
}

void print_cnf(Cnf cnf)
{
    Clause clause_prev;
    for (Clause clause : cnf) {
        if (clause_prev.size() != 0) {
            print_clause(clause_prev);
            std::cout << " ^ ";
        }
        clause_prev = clause;
    }
    print_clause(clause_prev);
    std::cout << std::endl;
}


/**
 * Loads a CNF fromula from a DIMACS (.cnf) file.
 */
Cnf cnf_from_file(std::string filepath)
{
    Cnf res;

    std::string line;
    std::string token;
    std::ifstream inFile(filepath);
    if (inFile.is_open()) {
        while (getline(inFile, line)) {
            Clause clause;
            std::istringstream ss(line);
            while (ss >> token) {
                if (token == "p" || token == "cnf") {
                    break;
                }
                int lit = std::stoi(token);
                if (lit != 0) {
                    clause.insert(lit);
                }
            }
            if (clause.size() > 0) {
                res.insert(clause);
            }
        }
        inFile.close();
    } else {
        std::cerr << "unable to open " << filepath << std::endl;
    }

    return res;
}


/**
 * Gets the number of variables from a DIMACS (.cnf) file.
 */
int nvars_from_file(std::string filepath)
{
    std::string line;
    std::string token;
    std::ifstream inFile(filepath);
    if (inFile.is_open()) {
        while (getline(inFile, line)) {
            std::istringstream ss(line);
            while (ss >> token) {
                if (token == "p") {
                    ss >> token;
                    if (token == "cnf") {
                        ss >> token; // should be number of vars
                        int nvars = std::stoi(token);
                        inFile.close();
                        return nvars;
                    }
                } else {
                    break;
                }
            }
        }
        inFile.close();
    }
    return 0;
}


/**
 * Converts a CNF object (set of sets) into a BDD (with vars starting at 0).
 */
Bdd cnf_to_bdd(Cnf f)
{
    Bdd res = Bdd::bddOne();
    for (Clause clause : f){
        Bdd c = Bdd::bddZero();
        for (int lit : clause) {
            if (lit != 0) {
                Bdd l = Bdd::bddVar(abs(lit)-1); // -1 to start vars at 0
                if (lit < 0) l = !l;
                c = c | l;
            }
        }
        res = res & c;
    }
    return res;
}


void parse_args(int argc, char** argv)
{
    if (argc < 4) {
        Abort("Usage: ./fromCNF initial.cnf relation.cnf [target.cnf]\n");
    }

    // Get S, R, and T
    Cnf cnf_s = cnf_from_file(argv[1]);
    Cnf cnf_targ = cnf_from_file(argv[3]);
    Cnf cnf_r = cnf_from_file(argv[2]);

    // convert CNFs to BDDs
    S = cnf_to_bdd(cnf_s).GetBDD();
    R = cnf_to_bdd(cnf_r).GetBDD();
    T = cnf_to_bdd(cnf_targ).GetBDD();

    printf("Source:\n");
    print_cnf(cnf_s);
    printf("Target:\n");
    print_cnf(cnf_targ);

    // Get state vars (assumes state vars = [0, 2, 4, ...,2*(n-1)])
    int nvars = nvars_from_file(argv[1]);
    if (nvars == 0) {
        Abort("Something went wrong parsing nvars from %s\n", argv[1]);
    }
    uint32_t vars_arr[nvars];
    for (int i = 0; i < nvars; i++) {
        vars_arr[i] = 2*i;
    }
    vars = sylvan_set_fromarray(vars_arr, nvars);

    INFO("Source set has %'0.0f state(s)\n", sylvan_satcount(S, vars));
    INFO("Target set has %'0.0f state(s)\n", sylvan_satcount(T, vars));
}


int main(int argc, char** argv)
{
    t_start = wctime();

    // Init Lace
    lace_start(1, 1000000);

    // Init Sylvan
    sylvan_set_limits(8LL<<30, 1, 6);
    //sylvan_set_limits(max, 1, 1);
    //sylvan_gc_disable();
    sylvan_init_package();
    sylvan_init_bdd();
    
    parse_args(argc, argv);
    

    // simple BFS
    // TODO: refactor BFS in bddmc to bdd_reach_algs, use that one
    // TODO: bdd_reach for this // BDD reachable = bdd_reach(R, S, vars);
    //       FIX: add a's to state and a-primes to rel
    BDD reachable = S;
    BDD prev = sylvan_false;
    while (prev != reachable) {
        prev = reachable;
        BDD successors = sylvan_relnext(reachable,  R, vars);
        reachable = sylvan_or(reachable, successors);
        INFO("Reachable states: %'0.0f state(s)\n", sylvan_satcount(reachable, vars));
    }

    FILE *fp = fopen("reachable.dot", "w");
    sylvan_fprintdot(fp, reachable);
    fclose(fp);

    
    double reach_states = sylvan_satcount(reachable, vars);
    INFO("Reachable states: %'0.0f state(s)\n", reach_states);

    if (T != sylvan_false) {
        BDD intersection = sylvan_and(reachable, T);
        if (intersection == sylvan_false) {
            INFO("Target state is not reachable\n");
        } else {
            INFO("Target state is reachable\n");
        }
    }


    sylvan_quit();
    lace_stop();

    return 0;
}