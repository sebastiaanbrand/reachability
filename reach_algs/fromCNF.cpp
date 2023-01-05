#include <iostream>
#include <fstream>
#include <sstream>
#include <set>
#include <sys/time.h>
#include <sylvan_obj.hpp>
//#include <sylvan.h>
//#include <sylvan_int.h>

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

extern "C" {
#include "bdd_reach_algs.h"
}

BDD S, R, T; // initial states, relation, (optional) target states
BDDSET vars; // state vars (i.e. vars of S)


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
                int var = std::stoi(token);
                clause.insert(var);
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
 * Converts a CNF object (set of sets) into a BDD.
 */
Bdd cnf_to_bdd(Cnf f)
{
    Bdd res = Bdd::bddOne();
    for (Clause clause : f){
        Bdd c = Bdd::bddZero();
        for (int lit : clause) {
            if (lit != 0) {
                Bdd l = Bdd::bddVar(abs(lit));
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
    if (argc < 2) {
        Abort("Usage: ./fromCNF initial.cnf relation.cnf [target.cnf]\n");
    }

    // Get S, R, and T
    S = cnf_to_bdd(cnf_from_file(argv[0])).GetBDD();
    R = cnf_to_bdd(cnf_from_file(argv[1])).GetBDD();
    T = sylvan_false;
    if (argc >= 3) {
        T = cnf_to_bdd(cnf_from_file(argv[2])).GetBDD();
    }

    // Get state vars (assumes state vars = [0, 2, 4, ...,2*(n-1)])
    int nvars = nvars_from_file(argv[0]);
    uint32_t vars_arr[nvars];
    for (int i = 0; i < nvars; i++) vars_arr[i] = 2*i;
    vars = sylvan_set_fromarray(vars_arr, nvars);
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
    BDD reachable = bdd_reach(R, S, vars);

    
    double reach_states = sylvan_satcount(reachable, vars);
    INFO("Final states: %'0.0f states\n", reach_states);

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