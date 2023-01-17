#include <stdbool.h>

#include <sylvan.h>

#ifdef __cplusplus
namespace sylvan {
extern "C" {
#endif /* __cplusplus */

TASK_DECL_4(BDD, go_rec, BDD, BDD, BDDSET, bool);
#define bdd_reach(S, R, vars) RUN(go_rec, S, R, vars, 0)

TASK_DECL_3(BDD, go_rec_partial, BDD, BDD, BDDSET);

TASK_DECL_5(BDD, go_bfs_plain, BDD, BDD, BDD, BDDSET, int*);
#define simple_bfs(S, R, T, vars, steps) RUN(go_bfs_plain, S, R, T, vars, steps)

#ifdef __cplusplus
}
}
#endif /* __cplusplus */
