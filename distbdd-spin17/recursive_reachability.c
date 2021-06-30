#include "recursive_reachability.h"
#include "sylvan_int.h"

TASK_IMPL_2(BDD, reachable_bfs, BDD, s, BDD, r)
{
    BDD prev = sylvan_false;
    BDD successors = sylvan_false;

    sylvan_protect(&s);
    sylvan_protect(&r);
    sylvan_protect(&prev);
    sylvan_protect(&successors);

    while (prev != s) {
        prev = s;
        successors = sylvan_relnext(s, r, sylvan_false);
        s = sylvan_or(s, successors);
    }

    sylvan_unprotect(&s);
    sylvan_unprotect(&r);
    sylvan_unprotect(&prev);
    sylvan_unprotect(&successors);

    return s;
}

/**
 * Implementation of (parallel) saturation
 * (assumes relations are ordered on first variable)
 * NOTE S: this looks like Alg. 1 from [Multi-core On-The-Fly Saturation]
 */
TASK_IMPL_4(BDD, reachable_sat, BDD, set, rel_t*, rels, int, relcount, int, idx)
{
    /* Terminal cases */
    if (set == sylvan_false) return sylvan_false;
    if (set == sylvan_true) return sylvan_true;
    if (idx == relcount) return set;

    /* Consult the cache */
    BDD result;
    const BDD _set = set;
    if (cache_get3(200LL<<40, _set, idx, 0, &result)) return result;
    mtbdd_refs_pushptr(&_set);

    /**
     * Possible improvement: cache more things (like intermediate results?)
     *   and chain-apply more of the current level before going deeper?
     */

    /* Check if the relation should be applied */
    const uint32_t var = sylvan_var(rels[idx]->variables);
    if (set == sylvan_true || var <= sylvan_var(set)) {
        /* Count the number of relations starting here */
        int count = idx+1;
        while (count < relcount && var == sylvan_var(rels[count]->variables)) count++;
        count -= idx;
        /*
         * Compute until fixpoint:
         * - SAT deeper
         * - chain-apply all current level once
         */
        BDD prev = sylvan_false;
        BDD step = sylvan_false;
        mtbdd_refs_pushptr(&set);
        mtbdd_refs_pushptr(&prev);
        mtbdd_refs_pushptr(&step);
        while (prev != set) {
            prev = set;
            // SAT deeper
            set = CALL(reachable_sat, set, rels, relcount, idx+count);
            // chain-apply all current level once
            for (int i=0;i<count;i++) {
                step = sylvan_relnext(set, rels[idx+i]->bdd, rels[idx+i]->variables);
                set = sylvan_or(set, step);
                step = sylvan_false; // unset, for gc
            }
        }
        mtbdd_refs_popptr(3);
        result = set;
    } else {
        /* Recursive computation */
        mtbdd_refs_spawn(SPAWN(reachable_sat, sylvan_low(set), rels, relcount, idx));
        BDD high = mtbdd_refs_push(CALL(reachable_sat, sylvan_high(set), rels, relcount, idx));
        BDD low = mtbdd_refs_sync(SYNC(reachable_sat));
        mtbdd_refs_pop(1);
        result = sylvan_makenode(sylvan_var(set), low, high);
    }

    /* Store in cache */
    cache_put3(200LL<<40, _set, idx, 0, result);
    mtbdd_refs_popptr(1);
    return result;
}

void
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

void
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

TASK_IMPL_3(BDD, reachable_rec, BDD, s, BDD, r, BDDVAR, curvar)
{
    assert(curvar % 2 == 0);

    /* Terminal cases */
    if (s == sylvan_true && r == sylvan_true) return sylvan_true;
    if (s == sylvan_false || r == sylvan_false) return sylvan_false;

    /* Count operation */
    sylvan_stats_count(BDD_REACHABLE);

    /* Consult cache */
    int cachenow = 1;
    if (cachenow) {
        BDD res;
        if (cache_get3(CACHE_BDD_REACHABLE, s, r, 0, &res)) {
            sylvan_stats_count(BDD_REACHABLE_CACHED);
            return res;
        }
    }

    // optional earlier terminal:
    //if ((nvars-1)*2 == curvar ) { // at last variable
    //    return reachable_bfs(s, r);
    //}


    // Relations, states, and var for next level of recursion
    BDDVAR nextvar = curvar + 2;
    BDD r00, r01, r10, r11, s0, s1;
    // TODO: maybe instead of splitting at every level, only split on (even) 
    // topvar of r,s

    /* Determine top level */
    bddnode_t ns = sylvan_isconst(s) ? 0 : MTBDD_GETNODE(s);
    bddnode_t nr = sylvan_isconst(r) ? 0 : MTBDD_GETNODE(r);

    BDDVAR vs = ns ? bddnode_getvariable(ns) : 0xffffffff;
    BDDVAR vr = nr ? bddnode_getvariable(nr) : 0xffffffff;
    BDDVAR level = vs < vr ? vs : vr;
    //BDDVAR level = curvar;
    partition_rel(r, level, &r00, &r01, &r10, &r11);
    partition_state(s, level, &s0, &s1);

    // TODO: protect relevant BDDs
    BDD prev0 = sylvan_false;
    BDD prev1 = sylvan_false;

    // TODO: maybe we can do this saturation-like loop more efficiently
    while (s0 != prev0 || s1 != prev1) {
        prev0 = s0;
        prev1 = s1;

        // TODO: spawn tasks
        BDD t00 = CALL(reachable_rec, s0, r00, nextvar);
        bdd_refs_push(t00);
        BDD t01 = sylvan_relnext(s0, r01, sylvan_false);
        bdd_refs_push(t01);
        BDD t10 = sylvan_relnext(s1, r10, sylvan_false);
        bdd_refs_push(t10);
        BDD t11 = CALL(reachable_rec, s1, r11, nextvar);
        bdd_refs_push(t11);

        BDD t0 = sylvan_or(t00, t10); // states where curvar = 0 after applying r00 / r10
        BDD t1 = sylvan_or(t01, t11); // states where curvar = 1 after applying r01 / r11
        bdd_refs_pop(4);

        /* Union with previously reachable set */
        s0 = sylvan_or(s0, t0);
        s1 = sylvan_or(s1, t1);
    }

    /* res = ((!curvar) ^ s0)  v  ((curvar) ^ s1) */
    BDD res = sylvan_makenode(level, s0, s1);

    /* Put in cache */
    if (cachenow) {
        if (cache_put3(CACHE_BDD_REACHABLE, s, r, 0, res)) 
            sylvan_stats_count(BDD_REACHABLE_CACHEDPUT);
    }

    return res;
}