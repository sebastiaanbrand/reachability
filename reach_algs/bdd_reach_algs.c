#include "bdd_reach_algs.h"
#include "cache_op_ids.h"

#include <sylvan_int.h>


/**
 * Partition relation r into r00, r01, r10, and r11
 */
static void
partition_rel(BDD r, BDDVAR topvar, BDD *r00, BDD *r01, BDD *r10, BDD *r11)
{
    // Check if unprimed var is skipped
    BDD r0, r1;
    if (!sylvan_isconst(r)) {
        bddnode_t n = MTBDD_GETNODE(r);
        if (bddnode_getvariable(n) == topvar) {
            r0 = node_low(r, n);
            r1 = node_high(r, n);
        } else {
            r0 = r1 = r;
        }
    } else {
        r0 = r1 = r;
    }

    // Check if primed var is skipped
    if (!sylvan_isconst(r0)) {
        bddnode_t n0 = MTBDD_GETNODE(r0);
        if (bddnode_getvariable(n0) == topvar + 1) {
            *r00 = node_low(r0, n0);
            *r01 = node_high(r0, n0);
        } else {
            *r00 = *r01 = r0;
        }
    } else {
        *r00 = *r01 = r0;
    }
    if (!sylvan_isconst(r1)) {
        bddnode_t n1 = MTBDD_GETNODE(r1);
        if (bddnode_getvariable(n1) == topvar + 1) {
            *r10 = node_low(r1, n1);
            *r11 = node_high(r1, n1);
        } else {
            *r10 = *r11 = r1;
        }
    } else {
        *r10 = *r11 = r1;
    }
}

/**
 * Store list of BDDs into an MDD to obtain a single 64 bit (40 bit) cache key.
 */
static MDD
hash_rel_list(BDD* r, int m)
{
    // NOTE: BDD complement bit is lost when storing in MDD, since
    // combining MDDs and BDDs is not really intended.
    // As a workaround, we encode the complement bit in the MDD value:
    // - MDD value % 2 == 1  <-->  BDD had complement bit
    // - MDD value % 2 == 0  <-->  BDD had no complement bit
    MDD rels = lddmc_false;
    for (int k = m-1; k >= 0; k--) {
        if (mtbdd_hascomp(r[k])) {
            rels = lddmc_makenode(2*k+1, r[k], rels);
        }
        else {
            rels = lddmc_makenode(2*k, r[k], rels);
        }
    }
    return rels;
}

/**
 * Get topvar of list of r[k]'s
 */
static BDDVAR
get_topvar_rel_list(BDD* r, int m)
{
    BDDVAR topvar = 0xffffffff;
    for (int k = 0; k < m; k++) {
        bddnode_t node = sylvan_isconst(r[k]) ? 0 : MTBDD_GETNODE(r[k]);
        BDDVAR v = node ? bddnode_getvariable(node) : 0xffffffff;
        topvar = v < topvar ? v : topvar;
    }
    return topvar;
}


/**
 * Partition states s into s0 and s1
 * TODO: maybe pass nodes so that this function doesn't need to call get_node
 */
static void
partition_state(BDD s, BDDVAR topvar, BDD *s0, BDD *s1)
{
    // Check if topvar is skipped
    if (!sylvan_isconst(s)) {
        bddnode_t n = MTBDD_GETNODE(s);
        BDDVAR var = bddnode_getvariable(n);
        if (var == topvar) {
            *s0 = node_low(s, n);
            *s1 = node_high(s, n);
        } else {
            assert(var > topvar);
            *s0 = *s1 = s;
        }
    } else {
        *s0 = *s1 = s;
    }
}

/**
 * ReachBDD: Implementation of recursive reachability algorithm for a single 
 * global relation.
 */
