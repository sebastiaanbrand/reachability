#include <stdbool.h>

#include "sylvan_int.h"

TASK_DECL_4(BDD, go_rec, BDD, BDD, BDDSET, bool);
#define bdd_reach(R, S, vars) RUN(go_rec, R, S, vars, 0)

TASK_DECL_3(BDD, go_rec_partial, BDD, BDD, BDDSET);
