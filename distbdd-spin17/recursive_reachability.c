#include "recursive_reachability.h"
#include "sylvan_int.h"

TASK_IMPL_2(BDD, reachable_bfs, BDD, s, BDD, r)
{
    printf("reachable bfs\n");
    BDD prev = sylvan_false;
    BDD successors = sylvan_false;
    int k = 0;
    uint64_t nodecount;

    sylvan_protect(&s);
    sylvan_protect(&r);
    sylvan_protect(&prev);
    sylvan_protect(&successors);

    printf("it %d, nodecount = %ld\n", k, sylvan_nodecount(s));
    while (prev != s) {
        prev = s;
        successors = sylvan_relnext(s, r, sylvan_false);

        s = sylvan_or(s, successors);
        nodecount = sylvan_nodecount(s);
        printf("it %d, nodecount = %ld\n", ++k, nodecount);
    }

    sylvan_unprotect(&s);
    sylvan_unprotect(&r);
    sylvan_unprotect(&prev);
    sylvan_unprotect(&successors);

    return s;
}

void
partition_rel(BDD r, BDDVAR topvar, BDD *r00, BDD *r01, BDD *r10, BDD *r11)
{
    // Determine top var of r
    bddnode_t n = sylvan_isconst(r) ? 0 : MTBDD_GETNODE(r);
    BDDVAR var = n ? bddnode_getvariable(n) : 0xffffffff;
    assert(topvar <= var);

    // Check if unprimed var is skipped
    BDD r0, r1;
    if (var == topvar) {
        r0 = node_low(r, n);
        r1 = node_high(r, n);
    } else {
        assert(var > topvar);
        r0 = r1 = r;
    }

    // Check if primed var is skipped
    if (!sylvan_isconst(r0)) {
        bddnode_t n0 = MTBDD_GETNODE(r0);
        if (bddnode_getvariable(n0) == topvar + 1) {
            *r00 = node_low(r0, n0);
            *r01 = node_high(r0, n0);
        } else {
            assert(var > topvar);
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
            assert(var > topvar);
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

TASK_IMPL_4(BDD, reachable_rec, BDD, s, BDD, r, BDDVAR, nvars, BDDVAR, curvar)
{
    assert(curvar % 2 == 0);

    /* TODO: better terminals */
    if (curvar > 0) {//((nvars-1)*2 == curvar ) { // at last variable
        printf("curvar = %d\n", curvar);
        return reachable_bfs(s, r);
    }

    printf("curvar = %d\n", curvar);

    /* Consult cache */
    int cachenow = 0;
    if (cachenow) {
        BDD res;
        if (cache_get3(CACHE_BDD_REACHABLE, s, r, 0, &res)) {
            return res;
        }
    }

    // Relations, states, and var for next level of recursion
    BDDVAR nextvar = curvar + 2;
    BDD r00, r01, r10, r11, s0, s1;
    partition_rel(r, curvar, &r00, &r01, &r10, &r11);
    partition_state(s, curvar, &s0, &s1);

    // TODO: protect relevant BDDs
    BDD prev0 = sylvan_false;
    BDD prev1 = sylvan_false;

    // TODO: maybe we can do this saturation-like loop more efficiently
    while (s0 != prev0 || s1 != prev1) {
        printf("loop\n");
        prev0 = s0;
        prev1 = s1;

        // TODO: spawn tasks
        BDD t00 = CALL(reachable_rec, s0, r00, nvars, nextvar);
        bdd_refs_push(t00);
        BDD t01 = CALL(reachable_rec, s0, r01, nvars, nextvar);
        bdd_refs_push(t01);
        BDD t10 = CALL(reachable_rec, s1, r10, nvars, nextvar);
        bdd_refs_push(t10);
        BDD t11 = CALL(reachable_rec, s1, r11, nvars, nextvar);
        bdd_refs_push(t11);

        // stitch partitioned states back together
        // ITE(a, true, b) === OR(a, b)
        // NOTE: I would think t01 and t10 would need to be flipped, but
        // flipping them gives incorrect results...
        BDD t0 = sylvan_or(t00, t01); // states where curvar = 0 after applying r00 / r10
        BDD t1 = sylvan_or(t10, t11); // states where curvar = 1 after applying r01 / r11

        s0 = sylvan_or(s0, t0);
        s1 = sylvan_or(s1, t1);
    }

    // s = ((!curvar) ^ s0)  v  ((curvar) ^ s1)
    BDD res = sylvan_makenode(curvar, s0, s1);

    /* Put in cache */
    if (cachenow) {
        cache_put3(CACHE_BDD_REACHABLE, s, r, 0, res);
    }

    return res;
}