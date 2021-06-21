#include "sylvan.h"

#define reachable_bfs(s,r,vars) RUN(reachable_bfs,s,r,vars)
TASK_DECL_3(BDD, reachable_bfs, BDD, BDD, BDDSET);

#define reachable_rec(s,r,vars,nvars) RUN(reachable_rec,s,r,vars,nvars,0)
TASK_DECL_5(BDD, reachable_rec, BDD, BDD, BDDSET, BDDVAR, BDDVAR);
