#include <stdbool.h>

#include "sylvan_int.h"

MDD lddmc_make_readwrite_meta(uint32_t nvars, bool action_label);

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

/**
 * Extend relation to full domain:
 *  meta = 0: skipped vars are replaced with 2 copy nodes
 *  meta = 3: only-read 'a' is replaced with read 'a', write 'a'
 *  meta = 4: only-write 'a' is replaced with read '*', write 'a'
 * 
 * NOTE: `nvars` is the number of vars in the relation (i.e. 2x state vars)
 */
TASK_DECL_3(MDD, lddmc_extend_rel, MDD, MDD, uint32_t);
#define lddmc_extend_rel(rel, meta, nvars) RUN(lddmc_extend_rel,rel,meta,nvars)