TASK_IMPL_4(BDD, go_rec, BDD, s, BDD, r, BDDSET, vars, bool, par)
{
    /* Terminal cases */
    if (s == sylvan_false) return sylvan_false; // empty.R* = empty
    if (r == sylvan_false) return s; // s.empty* = s.(empty union I)^+ = s
    if (s == sylvan_true || r == sylvan_true) return sylvan_true;
    // all.r* = all, s.all* = all (if s is not empty)

    /* Consult cache */
    int cachenow = 1;
    if (cachenow) {
        BDD res;
        if (cache_get3(CACHE_BDD_REACH, s, r, 0, &res)) {
            return res;
        }
    }

    /* Determine top level */
    bddnode_t ns = sylvan_isconst(s) ? 0 : MTBDD_GETNODE(s);
    bddnode_t nr = sylvan_isconst(r) ? 0 : MTBDD_GETNODE(r);

    BDDVAR vs = ns ? bddnode_getvariable(ns) : 0xffffffff;
    BDDVAR vr = nr ? bddnode_getvariable(nr) : 0xffffffff;
    BDDVAR level = vs < vr ? vs : vr;

    /* Relations, states, and vars for next level of recursion */
    BDD r00, r01, r10, r11, s0, s1;
    BDDSET next_vars = vars;
    while (sylvan_set_first(next_vars) < level) {
        next_vars = sylvan_set_next(next_vars);
    }
    bdd_refs_pushptr(&next_vars);

    partition_rel(r, level, &r00, &r01, &r10, &r11);
    partition_state(s, level, &s0, &s1);

    bdd_refs_pushptr(&s0);
    bdd_refs_pushptr(&s1);
    bdd_refs_pushptr(&r00);
    bdd_refs_pushptr(&r01);
    bdd_refs_pushptr(&r10);
    bdd_refs_pushptr(&r11);

    BDD prev0 = sylvan_false;
    BDD prev1 = sylvan_false;
    bdd_refs_pushptr(&prev0);
    bdd_refs_pushptr(&prev1);

    while (s0 != prev0 || s1 != prev1) {
        prev0 = s0;
        prev1 = s1;

        if (!par) {
            // sequential calls (in specific order)
            s0 = CALL(go_rec, s0, r00, next_vars, par);
            s1 = sylvan_or(s1, sylvan_relnext(s0, r01, next_vars));
            s1 = CALL(go_rec, s1, r11, next_vars, par);
            s0 = sylvan_or(s0, sylvan_relnext(s1, r10, next_vars));
        }
        else { // par
            // 2 recursive REACH calls in parallel
            bdd_refs_spawn(SPAWN(go_rec, s0, r00, next_vars, par));
            s1 = CALL(go_rec, s1, r11, next_vars, par);
            s0 = bdd_refs_sync(SYNC(go_rec)); // syncs s0 = s0.r00*

            // 2 relnext calls in parallel
            bdd_refs_spawn(SPAWN(sylvan_relnext, s0, r01, next_vars, 0));
            BDD t0 = CALL(sylvan_relnext, s1, r10, next_vars, 0);
            bdd_refs_push(t0);
            BDD t1 = bdd_refs_sync(SYNC(sylvan_relnext)); // syncs t1 = s0.r01
            bdd_refs_push(t1);

            // 2 or's in parallel ( or is implemented via !(!A ^ !B) )
            bdd_refs_spawn(SPAWN(sylvan_and, sylvan_not(s0), sylvan_not(t0), 0));
            s1 = sylvan_not(CALL(sylvan_and, sylvan_not(s1), sylvan_not(t1), 0));
            s0 = sylvan_not(bdd_refs_sync(SYNC(sylvan_and))); // syncs s0 = !(!s0 ^ !t0)

            bdd_refs_pop(2); // pops t0, t1
        }
    }

    bdd_refs_popptr(9);

    /* res = ((!level) ^ s0)  v  ((level) ^ s1) */
    BDD res = sylvan_makenode(level, s0, s1);

    /* Put in cache */
    if (cachenow)
        cache_put3(CACHE_BDD_REACH, s, r, 0, res);

    return res;
}


/**
 * Compute the union of images, i.e. R_0.S v R_1.S v ... v R_{k-1}.S
 */
TASK_4(BDD, relnext_union_naive, BDD, s, BDD*, r, int, m, BDDSET, vars)
{
    BDD res = sylvan_false;
    for (int k = 0; k < m; k++) {
        res = sylvan_or(res, sylvan_relnext(s, r[k], vars));
    }
    return res;
}

