#include "sylvan.h"
#include "test_assert.h"
#include "sylvan_int.h"
#include "ldd_custom.h"

static MDD write_rel(MDD rel)
{
    FILE *fp;
    fp = fopen("rel.dot", "w");
    lddmc_fprintdot(fp, rel);
    fclose(fp);
}

static MDD write_meta(MDD meta)
{
    FILE *fp;
    fp = fopen("meta.dot", "w");
    lddmc_fprintdot(fp, meta);
    fclose(fp);
}

MDD stack_transition(int r, int w, MDD next)
{
    MDD write = lddmc_makenode(w, next, lddmc_false);
    MDD read  = lddmc_makenode(r, write, lddmc_false);
    return read;
}

int test_lddmc_image()
{
    // Rel : {(0 -> 1), (0 -> 2), (0 -> 5)}
    MDD rel = lddmc_false;
    rel = lddmc_union(rel, stack_transition(0, 1, lddmc_true));
    rel = lddmc_union(rel, stack_transition(0, 2, lddmc_true));
    rel = lddmc_union(rel, stack_transition(0, 5, lddmc_true));
    MDD meta = lddmc_make_readwrite_meta(1);

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
    MDD meta = lddmc_make_readwrite_meta(1);

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

int test_lddmc_image_copy_nodes2()
{
    // Rel : ({* -> copy } v (5 -> 7))
    MDD r5_w7  = stack_transition(5, 7, lddmc_true);
    MDD w_copy = lddmc_make_copynode(lddmc_true, lddmc_false);
    MDD rel = lddmc_make_copynode(w_copy, r5_w7);
    MDD meta = lddmc_make_readwrite_meta(1);

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

    return 0;
}

int test_lddmc_extend_rel1()
{
    // Meta = [1, 2, 1, 2], Rel = {<6,4> -> <7, 3>),(<6,6> -> <10,20>)}
    MDD t1 = lddmc_true;
    t1 = stack_transition(4, 3, t1);
    t1 = stack_transition(6, 7, t1);
    MDD t2 = lddmc_true;
    t2 = stack_transition(6, 20, t2);
    t2 = stack_transition(6, 10, t2);
    MDD rel = lddmc_union(t1, t2);
    MDD meta = lddmc_make_readwrite_meta(2);

    test_assert(lddmc_satcount(t1) == 1);
    test_assert(lddmc_satcount(t2) == 1);
    test_assert(lddmc_satcount(rel) == 2);

    // relation is already ['read', 'write'] for all variables,
    // so nothing should change
    MDD rel_ext = lddmc_extend_rel(rel, meta, 2);
    test_assert(rel_ext == rel);

    return 0;
}

int test_lddmc_extend_rel2()
{
    // Meta = [0, 1, 2], Rel = {forall i: (<i,4> -> <i,5>)}
    MDD rel = stack_transition(4, 5, lddmc_true);
    MDD meta = lddmc_make_readwrite_meta(1);
    meta = lddmc_makenode(0, meta, lddmc_false);
    test_assert(lddmc_nodecount(rel) == 2);

    // the skipped var (meta = 0) should be replaced with 'read','write'
    MDD rel_ext = lddmc_extend_rel(rel, meta, 2);
    test_assert(lddmc_nodecount(rel_ext) == 4);
    test_assert(lddmc_iscopy(rel_ext));
    test_assert(lddmc_iscopy(lddmc_getdown(rel_ext)));

    return 0;
}

int test_lddmc_extend_rel3()
{
    // Meta = [1, 2, 0], Rel = {forall i: (<4,i> -> <5,i>)} (nvar = 2)
    MDD rel = stack_transition(4, 5, lddmc_true);
    MDD meta = lddmc_true;
    meta = lddmc_makenode(0, meta, lddmc_false);
    meta = lddmc_makenode(2, meta, lddmc_false);
    meta = lddmc_makenode(1, meta, lddmc_false);
    test_assert(lddmc_nodecount(rel) == 2);

    // for the skipped var, a r-w with copy nodes should be added
    MDD rel_ext = lddmc_extend_rel(rel, meta, 2);
    test_assert(lddmc_nodecount(rel_ext) == 4);
    test_assert(lddmc_getvalue(rel_ext) == 4); rel_ext = lddmc_getdown(rel_ext);
    test_assert(lddmc_getvalue(rel_ext) == 5); rel_ext = lddmc_getdown(rel_ext);
    test_assert(lddmc_iscopy(rel_ext));        rel_ext = lddmc_getdown(rel_ext);
    test_assert(lddmc_iscopy(rel_ext));        rel_ext = lddmc_getdown(rel_ext);
    test_assert(rel_ext == lddmc_true);

    return 0;   
}

int test_lddmc_extend_rel4()
{
    // Meta = [1, 2, -1], same as previous test but now with nvar = 3
    MDD rel = stack_transition(4, 5, lddmc_true);
    MDD meta = lddmc_true;
    meta = lddmc_makenode(-1, meta, lddmc_false);
    meta = lddmc_makenode(2, meta, lddmc_false);
    meta = lddmc_makenode(1, meta, lddmc_false);
    test_assert(lddmc_nodecount(rel) == 2);

    // for nvar=3, we expect 'read','write' levels to be added twice at the end
    MDD rel_ext = lddmc_extend_rel(rel, meta, 3);
    test_assert(lddmc_nodecount(rel_ext) == 6);
    test_assert(lddmc_getvalue(rel_ext) == 4); rel_ext = lddmc_getdown(rel_ext);
    test_assert(lddmc_getvalue(rel_ext) == 5); rel_ext = lddmc_getdown(rel_ext);
    test_assert(lddmc_iscopy(rel_ext));        rel_ext = lddmc_getdown(rel_ext);
    test_assert(lddmc_iscopy(rel_ext));        rel_ext = lddmc_getdown(rel_ext);
    test_assert(lddmc_iscopy(rel_ext));        rel_ext = lddmc_getdown(rel_ext);
    test_assert(lddmc_iscopy(rel_ext));        rel_ext = lddmc_getdown(rel_ext);
    test_assert(rel_ext == lddmc_true);

    return 0;
}

int test_lddmc_extend_rel5()
{
    // Meta = [1, 2, 5, -1], same as previous but with action label
    MDD rel = lddmc_makenode(123, lddmc_true, lddmc_false);
    rel = stack_transition(4, 5, rel);
    MDD meta = lddmc_true;
    meta = lddmc_makenode(-1, meta, lddmc_false);
    meta = lddmc_makenode(5, meta, lddmc_false);
    meta = lddmc_makenode(2, meta, lddmc_false);
    meta = lddmc_makenode(1, meta, lddmc_false);
    test_assert(lddmc_nodecount(rel) == 3);

    // we expect two added 'read-write' levels, and the action label removed
    MDD rel_ext = lddmc_extend_rel(rel, meta, 3);
    test_assert(lddmc_nodecount(rel_ext) == 6);
    test_assert(lddmc_getvalue(rel_ext) == 4); rel_ext = lddmc_getdown(rel_ext);
    test_assert(lddmc_getvalue(rel_ext) == 5); rel_ext = lddmc_getdown(rel_ext);
    test_assert(lddmc_iscopy(rel_ext));        rel_ext = lddmc_getdown(rel_ext);
    test_assert(lddmc_iscopy(rel_ext));        rel_ext = lddmc_getdown(rel_ext);
    test_assert(lddmc_iscopy(rel_ext));        rel_ext = lddmc_getdown(rel_ext);
    test_assert(lddmc_iscopy(rel_ext));        rel_ext = lddmc_getdown(rel_ext);
    test_assert(rel_ext == lddmc_true);

    return 0;
}

int test_lddmc_extend_rel6()
{
    // Test extending only-read (3)
    // Meta = [3, 1, 2], rel = {(<5,10> -> <5,20>)}
    MDD rel = stack_transition(10, 20, lddmc_true);
    rel = lddmc_makenode(5, rel, lddmc_false);
    MDD meta = lddmc_true;
    meta = lddmc_makenode(2, meta, lddmc_false);
    meta = lddmc_makenode(1, meta, lddmc_false);
    meta = lddmc_makenode(3, meta, lddmc_false);
    test_assert(lddmc_nodecount(rel) == 3);

    // we expect the first read-only to be replaced by read-write
    MDD rel_ext = lddmc_extend_rel(rel, meta, 2);
    test_assert(lddmc_nodecount(rel_ext) == 4);
    test_assert(lddmc_getvalue(rel_ext) == 5);  rel_ext = lddmc_getdown(rel_ext);
    test_assert(lddmc_getvalue(rel_ext) == 5);  rel_ext = lddmc_getdown(rel_ext);
    test_assert(lddmc_getvalue(rel_ext) == 10); rel_ext = lddmc_getdown(rel_ext);
    test_assert(lddmc_getvalue(rel_ext) == 20); rel_ext = lddmc_getdown(rel_ext);
    test_assert(rel_ext == lddmc_true);

    return 0;
}

int test_lddmc_extend_rel7()
{
    // Test extending only-write (4)
    // Meta = [4, 1, 2], rel = {(<*,10> -> <6, 20>)}
    MDD rel = stack_transition(10, 20, lddmc_true);
    rel = lddmc_makenode(6, rel, lddmc_false);
    MDD meta = lddmc_true;
    meta = lddmc_makenode(2, meta, lddmc_false);
    meta = lddmc_makenode(1, meta, lddmc_false);
    meta = lddmc_makenode(4, meta, lddmc_false);
    test_assert(lddmc_nodecount(rel) == 3);

    // we expect the first write-only to be replaced by 'read anything'-write
    MDD rel_ext = lddmc_extend_rel(rel, meta, 2);
    test_assert(lddmc_nodecount(rel_ext) == 4);
    test_assert(lddmc_iscopy(rel_ext));         rel_ext = lddmc_getdown(rel_ext);
    test_assert(lddmc_getvalue(rel_ext) == 6);  rel_ext = lddmc_getdown(rel_ext);
    test_assert(lddmc_getvalue(rel_ext) == 10); rel_ext = lddmc_getdown(rel_ext);
    test_assert(lddmc_getvalue(rel_ext) == 20); rel_ext = lddmc_getdown(rel_ext);
    test_assert(rel_ext == lddmc_true);

    // Meta = [1, 2, 4], rel = {(<15,*> -> <25, 7>)}
    MDD rel2 = lddmc_makenode(7, lddmc_true, lddmc_false);
    rel2 = stack_transition(15, 25, rel2);
    MDD meta2 = lddmc_true;
    meta2 = lddmc_makenode(4, meta2, lddmc_false);
    meta2 = lddmc_makenode(2, meta2, lddmc_false);
    meta2 = lddmc_makenode(1, meta2, lddmc_false);
    test_assert(lddmc_nodecount(rel2) == 3);

    // last write-only should be replaced by 'read anything'-write
    MDD rel2_ext = lddmc_extend_rel(rel2, meta2, 2);
    test_assert(lddmc_nodecount(rel2_ext) == 4);
    test_assert(lddmc_getvalue(rel2_ext) == 15); rel2_ext = lddmc_getdown(rel2_ext);
    test_assert(lddmc_getvalue(rel2_ext) == 25); rel2_ext = lddmc_getdown(rel2_ext);
    test_assert(lddmc_iscopy(rel2_ext));         rel2_ext = lddmc_getdown(rel2_ext);
    test_assert(lddmc_getvalue(rel2_ext) == 7);  rel2_ext = lddmc_getdown(rel2_ext);

    return 0;
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

    printf("Testing extend LDD relations to full domain...  ");
    fflush(stdout);
    if (test_lddmc_extend_rel1()) return 1;
    if (test_lddmc_extend_rel2()) return 1;
    if (test_lddmc_extend_rel3()) return 1;
    if (test_lddmc_extend_rel4()) return 1;
    if (test_lddmc_extend_rel5()) return 1;
    if (test_lddmc_extend_rel6()) return 1;
    if (test_lddmc_extend_rel7()) return 1;
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