#include "ldd_custom.h"
#include "cache_op_ids.h"

#define Abort(...) { fprintf(stderr, __VA_ARGS__); fprintf(stderr, "Abort at line %d!\n", __LINE__); exit(-1); }


MDD lddmc_make_normalnode(uint32_t value, MDD ifeq, MDD ifneq)
{
    assert(value < 1<<30);
    return lddmc_makenode(value, ifeq, ifneq);
}


MDD lddmc_make_homomorphism_node(uint32_t value, bool sign, MDD ifeq, MDD ifneq)
{
    assert(value < (uint32_t)(1<<29)); // top two bits are reserved
    value = value | 0x80000000; // first bit is hmorph flag
    if (sign) value = value | 0x40000000; // second bit is sign
    return lddmc_makenode(value, ifeq, ifneq);
}


bool lddmc_is_homomorphism(uint32_t *value)
{
    uint32_t flag = (*value) & 0x80000000;
    *value = (*value) & 0x7fffffff; // remove flag from value
    return (bool) flag; // flag = 1 represents homomorphism
}


bool lddmc_hmorph_get_sign(uint32_t * value)
{
    uint32_t sign = (*value) & 0x40000000;
    *value = (*value) & 0xbfffffff; // remove sign flag from value
    return (bool) sign;
}


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


