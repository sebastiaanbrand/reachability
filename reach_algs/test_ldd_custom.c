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

static MDD write_states(MDD states)
{
    FILE *fp;
    fp = fopen("states.dot", "w");
    lddmc_fprintdot(fp, states);
    fclose(fp);
}

MDD stack_transition(int r, int w, MDD next)
{
    MDD write = lddmc_makenode(w, next, lddmc_false);
    MDD read  = lddmc_makenode(r, write, lddmc_false);
    return read;
}

int test_lddmc_image_read_write()
{
    // Rel : {(0 -> 1), (0 -> 2), (0 -> 5)}
    MDD rel = lddmc_false;
    rel = lddmc_union(rel, stack_transition(0, 1, lddmc_true));
    rel = lddmc_union(rel, stack_transition(0, 2, lddmc_true));
    rel = lddmc_union(rel, stack_transition(0, 5, lddmc_true));
    MDD meta = lddmc_make_readwrite_meta(1, false);

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

int test_lddmc_image_copy1()
{
    // Rel : ({forall i : (i -> i)})
    MDD rel = lddmc_true;
    rel = lddmc_make_copynode(rel, lddmc_false);
    rel = lddmc_make_copynode(rel, lddmc_false);
    MDD meta = lddmc_make_readwrite_meta(1, false);

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

int test_lddmc_image_copy2()
{
    // Rel : ({* -> copy } v (5 -> 7))
    MDD r5_w7  = stack_transition(5, 7, lddmc_true);
    MDD w_copy = lddmc_make_copynode(lddmc_true, lddmc_false);
    MDD rel = lddmc_make_copynode(w_copy, r5_w7);
    MDD meta = lddmc_make_readwrite_meta(1, false);

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

    MDD t3 = lddmc_image(s3, rel, meta); // {3}
    MDD t5 = lddmc_image(s5, rel, meta); // {5,7}
    MDD t456 = lddmc_image(s456, rel, meta); // {4,5,6,7}
    test_assert(lddmc_satcount(t3) == 1);
    test_assert(lddmc_satcount(t5) == 2);
    test_assert(lddmc_satcount(t456) == 4);
    test_assert(lddmc_follow(t3, 3) == lddmc_true);
    test_assert(lddmc_follow(t5, 5) == lddmc_true);
    test_assert(lddmc_follow(t5, 7) == lddmc_true);
    test_assert(lddmc_follow(t456, 4) == lddmc_true);
    test_assert(lddmc_follow(t456, 5) == lddmc_true);
    test_assert(lddmc_follow(t456, 6) == lddmc_true);
    test_assert(lddmc_follow(t456, 7) == lddmc_true);

    return 0;
}

int test_lddmc_image_only_write1()
{
    // Rel : ({* -> 3} v (5 -> 7))
    MDD r5_w7 = stack_transition(5, 7, lddmc_true);
    MDD w3 = lddmc_makenode(3, lddmc_true, lddmc_false);
    MDD rel = lddmc_make_copynode(w3, r5_w7);
    MDD meta = lddmc_make_readwrite_meta(1, false);

    // s4 = {4}, s5 = {5}, s456 = {456}
    MDD s4 = lddmc_makenode(4, lddmc_true, lddmc_false);
    MDD s5 = lddmc_makenode(5, lddmc_true, lddmc_false);
    MDD s456 = lddmc_false;
    s456 = lddmc_union(s456, lddmc_makenode(4, lddmc_true, lddmc_false));
    s456 = lddmc_union(s456, lddmc_makenode(5, lddmc_true, lddmc_false));
    s456 = lddmc_union(s456, lddmc_makenode(6, lddmc_true, lddmc_false));
    test_assert(lddmc_satcount(s4) == 1);
    test_assert(lddmc_satcount(s5) == 1);
    test_assert(lddmc_satcount(s456) == 3);

    MDD t4 = lddmc_image(s4, rel, meta); // {3}
    MDD t5 = lddmc_image(s5, rel, meta); // {3, 7}
    MDD t456 = lddmc_image(s456, rel, meta); // {3, 7}
    test_assert(lddmc_satcount(t4) == 1);
    test_assert(lddmc_satcount(t5) == 2);
    test_assert(lddmc_satcount(t456) == 2);
    test_assert(lddmc_follow(t4, 3) == lddmc_true);
    test_assert(lddmc_follow(t5, 3) == lddmc_true);
    test_assert(lddmc_follow(t5, 7) == lddmc_true);
    test_assert(lddmc_follow(t456, 3) == lddmc_true);
    test_assert(lddmc_follow(t456, 7) == lddmc_true);

    return 0;
}

int test_lddmc_image_only_write2()
{
    // Rel : ({<10,*> -> <20,3>} v (<11,5> -> <12,7>))
    //   | 
    // [10]->[11]
    //   |     |
    // [20]  [12]
    //   |     |
    //  [*]   [5]
    //   |     |
    //  [3]   [7]
    MDD r115_w127 = lddmc_true;
    r115_w127 = stack_transition(5, 7, r115_w127);
    r115_w127 = stack_transition(11, 12, r115_w127);
    MDD w3 = lddmc_makenode(3, lddmc_true, lddmc_false);
    w3 = lddmc_make_copynode(w3, lddmc_false);
    MDD rel = lddmc_makenode(20, w3, lddmc_false);
    rel = lddmc_makenode(10, rel, r115_w127);
    MDD meta = lddmc_make_readwrite_meta(2, false);
    test_assert(lddmc_nodecount(rel) == 8);

    // s1 = {<10,4>}, s2 = {<10,5>}, s3 = s1 U s2 U {<10,6>}, s4 = s1 U {<11,5>}
    MDD s1 = lddmc_true;
    s1 = lddmc_makenode(4, s1, lddmc_false);
    s1 = lddmc_makenode(10,s1, lddmc_false);
    MDD s2 = lddmc_true;
    s2 = lddmc_makenode(5, s2, lddmc_false);
    s2 = lddmc_makenode(10,s2, lddmc_false);
    MDD s3 = lddmc_true;
    s3 = lddmc_makenode(6, s3, lddmc_false);
    s3 = lddmc_makenode(10,s3, lddmc_false);
    s3 = lddmc_union(s1, lddmc_union(s2, s3));
    MDD s4 = lddmc_true;
    s4 = lddmc_makenode(5, s4, lddmc_false);
    s4 = lddmc_makenode(11,s4, lddmc_false);
    s4 = lddmc_union(s1, s4);
    test_assert(lddmc_satcount(s1) == 1);
    test_assert(lddmc_satcount(s2) == 1);
    test_assert(lddmc_satcount(s3) == 3);
    test_assert(lddmc_satcount(s4) == 2);

    MDD t1 = lddmc_image(s1, rel, meta); // {<20,3>}
    MDD t2 = lddmc_image(s2, rel, meta); // {<20,3>}
    MDD t3 = lddmc_image(s3, rel, meta); // {<20,3>}
    MDD t4 = lddmc_image(s4, rel, meta); // {<20,3>, <12,7>}

    MDD temp;
    test_assert(t1 == t2);
    test_assert(t1 == t3);
    test_assert(lddmc_satcount(t1) == 1);
    test_assert(lddmc_getvalue(t1) == 20);  temp = lddmc_getdown(t1);
    test_assert(lddmc_getvalue(temp) == 3); temp = lddmc_getdown(temp);
    test_assert(temp == lddmc_true);
    test_assert(lddmc_satcount(t4) == 2);       temp = lddmc_getdown(t4);
    test_assert(lddmc_getvalue(temp) == 7);     temp = lddmc_getdown(temp);
    test_assert(temp == lddmc_true);            temp = lddmc_getright(t4);
    test_assert(lddmc_getvalue(temp) == 20);    temp = lddmc_getdown(temp);
    test_assert(lddmc_getvalue(temp) == 3);     temp = lddmc_getdown(temp);
    test_assert(temp == lddmc_true);

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
    MDD meta = lddmc_make_readwrite_meta(2, false);

    test_assert(lddmc_satcount(t1) == 1);
    test_assert(lddmc_satcount(t2) == 1);
    test_assert(lddmc_satcount(rel) == 2);

    // relation is already ['read', 'write'] for all variables,
    // so nothing should change
    MDD rel_ext = lddmc_extend_rel(rel, meta, 4);
    test_assert(rel_ext == rel);

    return 0;
}

int test_lddmc_extend_rel2()
{
    // Meta = [0, 1, 2], Rel = {forall i: (<i,4> -> <i,5>)}
    MDD rel = stack_transition(4, 5, lddmc_true);
    MDD meta = lddmc_make_readwrite_meta(1, false);
    meta = lddmc_makenode(0, meta, lddmc_false);
    test_assert(lddmc_nodecount(rel) == 2);

    // the skipped var (meta = 0) should be replaced with 'read','write'
    MDD rel_ext = lddmc_extend_rel(rel, meta, 4);
    test_assert(lddmc_nodecount(rel_ext) == 4);
    MDD temp = rel_ext;
    test_assert(lddmc_iscopy(temp));        temp = lddmc_getdown(temp);
    test_assert(lddmc_iscopy(temp));        temp = lddmc_getdown(temp);
    test_assert(lddmc_getvalue(temp) == 4); temp = lddmc_getdown(temp);
    test_assert(lddmc_getvalue(temp) == 5); temp = lddmc_getdown(temp);
    test_assert(temp == lddmc_true);

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
    MDD rel_ext = lddmc_extend_rel(rel, meta, 4);
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
    MDD rel_ext = lddmc_extend_rel(rel, meta, 6);
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
    MDD rel_ext = lddmc_extend_rel(rel, meta, 6);
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
    MDD rel_ext = lddmc_extend_rel(rel, meta, 4);
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
    MDD rel_ext = lddmc_extend_rel(rel, meta, 4);
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
    MDD rel2_ext = lddmc_extend_rel(rel2, meta2, 4);
    test_assert(lddmc_nodecount(rel2_ext) == 4);
    test_assert(lddmc_getvalue(rel2_ext) == 15); rel2_ext = lddmc_getdown(rel2_ext);
    test_assert(lddmc_getvalue(rel2_ext) == 25); rel2_ext = lddmc_getdown(rel2_ext);
    test_assert(lddmc_iscopy(rel2_ext));         rel2_ext = lddmc_getdown(rel2_ext);
    test_assert(lddmc_getvalue(rel2_ext) == 7);  rel2_ext = lddmc_getdown(rel2_ext);

    return 0;
}

int test_image_vs_relprod1()
{
    // 1. Create a rel + set of states
    // Meta = [1, 2, 1, 2], Rel = {<6,4> -> <7, 3>),(<6,6> -> <10,20>)}
    MDD rel1 = lddmc_true;
    rel1 = stack_transition(4, 3, rel1);
    rel1 = stack_transition(6, 7, rel1);
    MDD rel2 = lddmc_true;
    rel2 = stack_transition(6, 20, rel2);
    rel2 = stack_transition(6, 10, rel2);
    MDD rel = lddmc_union(rel1, rel2);
    MDD meta = lddmc_make_readwrite_meta(2, false);
    
    // s0 = {<6,4>}, s1 = {<6,4>,<6,6>}, s2 = {<6,5>}
    MDD s0 = lddmc_true;
    s0 = lddmc_makenode(4, s0, lddmc_false);
    s0 = lddmc_makenode(6, s0, lddmc_false);
    MDD s1 = lddmc_true;
    s1 = lddmc_makenode(6, s1, lddmc_false);
    s1 = lddmc_makenode(6, s1, lddmc_false);
    s1 = lddmc_union(s0, s1);
    MDD s2 = lddmc_true;
    s2 = lddmc_makenode(5, s2, lddmc_false);
    s2 = lddmc_makenode(6, s2, lddmc_false);
    test_assert(lddmc_satcount(s0) == 1);
    test_assert(lddmc_satcount(s1) == 2);
    test_assert(lddmc_satcount(s2) == 1);

    // 2. Test R.S with relprod
    MDD r0 = lddmc_relprod(s0, rel, meta); // {<7,3>}
    MDD r1 = lddmc_relprod(s1, rel, meta); // {<7,3>, <10,20>}
    MDD r2 = lddmc_relprod(s2, rel, meta); // {}
    test_assert(lddmc_satcount(r0) == 1);
    test_assert(lddmc_satcount(r1) == 2);
    test_assert(lddmc_satcount(r2) == 0);
    MDD temp = r0;
    test_assert(lddmc_getvalue(temp) == 7);     temp = lddmc_getdown(temp);
    test_assert(lddmc_getvalue(temp) == 3);     temp = lddmc_getdown(temp);
    test_assert(temp == lddmc_true);
    temp = r1;
    test_assert(lddmc_getvalue(temp) == 7);     temp = lddmc_getdown(temp);
    test_assert(lddmc_getvalue(temp) == 3);     temp = lddmc_getdown(temp);
    test_assert(temp == lddmc_true);
    temp = lddmc_getright(r1);
    test_assert(lddmc_getvalue(temp) == 10);    temp = lddmc_getdown(temp);
    test_assert(lddmc_getvalue(temp) == 20);    temp = lddmc_getdown(temp);
    test_assert(temp == lddmc_true);

    // 3. Extend R, test R.S with image
    MDD rel_ext  = lddmc_extend_rel(rel, meta, 4);
    MDD meta_ext = lddmc_make_readwrite_meta(2, false); 
    MDD t0 = lddmc_image(s0, rel_ext, meta_ext);
    MDD t1 = lddmc_image(s1, rel_ext, meta_ext);
    MDD t2 = lddmc_image(s2, rel_ext, meta_ext);
    test_assert(t0 == r0);
    test_assert(t1 == r1);
    test_assert(t2 == r2);

    return 0;
}

int test_image_vs_relprod2()
{
    // 1. Create a rel + set of states
    // Meta = [0, 1, 2], Rel = {forall i: (<i,4> -> <i,5>)}
    MDD rel = stack_transition(4, 5, lddmc_true);
    MDD meta = lddmc_make_readwrite_meta(1, false);
    meta = lddmc_makenode(0, meta, lddmc_false);
    test_assert(lddmc_nodecount(rel) == 2);

    // s0 = {<10,4>}, s1 = {<10,4>,<11,4>}, s2 = {<10,6>}
    MDD s0 = lddmc_true;
    s0 = lddmc_makenode(4, s0, lddmc_false);
    s0 = lddmc_makenode(10,s0, lddmc_false);
    MDD s1 = lddmc_true;
    s1 = lddmc_makenode(4, s1, lddmc_false);
    s1 = lddmc_makenode(11,s1, lddmc_false);
    s1 = lddmc_union(s0, s1);
    MDD s2 = lddmc_true;
    s2 = lddmc_makenode(6, s2, lddmc_false);
    s2 = lddmc_makenode(10,s2, lddmc_false);
    test_assert(lddmc_satcount(s0) == 1);
    test_assert(lddmc_satcount(s1) == 2);
    test_assert(lddmc_satcount(s2) == 1);

    // 2. Test R.S with relprod
    MDD r0 = lddmc_relprod(s0, rel, meta); // {<10,5>}
    MDD r1 = lddmc_relprod(s1, rel, meta); // {<10,5>, <11,5>}
    MDD r2 = lddmc_relprod(s2, rel, meta); // {}
    test_assert(lddmc_satcount(r0) == 1);
    test_assert(lddmc_satcount(r1) == 2);
    test_assert(lddmc_satcount(r2) == 0);
    MDD temp = r0;
    test_assert(lddmc_getvalue(temp) == 10);    temp = lddmc_getdown(temp);
    test_assert(lddmc_getvalue(temp) == 5);     temp = lddmc_getdown(temp);
    test_assert(temp == lddmc_true);
    temp = r1;
    test_assert(lddmc_getvalue(temp) == 10);    temp = lddmc_getdown(temp);
    test_assert(lddmc_getvalue(temp) == 5);     temp = lddmc_getdown(temp);
    test_assert(temp == lddmc_true);
    temp = lddmc_getright(r1);
    test_assert(lddmc_getvalue(temp) == 11);    temp = lddmc_getdown(temp);
    test_assert(lddmc_getvalue(temp) == 5);     temp = lddmc_getdown(temp);
    test_assert(temp == lddmc_true);

    // 3. Extend R, test R.S with image
    MDD rel_ext  = lddmc_extend_rel(rel, meta, 4);
    MDD meta_ext = lddmc_make_readwrite_meta(2, false);
    MDD t0 = lddmc_image(s0, rel_ext, meta_ext);
    MDD t1 = lddmc_image(s1, rel_ext, meta_ext);
    MDD t2 = lddmc_image(s2, rel_ext, meta_ext);
    test_assert(t0 == r0);
    test_assert(t1 == r1);
    test_assert(t2 == r2);

    return 0;
}

int test_image_vs_relprod0()
{
    // 1. Create a rel + set of states

    // 2. Test R.S with relprod

    // 3. Extend R, test R.S with image

    return 0;
}

int runtests()
{
    // we are not testing garbage collection
    sylvan_gc_disable();

    printf("Testing custom lddmc_image...\n");
    printf("    * read-write...                             "); fflush(stdout);
    if (test_lddmc_image_read_write()) return 1;
    printf("OK\n");
    printf("    * copy...                                   "); fflush(stdout);
    if (test_lddmc_image_copy1()) return 1;
    if (test_lddmc_image_copy2()) return 1;
    printf("OK\n");
    printf("    * only-write...                             "); fflush(stdout);
    if (test_lddmc_image_only_write1()) return 1;
    if (test_lddmc_image_only_write2()) return 1;
    printf("OK\n");

    printf("Testing extend LDD relations to full domain...  "); fflush(stdout);
    if (test_lddmc_extend_rel1()) return 1;
    if (test_lddmc_extend_rel2()) return 1;
    if (test_lddmc_extend_rel3()) return 1;
    if (test_lddmc_extend_rel4()) return 1;
    if (test_lddmc_extend_rel5()) return 1;
    if (test_lddmc_extend_rel6()) return 1;
    if (test_lddmc_extend_rel7()) return 1;
    printf("OK\n");

    printf("Testing lddmc_image against lddmc_relprod...    "); fflush(stdout);
    if (test_image_vs_relprod1()) return 1;
    if (test_image_vs_relprod2()) return 1;
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