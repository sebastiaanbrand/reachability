#include <stdbool.h>

#include "sylvan_int.h"

MDD lddmc_make_normalnode(uint32_t value, MDD ifeq, MDD ifneq);
MDD lddmc_make_homomorphism_node(uint32_t value, MDD ifeq, MDD ifneq);
// NOTE: also removes homomorphism flag from value
bool lddmc_is_homomorphism(uint32_t *value);

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
 * Version of image which also use right recursion.
 */
TASK_DECL_3(MDD, lddmc_image2, MDD, MDD, MDD);
#define lddmc_image2(s, r, meta) RUN(lddmc_image2, s, r, meta)

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

/**
 * In principle the `lddmc_union` already in Sylvan should work for our 
 * definition of rels, however in some cases it gives incorrect results so we
 * have this custom function for taking the union of relations.
 */
TASK_DECL_2(MDD, lddmc_rel_union, MDD, MDD);
#define lddmc_rel_union(a, b) RUN(lddmc_rel_union, a, b)
