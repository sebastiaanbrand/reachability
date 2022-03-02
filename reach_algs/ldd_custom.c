#include "ldd_custom.h"
#include "cache_op_ids.h"

static MDD
get_next_meta(MDD meta)
{
    /* Step over a read (1) and a write (2) in the meta MDD */
    mddnode_t n;

    n = LDD_GETNODE(meta);
    assert(mddnode_getvalue(n) == 1);
    meta = mddnode_getdown(n);

    n = LDD_GETNODE(meta);
    assert(mddnode_getvalue(n) == 2);
    meta = mddnode_getdown(n);

    return meta;
}

TASK_IMPL_3(MDD, lddmc_image, MDD, set, MDD, rel, MDD, meta)
{
    if (set == lddmc_false) return lddmc_false; // empty.R = empty
    if (rel == lddmc_false) return lddmc_false; // S.empty = empty
    if (set == lddmc_true && rel == lddmc_true) return lddmc_true; // all.all = all

    /* We assume the rel is over the entire domain, i.e. r,w for every var */
    mddnode_t n_meta = LDD_GETNODE(meta);
    uint32_t m_val = mddnode_getvalue(n_meta);

    // dont want to deal with these action labels right now
    if (m_val == 5) {
        return CALL(lddmc_relprod, set, rel, meta);
    }
    assert(lddmc_getvalue(meta) == 1);

    /* Consult cache */
    // TODO: more normalization / preprocessing before caching?
    // e.g. sync set and rel on their first common node?
    // (look at Tom's lddmc_relprod)
    int cachenow = 1;
    if (cachenow) {
        MDD res;
        if (cache_get3(CACHE_LDD_IMAGE, set, rel, 0, &res)) return res;
    }

    MDD next_meta = get_next_meta(meta);
    MDD itr_r  = lddmc_false;   lddmc_refs_pushptr(&itr_r);
    MDD itr_w  = lddmc_false;   lddmc_refs_pushptr(&itr_w);
    MDD rel_ij = lddmc_false;   lddmc_refs_pushptr(&rel_ij);
    MDD set_i  = lddmc_false;   lddmc_refs_pushptr(&set_i);
    MDD succ   = lddmc_false;   lddmc_refs_pushptr(&succ);
    
    // NOTE: Sylvan's lddmc_relprod, instead of this loop over children, simply
    // delegates this "iterating to the right" by using recursion. I find this
    // loop easier to think about, but maybe pure recursion is more efficient?
    // (This is e.g. inefficient if R is really dense and S is quite sparse).
    // (Also by using pure recursion it is easier to parallelize this func)

    // Iterate over all reads (i) of 'rel'
    for (itr_r = rel; itr_r != lddmc_false; itr_r = lddmc_getright(itr_r)) {

        uint32_t i = lddmc_getvalue(itr_r);
        set_i = lddmc_follow(set, i);

        // Iterate over all writes (j) corresponding to this read
        for (itr_w = lddmc_getdown(itr_r); itr_w != lddmc_false; itr_w = lddmc_getright(itr_w)) {

            uint32_t j = lddmc_getvalue(itr_w);
            rel_ij = lddmc_getdown(itr_w); // equiv to following i then j

            // Compute successors T_j = S_i.R_ij
            MDD succ_j = CALL(lddmc_image, set_i, rel_ij, next_meta);
        
            // Extend succ_j and add to successors
            MDD succ_j_ext = lddmc_makenode(j, succ_j, lddmc_false);
            succ = lddmc_union(succ, succ_j_ext);
        }
    }

    lddmc_refs_popptr(5);

    /* TODO: Put in cache */
    if (cachenow)
        cache_put3(CACHE_LDD_IMAGE, set, rel, 0, succ);

    return succ;
}