/**
 * Compute S.(R_0 v R_1 v ... v R_{m-1}) "natively" inside the relnext op
 * (Assumes all vars in r)
 */
TASK_IMPL_3(BDD, relnext_union, BDD, s, BDD*, r, int, m)
{
    /* Terminal cases */
    if (s == sylvan_false) return sylvan_false;
    bool all_rk_false = true;
    bool exist_rk_true = false;
    for (int k = 0; k < m; k++) {
        if (r[k] == sylvan_true) exist_rk_true = true;
        if (r[k] != sylvan_false) all_rk_false = false;
    }
    if (s == sylvan_true && exist_rk_true) return sylvan_true;
    if (all_rk_false) return sylvan_false;

    /* Consult cache */
    int cachenow = 1;
    if (cachenow) {
        BDD res;
        if (cache_get3(CACHE_BDD_RELNEXT_UNION, s, hash_rel_list(r, m), 0, &res)) {
            return res;
        }
    }

    /* Determine top level */
    bddnode_t ns = sylvan_isconst(s) ? 0 : MTBDD_GETNODE(s);
    BDDVAR vs = ns ? bddnode_getvariable(ns) : 0xffffffff;
    BDDVAR vr = get_topvar_rel_list(r, m);
    BDDVAR level = vs < vr ? vs : vr;
    level = level % 2 == 1 ? level-1 : level; // only sync on even (unprimed) levels

    /* Relations, states, and vars for next level of recursion */
    BDD s0, s1;
    partition_state(s, level, &s0, &s1);
    BDD *r00 = (BDD*)malloc(sizeof(BDD) * m);
    BDD *r01 = (BDD*)malloc(sizeof(BDD) * m);
    BDD *r10 = (BDD*)malloc(sizeof(BDD) * m);
    BDD *r11 = (BDD*)malloc(sizeof(BDD) * m);
    for (int k = 0; k < m; k++) {
        partition_rel(r[k], level, &(r00[k]), &(r01[k]), &(r10[k]), &(r11[k]));
    }

    bdd_refs_spawn(SPAWN(relnext_union, s0, r00, m));
    bdd_refs_spawn(SPAWN(relnext_union, s0, r01, m));
    bdd_refs_spawn(SPAWN(relnext_union, s1, r10, m));
    BDD t11 = bdd_refs_push(CALL(relnext_union, s1, r11, m));
    BDD t10 = bdd_refs_push(bdd_refs_sync(SYNC(relnext_union)));
    BDD t01 = bdd_refs_push(bdd_refs_sync(SYNC(relnext_union)));
    BDD t00 = bdd_refs_push(bdd_refs_sync(SYNC(relnext_union)));

    bdd_refs_spawn(SPAWN(sylvan_and, sylvan_not(t00), sylvan_not(t10), 0));
    BDD t1 = bdd_refs_push(sylvan_not(CALL(sylvan_and, sylvan_not(t01), sylvan_not(t11), 0)));
    BDD t0 = bdd_refs_push(sylvan_not(bdd_refs_sync(SYNC(sylvan_and))));

    bdd_refs_pop(6);
    free(r00);
    free(r01);
    free(r10);
    free(r11);

    // /* res = ((!level) ^ s0)  v  ((level) ^ s1) */
    BDD res = sylvan_makenode(level, t0, t1);

    // /* Put in cache */
    if (cachenow)
        cache_put3(CACHE_BDD_RELNEXT_UNION, s, hash_rel_list(r, m), 0, res);

    return res;
}


/**
 * ReachBDD-Union: compute S.(R_0 v R_1 v ... v R_{m-1})* recursively.
 */
