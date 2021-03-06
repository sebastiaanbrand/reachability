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


MDD lddmc_make_readwrite_meta(uint32_t nvars, bool action_label)
{
    MDD meta = lddmc_true;
    if (action_label)
        meta = lddmc_makenode(5, meta, lddmc_false);
    for (uint32_t i = 0; i < nvars; i++) {
        meta = lddmc_makenode(2, meta, lddmc_false);
        meta = lddmc_makenode(1, meta, lddmc_false);
    }
    return meta;
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
    if (!m_val == 1) {
        printf("m_val = %d\n", m_val);
    }

    /* Skip nodes if possible */
    if (!lddmc_iscopy(rel)) {
        if (!match_ldds(&set, &rel)) return lddmc_false;
    }

    /* Consult cache */
    MDD res = lddmc_false;
    if (cache_get3(CACHE_LDD_IMAGE, set, rel, 0, &res)) return res;

    MDD next_meta = get_next_meta(meta);

    MDD _rel   = rel;           lddmc_refs_pushptr(&_rel);
    MDD itr_r  = lddmc_false;   lddmc_refs_pushptr(&itr_r);
    MDD itr_w  = lddmc_false;   lddmc_refs_pushptr(&itr_w);
    MDD rel_ij = lddmc_false;   lddmc_refs_pushptr(&rel_ij);
    MDD set_i  = lddmc_false;   lddmc_refs_pushptr(&set_i);
    MDD rel_i  = lddmc_false;   lddmc_refs_pushptr(&rel_i); // might be possible to only use itr_w
    MDD tmp    = lddmc_false;   lddmc_refs_pushptr(&tmp);
    lddmc_refs_pushptr(&res);

    /* Handle copy nodes */
    if (lddmc_iscopy(rel)) {
        // current read is a copy node (i.e. interpret as R_ii for all i)
        rel_i = lddmc_getdown(rel);

        // if there is a copy node among the writes, it comes first
        if (lddmc_iscopy(rel_i)) {
            // rel = (* -> *), i.e. read anything, write what was read

            rel_ij = lddmc_getdown(rel_i); // j = i

            // Iterate over all reads of 'set'
            for (itr_r = set; itr_r != lddmc_false; itr_r = lddmc_getright(itr_r)) {
                
                uint32_t i = lddmc_getvalue(itr_r);
                set_i = lddmc_getdown(itr_r);

                // Compute successors T_i = S_i.R_ii
                tmp = CALL(lddmc_image, set_i, rel_ij, next_meta);

                // Extend succ_i and add to successors
                tmp = lddmc_makenode(i, tmp, lddmc_false);
                res = lddmc_union(res, tmp);
            }

            // After (* -> *), there might still be (* -> j)'s
            rel_i = lddmc_getright(rel_i);
        }
        // rel = (* -> j), i.e. read anything, write j

        // Iterate over all reads of 'set'
        for (itr_r = set; itr_r != lddmc_false; itr_r = lddmc_getright(itr_r)) {
            
            set_i = lddmc_getdown(itr_r);

            // Iterate over all writes (j) of rel
            for (itr_w = rel_i; itr_w != lddmc_false; itr_w = lddmc_getright(itr_w)) {

                uint32_t j = lddmc_getvalue(itr_w);
                rel_ij = lddmc_getdown(itr_w); // equiv to following * then j

                // Compute successors T_j = S_*.R_*j
                tmp = CALL(lddmc_image, set_i, rel_ij, next_meta);

                // Extend succ_j and add to successors
                tmp = lddmc_makenode(j, tmp, lddmc_false);
                res = lddmc_union(res, tmp);
            }
        }
        _rel = lddmc_getright(_rel);
    }


    // NOTE: Sylvan's lddmc_relprod, instead of this loop over children, 
    // simply delegates this "iterating to the right" by using recursion. 
    // I find this loop easier to think about, but maybe pure recursion is 
    // more efficient?
    // (Also by using pure recursion it is easier to parallelize this func)

    // Iterate over all reads (i) of 'rel'
    for (itr_r = _rel; itr_r != lddmc_false; itr_r = lddmc_getright(itr_r)) {

        uint32_t i = lddmc_getvalue(itr_r);
        set_i = lddmc_follow(set, i); // NOTE: this is not efficient, since
        // lddmc_follow needs to iterate 'set' from the beginning each time
        // (need to iterate over set and reads of rel at the same time)

        // Iterate over all writes (j) corresponding to this read
        for (itr_w = lddmc_getdown(itr_r); itr_w != lddmc_false; itr_w = lddmc_getright(itr_w)) {

            uint32_t j = lddmc_getvalue(itr_w);
            rel_ij = lddmc_getdown(itr_w); // equiv to following i then j

            // Compute successors T_j = S_i.R_ij
            tmp = CALL(lddmc_image, set_i, rel_ij, next_meta);
        
            // Extend succ_j and add to successors
            tmp = lddmc_makenode(j, tmp, lddmc_false);
            res = lddmc_union(res, tmp);
        }
    }

    lddmc_refs_popptr(8);

    /* Put in cache */
    cache_put3(CACHE_LDD_IMAGE, set, rel, 0, res);

    return res;
}

