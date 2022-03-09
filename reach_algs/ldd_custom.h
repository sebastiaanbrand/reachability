
#include "sylvan_int.h"

MDD lddmc_make_readwrite_meta(uint32_t nvars);

/**
 * Custom implementation of image computation for LDDs, which assumes the
 * relation covers the entire domain, but allows for copy-nodes (two levels) to
 * efficiently indicate identities.
 * 
 * NOTE: `meta` is only used for debugging asserts and should be removed as an 
 * argument later.
 */
TASK_DECL_3(MDD, lddmc_image, MDD, MDD, MDD);
#define lddmc_image(s, r, meta) RUN(lddmc_image, s, r, meta)

TASK_DECL_3(MDD, lddmc_extend_rel, MDD, MDD, uint32_t);
#define lddmc_extend_rel(rel, meta, nvars) RUN(lddmc_extend_rel,rel,meta,nvars)
