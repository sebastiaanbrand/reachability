#include "recursive_reachability.h"
#include "sylvan_int.h"

TASK_IMPL_3(BDD, reachable_bfs, BDD, s, BDD, r, BDDSET, vars)
{
    BDD prev = sylvan_false;
    BDD successors = sylvan_false;
    int k = 0;
    uint64_t nodecount;

    sylvan_protect(&s);
    sylvan_protect(&r);
    sylvan_protect(&vars);
    sylvan_protect(&prev);
    sylvan_protect(&successors);

    printf("it %d, nodecount = %ld ", k, sylvan_nodecount(s)); fflush(stdout);
    printf("(%'0.0f states)\n", sylvan_satcount(s, vars));
    while (prev != s) {
        prev = s;
        successors = sylvan_relnext(s, r, vars);

        s = sylvan_or(s, successors);
        nodecount = sylvan_nodecount(s);
        printf("it %d, nodecount = %ld ", ++k, nodecount); fflush(stdout);
        printf("(%'0.0f states)\n", sylvan_satcount(s, vars));
    }

    sylvan_unprotect(&s);
    sylvan_unprotect(&r);
    sylvan_unprotect(&vars);
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
    if (var > topvar) { 
        r0 = r;
        r1 = r;
    }
    else {
        assert(topvar == var);
        r0 = node_low(r, n);
        r1 = node_high(r, n);
    }

    // Determine top var of r0, r1
    bddnode_t n0 = sylvan_isconst(r0) ? 0 : MTBDD_GETNODE(r0);
    bddnode_t n1 = sylvan_isconst(r1) ? 0 : MTBDD_GETNODE(r1);
    BDDVAR var0 = n0 ? bddnode_getvariable(n0) : 0xffffffff;
    BDDVAR var1 = n1 ? bddnode_getvariable(n1) : 0xffffffff;

    // Check if primed var is skipped on r0
    if (var0 > topvar + 1) {
        *r00 = r0;
        *r01 = r0;
    }
    else {
        assert(var0 == topvar + 1);
        *r00 = node_low(r0, n0);
        *r01 = node_high(r0, n0);
    }

    // Check if primed var is skipped on r1
    if (var1 > topvar + 1) {
        *r10 = r1;
        *r11 = r1;
    }
    else {
        assert(var1 == topvar + 1);
        *r10 = node_low(r1, n1);
        *r11 = node_high(r1, n1);
    }
}

TASK_IMPL_5(BDD, reachable_rec, BDD, s, BDD, r, BDDSET, vars, BDDVAR, nvars, BDDVAR, curvar)
{
    // TODO: Terminal case
    return reachable_bfs(s, r, vars);

    // TODO: cache get

    // Relations for recursion
    BDD r00, r01, r10, r11;
    partition_rel(r, curvar, &r00, &r01, &r10, &r11);

    // TODO: State for recursion
    BDD s0 = s;
    BDD s1 = s;

    // Next var
    BDDVAR nextvar = curvar + 1;

    // TODO: Next vars (remove topvar from vars)
    // (maybe we don't need vars)

    // Recursion (TODO: put in loop)
    // TODO: get return values
    RUN(reachable_rec, s0, r00, vars, nvars, nextvar);
    RUN(reachable_rec, s0, r01, vars, nvars, nextvar);
    RUN(reachable_rec, s1, r10, vars, nvars, nextvar);
    RUN(reachable_rec, s1, r11, vars, nvars, nextvar);

    // TODO: put relevant topvar = 0 / topvar = 1 back into resulting s

    // TODO: cache put

    return reachable_bfs(s, r, vars);
}