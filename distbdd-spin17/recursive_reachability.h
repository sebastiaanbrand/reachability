#include "sylvan.h"

typedef struct relation {
    BDD bdd;
    BDDSET variables; // all variables (primed and unprimed) in the relation
} *rel_t;

#define reachable_sat(s,rels,relcount) RUN(reachable_sat,s,rels,relcount, 0)
TASK_DECL_4(BDD, reachable_sat, BDD, rel_t*, int, int);

#define reachable_bfs(s,r) RUN(reachable_bfs,s,r)
TASK_DECL_2(BDD, reachable_bfs, BDD, BDD);

#define reachable_rec(s,r) RUN(reachable_rec,s,r,0)
TASK_DECL_3(BDD, reachable_rec, BDD, BDD, BDDVAR);
