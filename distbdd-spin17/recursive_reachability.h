#include "sylvan.h"

#define reachable_bfs(s,r) RUN(reachable_bfs,s,r)
TASK_DECL_2(BDD, reachable_bfs, BDD, BDD);

#define reachable_rec(s,r,nvars) RUN(reachable_rec,s,r,nvars,0)
TASK_DECL_4(BDD, reachable_rec, BDD, BDD, BDDVAR, BDDVAR);
