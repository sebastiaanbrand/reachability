#include <stdbool.h>

#include <sylvan_int.h>

#ifdef __cplusplus
namespace sylvan {
extern "C" {
#endif /* __cplusplus */

TASK_DECL_4(BDD, go_rec, BDD, BDD, BDDSET, bool);
#define bdd_reach(R, S, vars) RUN(go_rec, R, S, vars, 0)

TASK_DECL_3(BDD, go_rec_partial, BDD, BDD, BDDSET);

#ifdef __cplusplus
}
}
#endif /* __cplusplus */