TASK_IMPL_5(BDD, go_rec_union, BDD, s, BDD*, r, int, m, BDDSET, vars, bool, par)
{
    /* Terminal cases */
    if (s == sylvan_false) return sylvan_false; // empty.R* = empty
    if (s == sylvan_true) return sylvan_true; // all.r* = all
    bool all_rk_false = true;
    for (int k = 0; k < m; k++) {
        if (r[k] == sylvan_true) return sylvan_true; // s.all* = all
        if (r[k] != sylvan_false) {
            all_rk_false = false;
            break;
        }
    }
    if (all_rk_false) return s; // s.empty* = s

    /* Consult cache */
    int cachenow = 1;
    if (cachenow) {
        BDD res;
        if (cache_get3(CACHE_BDD_REACH_UNION, s, hash_rel_list(r, m), 0, &res)) {
            return res;
        }
    }

    /* Determine top level */
    bddnode_t ns = sylvan_isconst(s) ? 0 : MTBDD_GETNODE(s);
    BDDVAR vs = ns ? bddnode_getvariable(ns) : 0xffffffff;
    BDDVAR vr = get_topvar_rel_list(r, m);
    BDDVAR level = vs < vr ? vs : vr;
    level = level % 2 == 1 ? level-1 : level; // only sync on even (unprimed) levels

    /* Relations, states, and vars for next level of recursion */
    BDD s0, s1;
    BDDSET next_vars = vars;
    while (sylvan_set_first(next_vars) < level) {
        next_vars = sylvan_set_next(next_vars);
    }
    partition_state(s, level, &s0, &s1);
    BDD *r00 = (BDD*)malloc(sizeof(BDD) * m);
    BDD *r01 = (BDD*)malloc(sizeof(BDD) * m);
    BDD *r10 = (BDD*)malloc(sizeof(BDD) * m);
    BDD *r11 = (BDD*)malloc(sizeof(BDD) * m);
    for (int k = 0; k < m; k++) {
        partition_rel(r[k], level, &(r00[k]), &(r01[k]), &(r10[k]), &(r11[k]));
    }

    BDD prev0 = sylvan_false;
    BDD prev1 = sylvan_false;
    bdd_refs_pushptr(&prev0);
    bdd_refs_pushptr(&prev1);
    bdd_refs_pushptr(&s0);
    bdd_refs_pushptr(&s1);

    while (s0 != prev0 || s1 != prev1) {
         prev0 = s0;
         prev1 = s1;

        if (!par) {
            // sequential calls (in specific order)
            s0 = CALL(go_rec_union, s0, r00, m, next_vars, par);
            s1 = sylvan_or(s1, CALL(relnext_union, s0, r01, m));
            s1 = CALL(go_rec_union, s1, r11, m, next_vars, par);
            s0 = sylvan_or(s0, CALL(relnext_union, s1, r10, m));
        }
        else { // par
            // 2 recursive REACH calls in parallel
            bdd_refs_spawn(SPAWN(go_rec_union, s0, r00, m, next_vars, par));
            s1 = CALL(go_rec_union, s1, r11, m, next_vars, par);
            s0 = bdd_refs_sync(SYNC(go_rec_union)); // syncs s0 = s0.r00*

            // 2 relnext calls in parallel
            bdd_refs_spawn(SPAWN(relnext_union, s0, r01, m));
            BDD t0 = CALL(relnext_union, s1, r10, m);
            bdd_refs_push(t0);
            BDD t1 = bdd_refs_sync(SYNC(relnext_union)); // syncs t1 = s0.r01
            bdd_refs_push(t1);

            // 2 or's in parallel ( or is implemented via !(!A ^ !B) )
            bdd_refs_spawn(SPAWN(sylvan_and, sylvan_not(s0), sylvan_not(t0), 0));
            s1 = sylvan_not(CALL(sylvan_and, sylvan_not(s1), sylvan_not(t1), 0));
            s0 = sylvan_not(bdd_refs_sync(SYNC(sylvan_and))); // syncs s0 = !(!s0 ^ !t0)

            bdd_refs_pop(2); // pops t0, t1
        }
    }

    bdd_refs_popptr(4);
    free(r00);
    free(r01);
    free(r10);
    free(r11);

    // /* res = ((!level) ^ s0)  v  ((level) ^ s1) */
    BDD res = sylvan_makenode(level, s0, s1);

    // /* Put in cache */
    if (cachenow)
        cache_put3(CACHE_BDD_REACH_UNION, s, hash_rel_list(r, m), 0, res);

    return res;
}


/**
 * Implementation of recursive reachability algorithm for a partial relation
 * over given vars.
 */
