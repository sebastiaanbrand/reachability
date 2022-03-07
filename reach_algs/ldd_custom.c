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


static int
match_ldds(MDD *one, MDD *two)
{
    MDD m1 = *one, m2 = *two;
    if (m1 == lddmc_false || m2 == lddmc_false) return 0;
    mddnode_t n1 = LDD_GETNODE(m1), n2 = LDD_GETNODE(m2);
    uint32_t v1 = mddnode_getvalue(n1), v2 = mddnode_getvalue(n2);
    while (v1 != v2) {
        if (v1 < v2) {
            m1 = mddnode_getright(n1);
            if (m1 == lddmc_false) return 0;
            n1 = LDD_GETNODE(m1);
            v1 = mddnode_getvalue(n1);
        } else if (v1 > v2) {
            m2 = mddnode_getright(n2);
            if (m2 == lddmc_false) return 0;
            n2 = LDD_GETNODE(m2);
            v2 = mddnode_getvalue(n2);
        }
    }
    *one = m1;
    *two = m2;
    return 1;
}


TASK_IMPL_3(MDD, lddmc_image, MDD, set, MDD, rel, MDD, meta)
{
    if (set == lddmc_false) return lddmc_false; // empty.R = empty
    if (rel == lddmc_false) return lddmc_false; // S.empty = empty
    if (set == lddmc_true && rel == lddmc_true) return lddmc_true; // all.all = all

    assert(meta != lddmc_true); // meta should not run out before set or rel

    /* We assume the rel is over the entire domain, i.e. r,w for every var */
    mddnode_t n_meta = LDD_GETNODE(meta);
    uint32_t m_val = mddnode_getvalue(n_meta);

    // dont want to deal with these action labels right now
    if (m_val == 5) {
        return CALL(lddmc_relprod, set, rel, meta);
    }
    assert(lddmc_getvalue(meta) == 1);

    /* Skip nodes if possible */
    if (!mddnode_getcopy(LDD_GETNODE(rel))) { // TODO: replace with iscopy for better legability
        if (!match_ldds(&set, &rel)) return lddmc_false;
    }

    /* Consult cache */
    MDD res = lddmc_false;
    if (cache_get3(CACHE_LDD_IMAGE, set, rel, 0, &res)) return res;

    MDD next_meta = get_next_meta(meta);

    /* Handle copy nodes */
    if (mddnode_getcopy(LDD_GETNODE(rel))) { // TODO: replace with iscopy for better legability
        // current read is a copy node (i.e. interpret as R_ii for all i)
        MDD rel_i = lddmc_getdown(rel);
        assert(mddnode_getcopy(LDD_GETNODE(rel_i)));

        MDD rel_ii = lddmc_getdown(rel_i);  lddmc_refs_pushptr(&rel_ii);
        MDD itr_s  = lddmc_false;           lddmc_refs_pushptr(&itr_s);
        MDD set_i  = lddmc_false;           lddmc_refs_pushptr(&set_i);

        // Iterate over all reads of 'set'
        for (itr_s = set; itr_s != lddmc_false; itr_s = lddmc_getright(itr_s)) {
            
            uint32_t i = lddmc_getvalue(itr_s);
            set_i = lddmc_follow(set, i); // NOTE: this is not efficient, since
            // lddmc_follow needs to iterate 'set' from the beginning each time
            // (here we should be able to replace this with getdown(itr_s))

            // Compute successors T_i = S_i.R_ii
            MDD succ_i = CALL(lddmc_image, set_i, rel_ii, next_meta);

            // Extend succ_i and add to successors
            MDD succ_i_ext = lddmc_makenode(i, succ_i, lddmc_false);
            res = lddmc_union(res, succ_i_ext);
        }

        lddmc_refs_popptr(3);
        rel = lddmc_getright(rel);
    }

    MDD itr_r  = lddmc_false;   lddmc_refs_pushptr(&itr_r);
    MDD itr_w  = lddmc_false;   lddmc_refs_pushptr(&itr_w);
    MDD rel_ij = lddmc_false;   lddmc_refs_pushptr(&rel_ij);
    MDD set_i  = lddmc_false;   lddmc_refs_pushptr(&set_i);
    lddmc_refs_pushptr(&res);

    // NOTE: Sylvan's lddmc_relprod, instead of this loop over children, 
    // simply delegates this "iterating to the right" by using recursion. 
    // I find this loop easier to think about, but maybe pure recursion is 
    // more efficient?
    // (Also by using pure recursion it is easier to parallelize this func)

    // Iterate over all reads (i) of 'rel'
    for (itr_r = rel; itr_r != lddmc_false; itr_r = lddmc_getright(itr_r)) {

        uint32_t i = lddmc_getvalue(itr_r);
        set_i = lddmc_follow(set, i); // NOTE: this is not efficient, since
        // lddmc_follow needs to iterate 'set' from the beginning each time
        // (need to iterate over set and reads of rel at the same time)

        // Iterate over all writes (j) corresponding to this read
        for (itr_w = lddmc_getdown(itr_r); itr_w != lddmc_false; itr_w = lddmc_getright(itr_w)) {

            uint32_t j = lddmc_getvalue(itr_w);
            rel_ij = lddmc_getdown(itr_w); // equiv to following i then j

            // Compute successors T_j = S_i.R_ij
            MDD succ_j = CALL(lddmc_image, set_i, rel_ij, next_meta);
        
            // Extend succ_j and add to successors
            MDD succ_j_ext = lddmc_makenode(j, succ_j, lddmc_false);
            res = lddmc_union(res, succ_j_ext);
        }
    }

    lddmc_refs_popptr(5);

    /* Put in cache */
    cache_put3(CACHE_LDD_IMAGE, set, rel, 0, res);

    return res;
}