static MDD 
apply_write(uint32_t read, uint32_t write, MDD down)
{
    MDD res;
    if (lddmc_is_homomorphism(&write)) {
        if (lddmc_hmorph_get_sign(&write)) {
            // "read i, write i-j, if i-j>=0"
            if ((int) read - (int) write >= 0) {
                res = lddmc_make_normalnode(read - write, down, lddmc_false);  
            } else {
                res = lddmc_false;
            }
        } else { // "read i, write i+j"
            res = lddmc_make_normalnode(read + write, down, lddmc_false); 
        }
    }
    else { // normal write
        res = lddmc_make_normalnode(write, down, lddmc_false);
    }
    return res;
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
    // NOTE: we cannot sync because of possible hmorph nodes
    //if (!lddmc_iscopy(rel)) {
    //    if (!match_ldds(&set, &rel)) return lddmc_false;
    //}

    /* Consult cache */
    MDD res = lddmc_false;
    if (cache_get3(CACHE_LDD_IMAGE, set, rel, 0, &res)) return res;

    MDD next_meta = get_next_meta(meta);

    // TODO: rename itr_r, itr_w to itr_i, itr_j ?
    // TODO: maybe we could do with fewer helper MDDs in this function
    MDD _rel   = rel;           lddmc_refs_pushptr(&_rel);
    MDD itr_r  = lddmc_false;   lddmc_refs_pushptr(&itr_r);
    MDD itr_w  = lddmc_false;   lddmc_refs_pushptr(&itr_w);
    MDD itr_3  = lddmc_false;   lddmc_refs_pushptr(&itr_3);
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
                tmp = lddmc_make_normalnode(i, tmp, lddmc_false);
                res = lddmc_union(res, tmp);
            }

            // After (* -> *), there might still be (* -> j)'s
            rel_i = lddmc_getright(rel_i);
        }
        // rel = (* -> j), i.e. read anything, write j

        // Iterate over all reads of 'set'
        // TODO: we only need to iterate over homomorphism nodes (although they are currently at the back)
        for (itr_r = set; itr_r != lddmc_false; itr_r = lddmc_getright(itr_r)) {
            
            uint32_t i = lddmc_getvalue(itr_r);
            set_i = lddmc_getdown(itr_r);

            // Iterate over all writes (j) of rel
            for (itr_w = rel_i; itr_w != lddmc_false; itr_w = lddmc_getright(itr_w)) {

                uint32_t j = lddmc_getvalue(itr_w);
                rel_ij = lddmc_getdown(itr_w); // equiv to following * then j

                // Compute successors T_j = S_*.R_*j
                tmp = CALL(lddmc_image, set_i, rel_ij, next_meta);

                // Write j (or some hmorph) and add to successors
                tmp = apply_write(i, j, tmp); // TODO: this should always be a normal write, so shouln't need this function here
                res = CALL(lddmc_union, res, tmp);
            }
        }
        _rel = lddmc_getright(_rel);
    }

    // Iterate over all reads (i) of 'rel'
    for (itr_r = _rel; itr_r != lddmc_false; itr_r = lddmc_getright(itr_r)) {

        uint32_t i = lddmc_getvalue(itr_r);
    
        // hmorph on read level is interpreted as "can sync on all reads >= i"
        if (lddmc_is_homomorphism(&i)) {

            // Iterate over all writes (j) corresponding to this read
            for (itr_w = lddmc_getdown(itr_r); itr_w != lddmc_false; itr_w = lddmc_getright(itr_w)) {

                uint32_t j = lddmc_getvalue(itr_w);

                // Iterate over all reads (v) of set
                for (itr_3 = set; itr_3 != lddmc_false; itr_3 = lddmc_getright(itr_3)) {
                    uint32_t v = lddmc_getvalue(itr_3);
                    if (v >= i) {
                        // Compute successors T_j = S_i.R_ij
                        rel_ij = lddmc_getdown(itr_w);
                        set_i = lddmc_getdown(itr_3);
                        tmp = CALL(lddmc_image, set_i, rel_ij, next_meta);

                        // Write (hmorph) j and add to successors
                        tmp = apply_write(v, j, tmp);
                        res = CALL(lddmc_union, res, tmp);
                    }
                }
            }

        }
        else { // THIS IS THE ACTUAL NORMAL "READ i, WRITE j"
            set_i = lddmc_follow(set, i); // NOTE: this is not efficient, since
            // lddmc_follow needs to iterate 'set' from the beginning each time
            // (need to iterate over set and reads of rel at the same time)          

            // Iterate over all writes (j) corresponding to this read
            for (itr_w = lddmc_getdown(itr_r); itr_w != lddmc_false; itr_w = lddmc_getright(itr_w)) {

                uint32_t j = lddmc_getvalue(itr_w);
                //printf("normal write j = %d\n", j);
                rel_ij = lddmc_getdown(itr_w); // equiv to following i then j

                // Compute successors T_j = S_i.R_ij
                tmp = CALL(lddmc_image, set_i, rel_ij, next_meta);
            
                // Extend succ_j and add to successors
                tmp = lddmc_make_normalnode(j, tmp, lddmc_false);
                res = lddmc_union(res, tmp);
            }
        }
    }

    lddmc_refs_popptr(9);

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
            uint32_t j;
            if (lddmc_iscopy(_rel)) j = lddmc_getvalue(_set);
            else j = lddmc_getvalue(_rel);

            // down recursive call
            tmp = CALL(lddmc_image2, lddmc_getdown(_set), lddmc_getdown(_rel), next_meta);
            
            // Write j (or some hmorph) and add to successors
            tmp = apply_write(lddmc_getvalue(_set), j, tmp);
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
    uint32_t value = mddnode_getvalue(node);
    assert(!mddnode_getcopy(node) && "there should be no copy-nodes to the right");
    
    lddmc_refs_pushptr(&next_right);
    lddmc_refs_pushptr(&down);
    next_right = CALL(only_read_helper, next_right, next_meta, next_nvars);
    down       = CALL(lddmc_extend_rel, down, next_meta, next_nvars);
    lddmc_refs_popptr(2);
    return lddmc_makenode(value, down, next_right);
}


