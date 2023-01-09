#include <iostream>
#include <fstream>
#include <sstream>
#include <set>
#include <argp.h>
#include <string.h>
#include <sys/time.h>
#include <sylvan_obj.hpp>

#include "bdd_reach_algs.h"


typedef std::set<int> Clause;
typedef std::set<Clause> Cnf;

static int strategy = 0;
static int workers = 1;

typedef enum strats {
    strat_bfs,
    strat_reach
} strategy_t;

using namespace sylvan;

std::string s_file, r_file, t_file;
BDD S, R, T; // initial states, relation, (optional) target states
BDDSET vars; // state vars (i.e. vars of S) (even vars starting at 0)

/* argp configuration */
static struct argp_option options[] =
{
    {"workers", 'w', "<workers>", 0, "Number of workers (default=1)", 0},
    {"strategy", 's', "<bfs|reach>", 0, "Strategy for reachability (default=bfs)", 0},
    {0, 0, 0, 0, 0, 0}
};

static error_t
parse_opt(int key, char *arg, struct argp_state *state)
{
    switch (key) {
    case 'w':
        workers = atoi(arg);
        break;
    case 's':
        if (strcmp(arg, "bfs")==0) strategy = strat_bfs;
        else if (strcmp(arg, "reach")==0) strategy = strat_reach;
        else argp_usage(state);
        break;
    case ARGP_KEY_ARG:
        if (state->arg_num == 0) s_file = arg;
        else if (state->arg_num == 1) r_file = arg;
        else if (state->arg_num == 2) t_file = arg;
        else if (state->arg_num > 2) argp_usage(state);
        break;
    case ARGP_KEY_END:
        if (state->arg_num < 3) argp_usage(state);
        break;
    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}
static struct argp argp = { options, parse_opt, "<s.cnf> <r.cnf> <t.cnf>", 0, 0, 0, 0 };


typedef struct stats {
    double reach_time;
} stats_t;
stats_t stats = {0};

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


void load_cnfs_to_bdds()
{
    // Get S, R, and T
    INFO("Loading CNFs from files\n");
    Cnf cnf_s = cnf_from_file(s_file);
    Cnf cnf_targ = cnf_from_file(t_file);
    Cnf cnf_r = cnf_from_file(r_file);

    // Convert CNFs to BDDs
    INFO("Converting CNFs to BDDs\n");
    S = cnf_to_bdd(cnf_s).GetBDD();
    R = cnf_to_bdd(cnf_r).GetBDD();
    T = cnf_to_bdd(cnf_targ).GetBDD();

    // Get state vars (assumes state vars = [0, 2, 4, ...,2*(n-1)])
    int nvars = nvars_from_file(s_file);
    if (nvars == 0) {
        Abort("Something went wrong parsing nvars from %s\n", s_file.c_str());
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
    /* Parse CL args */
    argp_parse(&argp, argc, argv, 0, 0, 0);
    t_start = wctime();

    // Init Lace
    lace_start(workers, 1000000);
    INFO("Lace workers: %d\n", lace_workers());

    // Init Sylvan
    sylvan_set_limits(8LL<<30, 1, 6);
    sylvan_init_package();
    sylvan_init_bdd();

    // parse args + load cnf files
    load_cnfs_to_bdds();

    INFO("Computing reachability...\n");
    BDD reachable = sylvan_false;
    if (strategy == strat_reach) {
        double t1 = wctime();
        // TODO: not sure if this works in all cases, since R has vars (a) which
        // aren't in the state space (but they are at the bottom of the DD)
        reachable = bdd_reach(S, R, vars);
        double t2 = wctime();
        stats.reach_time = t2-t1;
        INFO("REACH Time: %f\n", stats.reach_time);
    } else if (strategy == strat_bfs) {
        double t1 = wctime();
        reachable = simple_bfs(S, R, vars);
        double t2 = wctime();
        stats.reach_time = t2-t1;
        INFO("BFS Time: %f\n", stats.reach_time);
    } else {
        Abort("Invalid strategy set?!\n");
    }
    
    double nstates = sylvan_satcount(reachable, vars);
    INFO("Reachable states: %'0.0f state(s)\n", nstates);

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