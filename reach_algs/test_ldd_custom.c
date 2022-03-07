#include "sylvan.h"
#include "test_assert.h"
#include "sylvan_int.h"
#include "ldd_custom.h"

MDD stack_transition(int r, int w, MDD next)
{
    MDD write = lddmc_makenode(w, next, lddmc_false);
    MDD read  = lddmc_makenode(r, write, lddmc_false);
    return read;
}

MDD make_read_write_meta(int num_vars)
{
    MDD meta = lddmc_true;
    for (int i = 0; i < num_vars; i++) {
        meta = lddmc_makenode(2, meta, lddmc_false);
        meta = lddmc_makenode(1, meta, lddmc_false);
    }
    return meta;
}

int test_lddmc_image()
{
    // Rel : {(0 -> 1), (0 -> 2), (0 -> 5)}
    MDD rel = lddmc_false;
    rel = lddmc_union(rel, stack_transition(0, 1, lddmc_true));
    rel = lddmc_union(rel, stack_transition(0, 2, lddmc_true));
    rel = lddmc_union(rel, stack_transition(0, 5, lddmc_true));
    MDD meta = make_read_write_meta(1);

    // s0 = {0}, s5 = {5}
    MDD s0 = lddmc_makenode(0, lddmc_true, lddmc_false);
    MDD s5 = lddmc_makenode(5, lddmc_true, lddmc_false);
    test_assert(lddmc_satcount(s0) == 1);
    test_assert(lddmc_satcount(s5) == 1);

    MDD t0 = lddmc_image(s0, rel, meta); // should have 3 states
    MDD t5 = lddmc_image(s5, rel, meta); // should have 0 states
    test_assert(lddmc_satcount(t0) == 3);
    test_assert(lddmc_satcount(t5) == 0);

    return 0;
}

int test_lddmc_image_copy_nodes1()
{
    // Rel : ({forall i : (i -> i)})
    MDD rel = lddmc_true;
    rel = lddmc_make_copynode(rel, lddmc_false);
    rel = lddmc_make_copynode(rel, lddmc_false);
    MDD meta = make_read_write_meta(1);

    // s0 = {0}, s134 = {1,3,4}
    MDD s0 = lddmc_makenode(0, lddmc_true, lddmc_false);
    MDD s134 = lddmc_false;
    s134 = lddmc_union(s134, lddmc_makenode(1, lddmc_true, lddmc_false));
    s134 = lddmc_union(s134, lddmc_makenode(3, lddmc_true, lddmc_false));
    s134 = lddmc_union(s134, lddmc_makenode(4, lddmc_true, lddmc_false));
    test_assert(lddmc_satcount(s0) == 1);
    test_assert(lddmc_satcount(s134) == 3);

    MDD t0 = lddmc_image(s0, rel, meta);
    MDD t134 = lddmc_image(s134, rel, meta);
    test_assert(lddmc_satcount(t0) == 1);
    test_assert(lddmc_satcount(t134) == 3);
    test_assert(t0 == s0);
    test_assert(t134 == s134);

    return 0;
}

int test_lddmc_image_copy_nodes2() {
    // Rel : ({* -> copy } v (5 -> 7))
    MDD r5_w7  = stack_transition(5, 7, lddmc_true);
    MDD w_copy = lddmc_make_copynode(lddmc_true, lddmc_false);
    MDD rel = lddmc_make_copynode(w_copy, r5_w7);
    MDD meta = make_read_write_meta(1);

    // s3 = {3}, s5 = {5}, s456 = {4,5,6}
    MDD s3 = lddmc_makenode(3, lddmc_true, lddmc_false);
    MDD s5 = lddmc_makenode(5, lddmc_true, lddmc_false);
    MDD s456 = lddmc_false;
    s456 = lddmc_union(s456, lddmc_makenode(4, lddmc_true, lddmc_false));
    s456 = lddmc_union(s456, lddmc_makenode(5, lddmc_true, lddmc_false));
    s456 = lddmc_union(s456, lddmc_makenode(6, lddmc_true, lddmc_false));
    test_assert(lddmc_satcount(s3) == 1);
    test_assert(lddmc_satcount(s5) == 1);
    test_assert(lddmc_satcount(s456) == 3);

    MDD t3 = lddmc_image(s3, rel, meta);
    MDD t5 = lddmc_image(s5, rel, meta);
    MDD t456 = lddmc_image(s456, rel, meta);
    test_assert(lddmc_satcount(t3) == 1); // {3}
    test_assert(lddmc_satcount(t5) == 2); // {5,7}
    test_assert(lddmc_satcount(t456) == 4); // {4,5,6,7}
}

int runtests()
{
    // we are not testing garbage collection
    sylvan_gc_disable();

    printf("Testing custom lddmc_image w/o copy nodes...    ");
    fflush(stdout);
    if (test_lddmc_image()) return 1;
    printf("OK\n");

    printf("Testing custom lddmc_image with copy nodes...   ");
    fflush(stdout);
    if (test_lddmc_image_copy_nodes1()) return 1;
    if (test_lddmc_image_copy_nodes2()) return 1;
    printf("OK\n");

    return 0;
}

int main()
{
    // Standard Lace initialization with 1 worker
    lace_start(1, 0);

    // Simple Sylvan initialization, also initialize BDD, MTBDD and LDD support
    sylvan_set_sizes(1LL<<20, 1LL<<20, 1LL<<16, 1LL<<16);
    sylvan_init_package();
    sylvan_init_bdd();
    sylvan_init_mtbdd();
    sylvan_init_ldd();

    printf("Sylvan initialization complete.\n");

    int res = runtests();

    sylvan_quit();
    lace_stop();

    return res;
}