TASK_IMPL_3(MDD, lddmc_extend_rel, MDD, rel, MDD, meta, uint32_t, nvars)
{
    if (nvars == 0) return rel;

    /* Consult cache */
    MDD res = lddmc_false;
    if (cache_get3(CACHE_LDD_EXTEND_REL, rel, meta, nvars, &res)) return res;

    uint32_t meta_val = lddmc_getvalue(meta);
    assert(meta_val != 5); // shouldn't run into a 5

    /* Get right and down */
    MDD right, down;
    uint32_t value;
    if (rel == lddmc_false || rel == lddmc_true) {
        right = lddmc_false;
        down  = rel;
    }
    else {
        mddnode_t node = LDD_GETNODE(rel);
        right = mddnode_getright(node);
        down  = mddnode_getdown(node);
        value = mddnode_getvalue(node);
    }

    /* Get next meta (down unless we run into a -1 or 5) */
    MDD next_meta;
    if (meta_val == (uint32_t)-1 || meta_val == 5)
        next_meta = meta;
    else
        next_meta = lddmc_getdown(meta);

    /* Call function on children */
    assert(right != lddmc_true);
    uint32_t next_nvars = (meta_val == 1) ? nvars : nvars-1;
    if (right != lddmc_false)
        right = CALL(lddmc_extend_rel, right, meta, nvars);
    down = CALL(lddmc_extend_rel, down, next_meta, next_nvars);

    // meta == 'read' : change nothing
    if (meta_val == 1) {
        assert(lddmc_getvalue(next_meta) == 2);
        res = lddmc_makenode(value, down, right);
    }
    // meta == 'write' : change nothing
    else if (meta_val == 2) {
        res = lddmc_makenode(value, down, right);
    }
    // meta == 'skipped variable' : change into two levels of *
    else if (meta_val == 0 || meta_val == (uint32_t)-1) {
        assert(lddmc_iscopy(rel) || rel == lddmc_true || rel == lddmc_false);
        assert(right == lddmc_false); // in rels, * nodes shouldn't have neighbors        
        down = lddmc_make_copynode(down, lddmc_false);
        res = lddmc_make_copynode(down, lddmc_false);
    }
    // TODO: handle meta_val == 3 and 4

     /* Put in cache */
    cache_put3(CACHE_LDD_EXTEND_REL, rel, meta, nvars, res);

    return res;
}
