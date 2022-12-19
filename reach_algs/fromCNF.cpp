#include <iostream>
#include <fstream>
#include <set>
#include <sylvan_obj.hpp>
#include <sylvan.h>
//#include <sylvan_int.h>

using namespace sylvan;

extern "C" {
#include "bdd_reach_algs.h"
}


int main(int argc, char** argv)
{
    // Init Lace
    lace_start(1, 1000000);

    // Init Sylvan
    sylvan_set_limits(8LL<<30, 1, 6);
    //sylvan_set_limits(max, 1, 1);
    //sylvan_gc_disable();
    sylvan_init_package();
    sylvan_init_bdd();

    BDD R = sylvan_false;
    BDD S = sylvan_true;
    BDDSET vars = sylvan_false;
    BDD reachable = bdd_reach(R, S, vars);


    sylvan_quit();
    lace_stop();

    return 0;
}