TASK_IMPL_3(BDD, go_rec_partial, BDD, s, BDD, r, BDDSET, vars)
{
    /* Terminal cases */
    if (s == sylvan_false) return sylvan_false; // empty.R* = empty
    if (r == sylvan_false) return s; // s.empty* = s.(empty union I)^+ = s
    if (s == sylvan_true || r == sylvan_true) return sylvan_true;
    // all.r* = all, s.all* = all (if s is non empty)
    if (sylvan_set_isempty(vars)) return s;

    /* Consult cache */
    int cachenow = 1;
    if (cachenow) {
        BDD res;
        // TODO: put these op-ids in a headerfile somewhere
        if (cache_get3(CACHE_BDD_REACH_PARTIAL, s, r, vars, &res)) {
            return res;
        }
    }

    /* Determine top level */
    bddnode_t ns = sylvan_isconst(s) ? 0 : MTBDD_GETNODE(s);
    bddnode_t nr = sylvan_isconst(r) ? 0 : MTBDD_GETNODE(r);

    BDDVAR vs = ns ? bddnode_getvariable(ns) : 0xffffffff;
    BDDVAR vr = nr ? bddnode_getvariable(nr) : 0xffffffff;
    BDDVAR level = vs < vr ? vs : vr;
    level = level % 2 == 1 ? level-1 : level; // only sync on even (unprimed) levels

    /* Skip variables not in `vars` */
    int is_s_or_t = 0;
    bddnode_t nv = 0;
    if (vars == sylvan_false) { // use all variables when vars == sylvan_false
        is_s_or_t = 1;
    } else {
        nv = MTBDD_GETNODE(vars);
        for (;;) {
            // check if level = (s)ource or (t)arget variable
            BDDVAR vv = bddnode_getvariable(nv);
            if (level == vv || (level^1) == vv) {
                is_s_or_t = 1;
                break;
            }
            // check if level < s or t
            if (level < vv) break;
            vars = node_high(vars, nv); // get next in vars
            if (sylvan_set_isempty(vars)) return s;
            nv = MTBDD_GETNODE(vars);
        }
    }

    //if (next_count == 1) {
    //    assert(is_s_or_t == 1);
    //}

    BDD res;

    if (is_s_or_t) {
        /* Relations, states, and vars for next level of recursion */
        BDD r00, r01, r10, r11, s0, s1;
        //BDDSET next_vars = sylvan_set_next(vars);
        BDDSET next_vars = vars == sylvan_false ? sylvan_false : node_high(vars, nv);
        
        partition_rel(r, level, &r00, &r01, &r10, &r11);
        partition_state(s, level, &s0, &s1);

        bdd_refs_pushptr(&s0);
        bdd_refs_pushptr(&s1);
        bdd_refs_pushptr(&r00);
        bdd_refs_pushptr(&r01);
        bdd_refs_pushptr(&r10);
        bdd_refs_pushptr(&r11);

        BDD prev0 = sylvan_false;
        BDD prev1 = sylvan_false;
        bdd_refs_pushptr(&prev0);
        bdd_refs_pushptr(&prev1);

        while (s0 != prev0 || s1 != prev1) {
            prev0 = s0;
            prev1 = s1;

            /* Do in parallel */
            bdd_refs_spawn(SPAWN(go_rec_partial, s0, r00, next_vars));
            bdd_refs_spawn(SPAWN(sylvan_relnext, s0, r01, next_vars, 0));
            bdd_refs_spawn(SPAWN(sylvan_relnext, s1, r10, next_vars, 0));
            bdd_refs_spawn(SPAWN(go_rec_partial, s1, r11, next_vars));

            BDD t11 = bdd_refs_sync(SYNC(go_rec_partial));  bdd_refs_push(t11);
            BDD t10 = bdd_refs_sync(SYNC(sylvan_relnext));  bdd_refs_push(t10);
            BDD t01 = bdd_refs_sync(SYNC(sylvan_relnext));  bdd_refs_push(t01);
            BDD t00 = bdd_refs_sync(SYNC(go_rec_partial));  bdd_refs_push(t00);

            /* Union with previously reachable set */
            s0 = sylvan_or(s0, t00);
            s0 = sylvan_or(s0, t10);
            s1 = sylvan_or(s1, t11);
            s1 = sylvan_or(s1, t01);
            bdd_refs_pop(4);
        }

        bdd_refs_popptr(8);

        /* res = ((!level) ^ s0)  v  ((level) ^ s1) */
        res = sylvan_makenode(level, s0, s1);
    } else {
        // (copied from rel_next)
        /* Variable not in vars! Take s, quantify r */
        // TODO: replace with calls to "parition_state" (?)
        BDD s0, s1, r0, r1;
        if (ns && vs == level) {
            s0 = node_low(s, ns);
            s1 = node_high(s, ns);
        } else {
            s0 = s1 = s;
        }
        if (nr && vr == level) {
            r0 = node_low(r, nr);
            r1 = node_high(r, nr);
        } else {
            r0 = r1 = r;
        }

        if (r0 != r1) {
            if (s0 == s1) {
                /* Quantify "r" variables */
                bdd_refs_spawn(SPAWN(go_rec_partial, s0, r0, vars));
                bdd_refs_spawn(SPAWN(go_rec_partial, s1, r1, vars));

                BDD res1 = bdd_refs_sync(SYNC(go_rec_partial)); bdd_refs_push(res1);
                BDD res0 = bdd_refs_sync(SYNC(go_rec_partial)); bdd_refs_push(res0);
                res = sylvan_or(res0, res1);
                bdd_refs_pop(2);
            } else {
                /* Quantify "r" variables, but keep "a" variables */
                bdd_refs_spawn(SPAWN(go_rec_partial, s0, r0, vars));
                bdd_refs_spawn(SPAWN(go_rec_partial, s0, r1, vars));
                bdd_refs_spawn(SPAWN(go_rec_partial, s1, r0, vars));
                bdd_refs_spawn(SPAWN(go_rec_partial, s1, r1, vars));

                BDD res11 = bdd_refs_sync(SYNC(go_rec_partial)); bdd_refs_push(res11);
                BDD res10 = bdd_refs_sync(SYNC(go_rec_partial)); bdd_refs_push(res10);
                BDD res01 = bdd_refs_sync(SYNC(go_rec_partial)); bdd_refs_push(res01);
                BDD res00 = bdd_refs_sync(SYNC(go_rec_partial)); bdd_refs_push(res00);

                bdd_refs_spawn(SPAWN(sylvan_ite, res00, sylvan_true, res01, 0));
                bdd_refs_spawn(SPAWN(sylvan_ite, res10, sylvan_true, res11, 0));

                BDD res1 = bdd_refs_sync(SYNC(sylvan_ite)); bdd_refs_push(res1);
                BDD res0 = bdd_refs_sync(SYNC(sylvan_ite));
                bdd_refs_pop(5);

                res = sylvan_makenode(level, res0, res1);
            }
        } else { // r0 == r1
            /* Keep "s" variables */
            bdd_refs_spawn(SPAWN(go_rec_partial, s0, r0, vars));
            bdd_refs_spawn(SPAWN(go_rec_partial, s1, r1, vars));

            BDD res1 = bdd_refs_sync(SYNC(go_rec_partial)); bdd_refs_push(res1);
            BDD res0 = bdd_refs_sync(SYNC(go_rec_partial));
            bdd_refs_pop(1);
            res = sylvan_makenode(level, res0, res1);
        }
    }

    /* Put in cache */
    if (cachenow)
        cache_put3(CACHE_BDD_REACH_PARTIAL, s, r, vars, res);

    return res;
}

TASK_IMPL_5(BDD, go_bfs_plain, BDD, s, BDD, r, BDD, t, BDDSET, vars, int*, steps) 
{
    BDD reachable = s;
    BDD prev = sylvan_false;
    BDD successors = sylvan_false;

    sylvan_protect(&reachable);
    sylvan_protect(&prev);
    sylvan_protect(&successors);

    int k = 0;
    while (prev != reachable) {
        k++;
        prev = reachable;
        successors = sylvan_relnext(reachable, r, vars);
        reachable = sylvan_or(reachable, successors);

        if (sylvan_and(t, reachable)) {
            *steps = k;
            break;
        }
    }

    sylvan_unprotect(&reachable);
    sylvan_unprotect(&prev);
    sylvan_unprotect(&successors);

    return reachable;
}