TASK_IMPL_3(MDD, lddmc_image2, MDD, set, MDD, rel, MDD, meta)
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
    if (!m_val == 1) {
        printf("m_val = %d\n", m_val);
    }

    MDD next_meta = mddnode_getdown(n_meta);

    /* Skip nodes if possible */
    if (m_val == 1 && !lddmc_iscopy(rel)) {
        if (!match_ldds(&set, &rel)) return lddmc_false;
    }

    /* Consult cache */
    MDD res = lddmc_false;
    if (cache_get3(CACHE_LDD_IMAGE, set, rel, meta, &res)) return res;
    lddmc_refs_pushptr(&res);


    //mddnode_t n_set = LDD_GETNODE(set);
    //mddnode_t n_rel = LDD_GETNODE(rel);
    MDD _set = set, _rel = rel, tmp = lddmc_false;
    lddmc_refs_pushptr(&_set);
    lddmc_refs_pushptr(&_rel);
    lddmc_refs_pushptr(&tmp);

    /* Recursive operations */
    if (m_val == 1) { // read

        tmp = CALL(lddmc_image2, _set, lddmc_getright(_rel), meta);
        res = lddmc_union(res, tmp);

        // for this read, either it is copy ('for all') or it is normal match
        if (lddmc_iscopy(_rel)) {

            // call for every read value of set
            for (; _set != lddmc_false; _set = lddmc_getright(_set)) {
                // stay at same level of set (for write)
                tmp = CALL(lddmc_image2, _set, lddmc_getdown(_rel), next_meta);
                res = CALL(lddmc_union, res, tmp);
            }
        }
        else {
            // if not copy, then set&rel are already matched
            // stay at the same level of set (for write)
            tmp = CALL(lddmc_image2, _set, lddmc_getdown(_rel), next_meta);
            res = CALL(lddmc_union, res, tmp);
        }
    }
    else if (m_val == 2) { // write

        // call for every value to write (rel)
        for (;;) {
            uint32_t value;
            if (lddmc_iscopy(_rel)) value = lddmc_getvalue(_set);
            else value = lddmc_getvalue(_rel);

            tmp = CALL(lddmc_image2, lddmc_getdown(_set), lddmc_getdown(_rel), next_meta);
            tmp = lddmc_makenode(value, tmp, lddmc_false);
            res = CALL(lddmc_union, res, tmp);

            _rel = lddmc_getright(_rel);
            if (_rel == lddmc_false) break;
            //n_rel = LDD_GETNODE(rel);
        }
    }
    else {
        printf("unexpected m_val = %d\n", m_val);
        exit(1);
    }

    /* Put in cache */
    cache_put3(CACHE_LDD_IMAGE, set, rel, meta, res);

    lddmc_refs_popptr(4);

    return res;
}


TASK_3(MDD, only_read_helper, MDD, right, MDD, next_meta, MDD, next_nvars)
{
    // not sure how to solve this more elegantly w/o this helper function
    if (right == lddmc_false) return lddmc_false;
    assert(right != lddmc_true);

    mddnode_t node = LDD_GETNODE(right);
    MDD next_right = mddnode_getright(node);
    MDD down       = mddnode_getdown(node);
    MDD value      = mddnode_getvalue(node);
    assert(!mddnode_getcopy(node) && "there should be no copy-nodes to the right");
    
    next_right = CALL(only_read_helper, next_right, next_meta, next_nvars);
    down       = CALL(lddmc_extend_rel, down, next_meta, next_nvars);
    return lddmc_makenode(value, down, next_right);
}