TASK_IMPL_3(MDD, lddmc_extend_rel, MDD, rel, MDD, meta, uint32_t, nvars)
{
    // No need to extend an empty relation, as this is a terminal case for
    // REACH, union, and custom_image.
    if (rel == lddmc_false) return lddmc_false;

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
    MDD next_meta = lddmc_getdown(meta);
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

TASK_IMPL_2(MDD, lddmc_rel_union, MDD, a, MDD, b)
{
    /* Terminal cases */
    if (a == b) return a;
    if (a == lddmc_false) return b;
    if (b == lddmc_false) return a;
    assert(a != lddmc_true && b != lddmc_true); // expect same lenght

    /* Consult cache */
    MDD res;
    if (cache_get3(CACHE_LDD_REL_UNION, a, b, 0, &res)) {
        return res;
    }

    /* Get nodes */
    mddnode_t na = LDD_GETNODE(a);
    mddnode_t nb = LDD_GETNODE(b);

    const int na_copy = mddnode_getcopy(na) ? 1 : 0;
    const int nb_copy = mddnode_getcopy(nb) ? 1 : 0;
    const uint32_t na_value = mddnode_getvalue(na);
    const uint32_t nb_value = mddnode_getvalue(nb);

    /* Recursive calls */
    // (assume read level)
    if (na_copy && nb_copy) {
        MDD down = CALL(lddmc_rel_union, mddnode_getdown(na), mddnode_getdown(nb));
        MDD right = CALL(lddmc_rel_union, mddnode_getright(na), mddnode_getright(nb));
        res = lddmc_make_copynode(down, right);
    }
    else if (na_copy) {
        MDD right = CALL(lddmc_rel_union, mddnode_getright(na), b);
        res = lddmc_make_copynode(mddnode_getdown(na), right);
    }
    else if (nb_copy) {
        MDD right = CALL(lddmc_rel_union, a, mddnode_getright(nb));
        res = lddmc_make_copynode(mddnode_getdown(nb), right);
    }
    else if (na_value < nb_value) {
        MDD right = CALL(lddmc_rel_union, mddnode_getright(na), b);
        res = lddmc_makenode(na_value, mddnode_getdown(na), right);
    }
    else if (na_value == nb_value) {
        MDD down = CALL(lddmc_rel_union, mddnode_getdown(na), mddnode_getdown(nb));
        MDD right = CALL(lddmc_rel_union, mddnode_getright(na), mddnode_getright(nb));
        res = lddmc_makenode(na_value, down, right);
    }
    else /* na_value > nb_value */ {
        MDD right = CALL(lddmc_rel_union, a, mddnode_getright(nb));
        res = lddmc_makenode(nb_value, mddnode_getdown(nb), right);
    }

    /* Put in cache */
    cache_put3(CACHE_LDD_REL_UNION, a, b, 0, res);

    return res;
}


/**
 * ReachLDD: Implementation of recursive reachability algorithm for a single 
 * global relation.
 */
TASK_IMPL_4(MDD, go_rec, MDD, set, MDD, rel, MDD, meta, int, img)
{
    /* Terminal cases */
    if (set == lddmc_false) return lddmc_false; // empty.R* = empty
    if (rel == lddmc_false) return set; // s.empty* = s.(empty union I)^+ = s
    if (set == lddmc_true || rel == lddmc_true) return lddmc_true;
    // all.r* = all, s.all* = all (if s is not empty)

    /* Assert assumptions about rel */
    assert(lddmc_getvalue(meta) == 1);

    /* We can skip nodes if Rel requires a "read" which is not in S */
    //if (!match_ldds(&set, &rel)) return lddmc_false;

    /* Consult cache */
    int cachenow = 1;
    if (cachenow) {
        MDD res;
        if (cache_get3(CACHE_LDD_REACH, set, rel, 0, &res)) return res;
    }
    
    /* Protect relevant MDDs */
    MDD next_meta = get_next_meta(meta); // Q: protect this?
    MDD prev      = lddmc_false;    lddmc_refs_pushptr(&prev);
    MDD itr_r     = lddmc_false;    lddmc_refs_pushptr(&itr_r);
    MDD itr_w     = lddmc_false;    lddmc_refs_pushptr(&itr_w);
    MDD rel_ij    = lddmc_false;    lddmc_refs_pushptr(&rel_ij);
    MDD set_i     = lddmc_false;    lddmc_refs_pushptr(&set_i);
    MDD rel_i     = lddmc_false;    lddmc_refs_pushptr(&rel_i); // might be possible to only use itr_w
    MDD _set      = set;            lddmc_refs_pushptr(&_set);
    MDD _rel      = rel;            lddmc_refs_pushptr(&_rel);

    /* Loop until reachable set has converged */
    while (_set != prev) {
        prev = _set;
        _rel = rel;

        // 1. Iterate over all reads (i) of 'rel'
        // 1a. (possibly a copy node first)
        if (lddmc_iscopy(_rel)) {
            // Current read is a copy node (i.e. interpret as R_ii for all i)
            rel_i = lddmc_getdown(_rel);

            // 2. Iterate over all corresponding writes (j) 
            // 2a. rel = (* -> *), i.e. read anything, write what was read
            if (lddmc_iscopy(rel_i)) {

                rel_ij = lddmc_getdown(rel_i); // j = i

                // Iterate over all reads of 'set'
                for (itr_r = _set; itr_r != lddmc_false; itr_r = lddmc_getright(itr_r)) {
                    uint32_t i = lddmc_getvalue(itr_r);
                    set_i = lddmc_getdown(itr_r);

                    // Compute REACH for S_i.R_ii*
                    set_i = CALL(go_rec, set_i, rel_ij, next_meta, img);

                    // Extend set_i and add to 'set'
                    // TODO: maybe we could use lddmc_extend_node here
                    // instead of makenode + union?
                    MDD set_i_ext = lddmc_make_normalnode(i, set_i, lddmc_false);
                    lddmc_refs_push(set_i_ext);
                    _set = lddmc_union(_set, set_i_ext);
                    lddmc_refs_pop(1);
                }

                // After (* -> *), there might still be (* -> j)'s
                rel_i = lddmc_getright(rel_i);
            }
            // 2b. rel = (* -> j), i.e. read anything, write j
            // Iterate over all reads (i) of 'set'
            _rel = lddmc_getright(_rel);
            for (itr_r = _set; itr_r != lddmc_false; itr_r = lddmc_getright(itr_r)) {

                uint32_t i = lddmc_getvalue(itr_r);
                set_i = lddmc_getdown(itr_r);

                // Iterate over over all writes (j) of rel
                for (itr_w = rel_i; itr_w != lddmc_false; itr_w = lddmc_getright(itr_w)) {
                    
                    uint32_t j = lddmc_getvalue(itr_w);
                    rel_ij = lddmc_getdown(itr_w); // equiv to following * then j

                    if (i == j) { // || is_zero_hmorph(j)
                        // TODO: it might be an hmorph node of +0 (effectively a 
                        // copy node), in this case we can call rec
                        set_i = CALL(go_rec, set_i, rel_ij, next_meta, img);
                    }
                    else {
                        MDD succ_j;
                        if (img == 1)
                            succ_j = lddmc_image(set_i, rel_ij, next_meta);
                        else if (img == 2)
                            succ_j = lddmc_image2(set_i, rel_ij, next_meta);
                        else {
                            Abort("Must use custom image w/ copy nodes in rel\n");
                        }

                        // Write j (or some hmorph) and add to set
                        MDD set_j_ext = apply_write(i, j, succ_j);
                        // Extend succ_j and add to 'set'
                        // TODO: maybe we could use lddmc_extend_node here
                        // instead of makenode + union?
                        lddmc_refs_push(set_j_ext);
                        _set = lddmc_union(_set, set_j_ext);
                        lddmc_refs_pop(1);
                    }
                }

                // Extend set_i and add to 'set'
                // TODO: maybe we could use lddmc_extend_node here
                // instead of makenode + union?
                MDD set_i_ext = lddmc_make_normalnode(i, set_i, lddmc_false);
                lddmc_refs_push(set_i_ext);
                _set = lddmc_union(_set, set_i_ext);
                lddmc_refs_pop(1);
            }
        }
        
        // 1b. normal reads of rel
        for (itr_r = _rel;  itr_r != lddmc_false; itr_r = lddmc_getright(itr_r)) {

            uint32_t i = lddmc_getvalue(itr_r);
            set_i = lddmc_follow(_set, i);

            // Iterate over all writes (j) corresponding to reading 'i'
            for (itr_w = lddmc_getdown(itr_r); itr_w != lddmc_false; itr_w = lddmc_getright(itr_w)) {

                uint32_t j = lddmc_getvalue(itr_w);
                rel_ij = lddmc_getdown(itr_w); // equiv to following i then j from rel
                assert(!lddmc_is_homomorphism(&j)); // we shouldn't have homomorphisms after a normal read
                
                if (i == j) {
                    set_i = CALL(go_rec, set_i, rel_ij, next_meta, img);
                }
                else {
                    // TODO: USE CALL instead of RUN?
                    MDD succ_j;
                    if (img == 1)
                        succ_j = lddmc_image(set_i, rel_ij, next_meta);
                    else if (img == 2)
                        succ_j = lddmc_image2(set_i, rel_ij, next_meta);
                    else
                        succ_j = lddmc_relprod(set_i, rel_ij, next_meta);

                    // Extend succ_j and add to 'set'
                    // TODO: maybe we could use lddmc_extend_node here
                    // instead of makenode + union?
                    MDD set_j_ext = lddmc_make_normalnode(j, succ_j, lddmc_false);
                    lddmc_refs_push(set_j_ext);
                    _set = lddmc_union(_set, set_j_ext);
                    lddmc_refs_pop(1);
                } 
            }

            // Extend set_i and add to 'set'
            // TODO: maybe we could use lddmc_extend_node here
            // instead of makenode + union?
            MDD set_i_ext = lddmc_make_normalnode(i, set_i, lddmc_false);
            lddmc_refs_push(set_i_ext);
            _set = lddmc_union(_set, set_i_ext);
            lddmc_refs_pop(1);
        }
    }

    lddmc_refs_popptr(8);

    /* Put in cache */
    if (cachenow)
        cache_put3(CACHE_LDD_REACH, set, rel, 0, _set);

    return _set;
}


/**
 * REACH but now with right-recursion instead of loop over reads of rel
 */
TASK_IMPL_4(MDD, go_rec2, MDD, set, MDD, rel, MDD, meta, int, img)
{
    /* Terminal cases */
    if (set == lddmc_false) return lddmc_false; // empty.R* = empty
    if (rel == lddmc_false) return set; // s.empty* = s.(empty union I)^+ = s
    if (set == lddmc_true || rel == lddmc_true) return lddmc_true;
    // all.r* = all, s.all* = all (if s is not empty)

    // we'll still deal with the read and write levels in the same recursive call
    assert(lddmc_getvalue(meta) == 1);

    /* Consult cache */
    int cachenow = 1;
    if (cachenow) {
        MDD res;
        if (cache_get3(CACHE_LDD_REACH, set, rel, 0, &res)) return res;
    }

    /* Protect relevant MDDs */
    MDD next_meta = get_next_meta(meta); // Q: protect this?
    MDD prev      = lddmc_false;    lddmc_refs_pushptr(&prev);
    MDD itr_r     = lddmc_false;    lddmc_refs_pushptr(&itr_r);
    MDD itr_w     = lddmc_false;    lddmc_refs_pushptr(&itr_w);
    MDD rel_ij    = lddmc_false;    lddmc_refs_pushptr(&rel_ij);
    MDD set_i     = lddmc_false;    lddmc_refs_pushptr(&set_i);
    MDD rel_i     = lddmc_false;    lddmc_refs_pushptr(&rel_i); // might be possible to only use itr_w
    MDD _set      = set;            lddmc_refs_pushptr(&_set);
    MDD _rel      = rel;            lddmc_refs_pushptr(&_rel);


    /* Loop until reachable set has converged */
    while (_set != prev) {
        prev = _set;

        // 1. Iterate over all reads (i) of 'rel'
        // 1a. (possibly a copy node first)
        if (lddmc_iscopy(_rel)) {
            // Current read is a copy node (i.e. interpret as R_ii for all i)
            rel_i = lddmc_getdown(_rel);

            // 2. Iterate over all corresponding writes (j) 
            // 2a. rel = (* -> *), i.e. read anything, write what was read
            if (lddmc_iscopy(rel_i)) {

                rel_ij = lddmc_getdown(rel_i); // j = i

                // Iterate over all reads of 'set'
                for (itr_r = _set; itr_r != lddmc_false; itr_r = lddmc_getright(itr_r)) {
                    uint32_t i = lddmc_getvalue(itr_r);
                    set_i = lddmc_getdown(itr_r);

                    // Compute REACH for S_i.R_ii*
                    set_i = CALL(go_rec2, set_i, rel_ij, next_meta, img);

                    // Extend set_i and add to 'set'
                    // TODO: maybe we could use lddmc_extend_node here
                    // instead of makenode + union?
                    MDD set_i_ext = lddmc_makenode(i, set_i, lddmc_false);
                    lddmc_refs_push(set_i_ext);
                    _set = lddmc_union(_set, set_i_ext);
                    lddmc_refs_pop(1);
                }

                // After (* -> *), there might still be (* -> j)'s
                rel_i = lddmc_getright(rel_i);
            }
            // 2b. rel = (* -> j), i.e. read anything, write j
            // Iterate over all reads (i) of 'set'
            for (itr_r = _set; itr_r != lddmc_false; itr_r = lddmc_getright(itr_r)) {

                uint32_t i = lddmc_getvalue(itr_r);
                set_i = lddmc_getdown(itr_r);

                // Iterate over over all writes (j) of rel
                for (itr_w = rel_i; itr_w != lddmc_false; itr_w = lddmc_getright(itr_w)) {
                    
                    uint32_t j = lddmc_getvalue(itr_w);
                    rel_ij = lddmc_getdown(itr_w); // equiv to following * then j

                    if (i == j) {
                        set_i = CALL(go_rec2, set_i, rel_ij, next_meta, img);
                    }
                    else {
                        MDD succ_j;
                        if (img == 1)
                            succ_j = lddmc_image(set_i, rel_ij, next_meta);
                        else if (img == 2)
                            succ_j = lddmc_image2(set_i, rel_ij, next_meta);
                        else
                            Abort("Must use custom image w/ copy nodes in rel\n");


                        // Write j (or some hmorph) and add to successors
                        MDD set_j_ext = apply_write(i, j , succ_j);
                        // TODO: maybe we could use lddmc_extend_node here
                        // instead of makenode + union?
                        lddmc_refs_push(set_j_ext);
                        _set = lddmc_union(_set, set_j_ext);
                        lddmc_refs_pop(1);
                    }
                }

                // Extend set_i and add to 'set'
                // TODO: maybe we could use lddmc_extend_node here
                // instead of makenode + union?
                MDD set_i_ext = lddmc_makenode(i, set_i, lddmc_false);
                lddmc_refs_push(set_i_ext);
                _set = lddmc_union(_set, set_i_ext);
                lddmc_refs_pop(1);
            }
        }
        else { // not copy, normal read
            uint32_t i = lddmc_getvalue(_rel);
            set_i = lddmc_follow(_set, i);

            // Iterate over all writes (j) corresponding to reading 'i'
            for (itr_w = lddmc_getdown(_rel); itr_w != lddmc_false; itr_w = lddmc_getright(itr_w)) {

                uint32_t j = lddmc_getvalue(itr_w);
                rel_ij = lddmc_getdown(itr_w); // equiv to following i then j from rel
                
                if (i == j) {
                    set_i = CALL(go_rec2, set_i, rel_ij, next_meta, img);
                }
                else {
                    // TODO: USE CALL instead of RUN?
                    MDD succ_j;
                    if (img == 1)
                        succ_j = lddmc_image(set_i, rel_ij, next_meta);
                    else if (img == 2)
                        succ_j = lddmc_image2(set_i, rel_ij, next_meta);
                    else
                        succ_j = lddmc_relprod(set_i, rel_ij, next_meta);

                    // Extend succ_j and add to 'set'
                    // TODO: maybe we could use lddmc_extend_node here
                    // instead of makenode + union?
                    MDD set_j_ext = lddmc_makenode(j, succ_j, lddmc_false);
                    lddmc_refs_push(set_j_ext);
                    _set = lddmc_union(_set, set_j_ext);
                    lddmc_refs_pop(1);
                } 
            }

            // Extend set_i and add to 'set'
            // TODO: maybe we could use lddmc_extend_node here
            // instead of makenode + union?
            MDD set_i_ext = lddmc_makenode(i, set_i, lddmc_false);
            lddmc_refs_push(set_i_ext);
            _set = lddmc_union(_set, set_i_ext);
            lddmc_refs_pop(1);
        }

        // NEW: instead of iterating over _rel to the right here, recursively call self
        MDD res_right = CALL(go_rec2, _set, lddmc_getright(_rel), meta, img);
        lddmc_refs_push(res_right);
        _set = lddmc_union(_set, res_right);
        lddmc_refs_pop(1);
    }

    lddmc_refs_popptr(8);

    /* Put in cache */
    if (cachenow)
        cache_put3(CACHE_LDD_REACH, set, rel, 0, _set);

    return _set;
}