TASK_IMPL_3(MDD, lddmc_extend_rel, MDD, rel, MDD, meta, uint32_t, nvars)
{
    if (nvars == 0) {
        if (rel == lddmc_true || rel == lddmc_false) return rel;
        if (lddmc_getvalue(meta) == 5) return rel;
        // I don't understand why, but removing the action label with the code
        // below gives issues
        //if (lddmc_getvalue(meta) == 5) {
        //    assert(lddmc_getdown(rel) == lddmc_true || lddmc_getdown(rel) == lddmc_false);
        //    return lddmc_getdown(rel);
        //}
    }

    /* Consult cache */
    MDD res = lddmc_false;
    if (cache_get3(CACHE_LDD_EXTEND_REL, rel, meta, nvars, &res)) return res;

    uint32_t meta_val = lddmc_getvalue(meta);

    // In case of 5 or -1, we'll consider all the remaining vars as skipped and
    // insert two levels of * for them.
    if (meta_val == 5 || meta_val == (uint32_t)-1) {
        res = lddmc_true;
        assert (nvars % 2 == 0);
        for (uint32_t i = 0; i < (nvars/2); i++) {
            res = lddmc_make_copynode(res, lddmc_false);
            res = lddmc_make_copynode(res, lddmc_false);
        }
        /* Put in cache */
        cache_put3(CACHE_LDD_EXTEND_REL, rel, meta, nvars, res);

        return res;
    }

    /* Get right and down */
    MDD right = lddmc_false;
    MDD down  = lddmc_false;
    lddmc_refs_pushptr(&right);
    lddmc_refs_pushptr(&down);
    uint32_t value;
    if (rel == lddmc_false || rel == lddmc_true) {
        right = lddmc_false;
        down  = rel;
    }
    else if (meta_val == 0) {
        // for meta_val = 0, the node in the rel is skipped
        right = lddmc_false;
        down  = rel;
    }
    else {
        mddnode_t node = LDD_GETNODE(rel);
        right = mddnode_getright(node);
        down  = mddnode_getdown(node);
        value = mddnode_getvalue(node);
    }

    /* Get next meta */
    MDD next_meta = next_meta = lddmc_getdown(meta);
    uint32_t next_nvars;
    if (meta_val == 1 || meta_val == 2)
        next_nvars = nvars - 1; // for a read/write we step 1 var
    else
        next_nvars = nvars - 2; // for meta_val=0, 3, or 4 we step 2 vars

    /* Call function on children */
    assert(right != lddmc_true);
    if (right != lddmc_false && meta_val != 4)
        right = CALL(lddmc_extend_rel, right, meta, nvars);
    down = CALL(lddmc_extend_rel, down, next_meta, next_nvars);

    
    // meta == 'skipped variable' : change into two levels of *
    if (meta_val == 0) {
        down = lddmc_make_copynode(down, lddmc_false);
        res = lddmc_make_copynode(down, lddmc_false);
    }
    // meta == 'read' : change nothing
    else if (meta_val == 1) {
        assert(lddmc_getvalue(next_meta) == 2);
        if (lddmc_iscopy(rel))
            res = lddmc_make_copynode(down, right);
        else {
            if (lddmc_iscopy(down)) {
                // replace [read 'a' -> copy] with [read 'a' -> write 'a']
                mddnode_t n_down = LDD_GETNODE(down);
                down = lddmc_makenode(value, mddnode_getdown(n_down), mddnode_getright(n_down));
            }
            res = lddmc_makenode(value, down, right);
        }
    }
    // meta == 'write' : change nothing
    else if (meta_val == 2) {
        //assert(!lddmc_iscopy(rel) && "meta_val == 2");
        if (lddmc_iscopy(rel)) {
            // this returns a copy node, then at the read level (1)
            // it is converted to an explicit write
            res = lddmc_make_copynode(down, right);
        }
        else {
            res = lddmc_makenode(value, down, right);
        }
    }
    // replace [only-read 'a'] with [read 'a', write 'a']
    else if (meta_val == 3) {
        if (lddmc_iscopy(rel)) {
            down = lddmc_make_copynode(down, lddmc_false);
            res  = lddmc_make_copynode(down, right);
        }
        else {
            down = lddmc_makenode(value, down, lddmc_false); // write 'value'
            res  = lddmc_makenode(value, down, right);       // read 'value' on top
        }
    }
    else if (meta_val == 4) {
        // if meta = 4, use this helper to avoid recursive calls to extend on
        // right-children
        //assert(!lddmc_iscopy(rel) && "meta_val == 4");
        right = CALL(only_read_helper, right, next_meta, next_nvars);
        if (lddmc_iscopy(rel)) {
            down = lddmc_make_copynode(down, right);
            res = lddmc_make_copynode(down, lddmc_false);
        }
        else {
            down = lddmc_makenode(value, down, right);
            res = lddmc_make_copynode(down, lddmc_false);
        }
    }
    else {
        printf("Unexpected meta val = %d\n", meta_val);
        exit(1);
    }

    lddmc_refs_popptr(2);

     /* Put in cache */
    cache_put3(CACHE_LDD_EXTEND_REL, rel, meta, nvars, res);

    return res;
}
