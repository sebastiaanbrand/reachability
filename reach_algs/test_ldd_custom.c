#include "sylvan.h"
#include "test_assert.h"
#include "sylvan_int.h"
#include "ldd_custom.h"

static MDD write_mdd(MDD rel, char *name, int i)
{
    FILE *fp;
    char *fname = (char*)malloc(200 * sizeof(char));
    sprintf(fname, "%s_%d.dot", name, i);
    fp = fopen(fname, "w");
    lddmc_fprintdot(fp, rel);
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

int test_lddmc_extend_rel8()
{
    // Meta = [1, 2, 1, 2], Rel = {<6,4> -> <7, 3>),(<*,20> -> <10,10>)}
    // (including a copy node for a read (1))
    MDD t1 = lddmc_true;
    t1 = stack_transition(4, 3, t1);
    t1 = stack_transition(6, 7, t1);
    MDD t2 = lddmc_true;
    t2 = stack_transition(20, 10, t2);
    t2 = lddmc_makenode(10, t2, lddmc_false);
    t2 = lddmc_make_copynode(t2, lddmc_false);
    MDD rel = lddmc_union(t1, t2);
    MDD meta = lddmc_make_readwrite_meta(2, false);

    // we should handle these copy nodes at a "1" level the same as sylvan
    // already does, so extended rel should be the same
    MDD rel_ext = lddmc_extend_rel(rel, meta, 4);
    test_assert(rel_ext == rel);

    return 0;
}

int test_lddmc_extend_rel9()
{
    // Test extending only-read (3)
    // Meta = [3, 1, 2], rel = {(<*,10> -> <*,20>)}
    // with a copy node ("read anything") for only-read (3)
    MDD rel = stack_transition(10, 20, lddmc_true);
    rel = lddmc_make_copynode(rel, lddmc_false);
    MDD meta = lddmc_true;
    meta = lddmc_makenode(2, meta, lddmc_false);
    meta = lddmc_makenode(1, meta, lddmc_false);
    meta = lddmc_makenode(3, meta, lddmc_false);
    test_assert(lddmc_nodecount(rel) == 3);

    // we expect the first read-only/anything to be repaced by two copy nodes
    MDD rel_ext = lddmc_extend_rel(rel, meta, 4);
    test_assert(lddmc_nodecount(rel_ext) == 4);
    test_assert(lddmc_iscopy(rel_ext));         rel_ext = lddmc_getdown(rel_ext);
    test_assert(lddmc_iscopy(rel_ext));         rel_ext = lddmc_getdown(rel_ext);
    test_assert(lddmc_getvalue(rel_ext) == 10); rel_ext = lddmc_getdown(rel_ext);
    test_assert(lddmc_getvalue(rel_ext) == 20); rel_ext = lddmc_getdown(rel_ext);
    test_assert(rel_ext == lddmc_true);

    return 0;
}

int test_lddmc_extend_rel10()
{
    // Test extending read-write with a copy node on the write (2) level
    // Meta = [1,2,1,2], rel = {(<10,20> -> <11,*>)}
    MDD rel = lddmc_true;
    rel = lddmc_make_copynode(rel, lddmc_false);
    rel = lddmc_makenode(20, rel, lddmc_false);
    rel = lddmc_makenode(11, rel, lddmc_false);
    rel = lddmc_makenode(10, rel, lddmc_false);
    MDD meta = lddmc_make_readwrite_meta(2, false);
    test_assert(lddmc_nodecount(rel) == 4);

    // we expect the copy node to be replaced with an explict write
    MDD rel_ext = lddmc_extend_rel(rel, meta, 4);
    test_assert(lddmc_nodecount(rel_ext) == 4);
    test_assert(lddmc_getvalue(rel_ext) == 10); rel_ext = lddmc_getdown(rel_ext);
    test_assert(lddmc_getvalue(rel_ext) == 11); rel_ext = lddmc_getdown(rel_ext);
    test_assert(lddmc_getvalue(rel_ext) == 20); rel_ext = lddmc_getdown(rel_ext);
    test_assert(lddmc_getvalue(rel_ext) == 20); rel_ext = lddmc_getdown(rel_ext);
    test_assert(rel_ext == lddmc_true);

    return 0;
}

int test_lddmc_extend_rel11()
{
    // Test extending read-write with a copy node on the only-write (4) level
    // Meta = [1,2,4], rel = {(<10,*> -> <11,*>)}
    MDD rel = lddmc_make_copynode(lddmc_true, lddmc_false);
    rel = stack_transition(10, 11, rel);
    MDD meta = lddmc_true;
    meta = lddmc_makenode(4, meta, lddmc_false);
    meta = lddmc_makenode(2, meta, lddmc_false);
    meta = lddmc_makenode(1, meta, lddmc_false);
    test_assert(lddmc_nodecount(rel) == 3);

    // we expect the copy node for only-read to be repaced by two copy nodes
    MDD rel_ext = lddmc_extend_rel(rel, meta, 4);
    test_assert(lddmc_nodecount(rel_ext) == 4);
    test_assert(lddmc_getvalue(rel_ext) == 10); rel_ext = lddmc_getdown(rel_ext);
    test_assert(lddmc_getvalue(rel_ext) == 11); rel_ext = lddmc_getdown(rel_ext);
    test_assert(lddmc_iscopy(rel_ext));         rel_ext = lddmc_getdown(rel_ext);
    test_assert(lddmc_iscopy(rel_ext));         rel_ext = lddmc_getdown(rel_ext);
    test_assert(rel_ext == lddmc_true);

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

int test_image_vs_relprod3()
{
    // 1. Create a rel + set of states
    // Meta = [1, 2, 0], Rel = {forall i: (<4,i> -> <5,i>)} (nvar = 2)
    MDD rel = stack_transition(4, 5, lddmc_true);
    MDD meta = lddmc_true;
    meta = lddmc_makenode(0, meta, lddmc_false);
    meta = lddmc_makenode(2, meta, lddmc_false);
    meta = lddmc_makenode(1, meta, lddmc_false);
    test_assert(lddmc_nodecount(rel) == 2);

    // s0 = {<4,10>}, s1 = {<4,10>,<4,11>}, s2 = {<5,3>}
    MDD s0 = lddmc_true;
    s0 = lddmc_makenode(10,s0, lddmc_false);
    s0 = lddmc_makenode(4, s0, lddmc_false);
    MDD s1 = lddmc_true;
    s1 = lddmc_makenode(11,s1, lddmc_false);
    s1 = lddmc_makenode(4, s1, lddmc_false);
    s1 = lddmc_union(s0, s1);
    MDD s2 = lddmc_true;
    s2 = lddmc_makenode(5, s2, lddmc_false);
    s2 = lddmc_makenode(3, s2, lddmc_false);
    test_assert(lddmc_satcount(s0) == 1);
    test_assert(lddmc_satcount(s1) == 2);
    test_assert(lddmc_satcount(s2) == 1);

    // 2. Test R.S with relprod
    MDD r0 = lddmc_relprod(s0, rel, meta); // {<5,10>}
    MDD r1 = lddmc_relprod(s1, rel, meta); // {<5,10>, <5,11>}
    MDD r2 = lddmc_relprod(s2, rel, meta); // {}
    test_assert(lddmc_satcount(r0) == 1);
    test_assert(lddmc_satcount(r1) == 2);
    test_assert(lddmc_satcount(r2) == 0);
    MDD temp = r0;
    test_assert(lddmc_getvalue(temp) == 5);     temp = lddmc_getdown(temp);
    test_assert(lddmc_getvalue(temp) == 10);    temp = lddmc_getdown(temp);
    test_assert(temp == lddmc_true);
    temp = r1;
    test_assert(lddmc_getvalue(temp) == 5);     temp = lddmc_getdown(temp);
    test_assert(lddmc_getvalue(temp) == 10);    temp = lddmc_getright(temp);
    test_assert(lddmc_getvalue(temp) == 11);    temp = lddmc_getdown(temp);
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

int test_image_vs_relprod4()
{
    // 1. Create a rel + set of states
    // Meta = [1, 2, -1], same as previous test but now with nvar = 3
    MDD rel = stack_transition(4, 5, lddmc_true);
    MDD meta = lddmc_true;
    meta = lddmc_makenode(-1, meta, lddmc_false);
    meta = lddmc_makenode(2, meta, lddmc_false);
    meta = lddmc_makenode(1, meta, lddmc_false);
    test_assert(lddmc_nodecount(rel) == 2);

    // s0 = {<4,10,10>}, s1 = {<4,10,10>,<4,11,3>}, s2 = {<5,3,8>}
    MDD s0 = lddmc_true;
    s0 = lddmc_makenode(10,s0, lddmc_false);
    s0 = lddmc_makenode(10,s0, lddmc_false);
    s0 = lddmc_makenode(4, s0, lddmc_false);
    MDD s1 = lddmc_true;
    s1 = lddmc_makenode(3, s1, lddmc_false);
    s1 = lddmc_makenode(11,s1, lddmc_false);
    s1 = lddmc_makenode(4, s1, lddmc_false);
    s1 = lddmc_union(s0, s1);
    MDD s2 = lddmc_true;
    s2 = lddmc_makenode(8, s2, lddmc_false);
    s2 = lddmc_makenode(5, s2, lddmc_false);
    s2 = lddmc_makenode(3, s2, lddmc_false);
    test_assert(lddmc_satcount(s0) == 1);
    test_assert(lddmc_satcount(s1) == 2);
    test_assert(lddmc_satcount(s2) == 1);

    // 2. Test R.S with relprod
    MDD r0 = lddmc_relprod(s0, rel, meta); // {<5,10,10>}
    MDD r1 = lddmc_relprod(s1, rel, meta); // {<5,10,10>, <5,11,3>}
    MDD r2 = lddmc_relprod(s2, rel, meta); // {}
    test_assert(lddmc_satcount(r0) == 1);
    test_assert(lddmc_satcount(r1) == 2);
    test_assert(lddmc_satcount(r2) == 0);
    MDD temp = r0;
    test_assert(lddmc_getvalue(temp) == 5);     temp = lddmc_getdown(temp);
    test_assert(lddmc_getvalue(temp) == 10);    temp = lddmc_getdown(temp);
    test_assert(lddmc_getvalue(temp) == 10);    temp = lddmc_getdown(temp);
    test_assert(temp == lddmc_true);
    temp = r1;
    test_assert(lddmc_getvalue(temp) == 5);     temp = lddmc_getdown(temp);
    test_assert(lddmc_getvalue(temp) == 10);    temp = lddmc_getdown(temp);
    test_assert(lddmc_getvalue(temp) == 10);    temp = lddmc_getdown(temp);
    test_assert(temp == lddmc_true);
    temp = r1;
    test_assert(lddmc_getvalue(temp) == 5);     temp = lddmc_getdown(temp);
    test_assert(lddmc_getvalue(temp) == 10);    temp = lddmc_getright(temp);
    test_assert(lddmc_getvalue(temp) == 11);    temp = lddmc_getdown(temp);
    test_assert(lddmc_getvalue(temp) == 3);     temp = lddmc_getdown(temp);
    test_assert(temp == lddmc_true);

    // 3. Extend R, test R.S with image
    MDD rel_ext  = lddmc_extend_rel(rel, meta, 6);
    MDD meta_ext = lddmc_make_readwrite_meta(3, false);
    MDD t0 = lddmc_image(s0, rel_ext, meta_ext);
    MDD t1 = lddmc_image(s1, rel_ext, meta_ext);
    MDD t2 = lddmc_image(s2, rel_ext, meta_ext);
    test_assert(t0 == r0);
    test_assert(t1 == r1);
    test_assert(t2 == r2);

    return 0;
}

int test_image_vs_relprod5()
{
    // 1. Create a rel + set of states
    // Meta = [1, 2, 5, -1], same as previous but with action label
    MDD rel = lddmc_makenode(123, lddmc_true, lddmc_false); // action label
    rel = stack_transition(4, 5, rel);
    MDD meta = lddmc_true;
    meta = lddmc_makenode(-1, meta, lddmc_false);
    meta = lddmc_makenode(5, meta, lddmc_false);
    meta = lddmc_makenode(2, meta, lddmc_false);
    meta = lddmc_makenode(1, meta, lddmc_false);
    test_assert(lddmc_nodecount(rel) == 3);

    // s0 = {<4,10,10>}, s1 = {<4,10,10>,<4,11,3>}, s2 = {<5,3,8>}
    MDD s0 = lddmc_true;
    s0 = lddmc_makenode(10,s0, lddmc_false);
    s0 = lddmc_makenode(10,s0, lddmc_false);
    s0 = lddmc_makenode(4, s0, lddmc_false);
    MDD s1 = lddmc_true;
    s1 = lddmc_makenode(3, s1, lddmc_false);
    s1 = lddmc_makenode(11,s1, lddmc_false);
    s1 = lddmc_makenode(4, s1, lddmc_false);
    s1 = lddmc_union(s0, s1);
    MDD s2 = lddmc_true;
    s2 = lddmc_makenode(8, s2, lddmc_false);
    s2 = lddmc_makenode(5, s2, lddmc_false);
    s2 = lddmc_makenode(3, s2, lddmc_false);
    test_assert(lddmc_satcount(s0) == 1);
    test_assert(lddmc_satcount(s1) == 2);
    test_assert(lddmc_satcount(s2) == 1);

    // 2. Test R.S with relprod
    MDD r0 = lddmc_relprod(s0, rel, meta); // {<5,10,10>}
    MDD r1 = lddmc_relprod(s1, rel, meta); // {<5,10,10>, <5,11,3>}
    MDD r2 = lddmc_relprod(s2, rel, meta); // {}
    test_assert(lddmc_satcount(r0) == 1);
    test_assert(lddmc_satcount(r1) == 2);
    test_assert(lddmc_satcount(r2) == 0);
    MDD temp = r0;
    test_assert(lddmc_getvalue(temp) == 5);     temp = lddmc_getdown(temp);
    test_assert(lddmc_getvalue(temp) == 10);    temp = lddmc_getdown(temp);
    test_assert(lddmc_getvalue(temp) == 10);    temp = lddmc_getdown(temp);
    test_assert(temp == lddmc_true);
    temp = r1;
    test_assert(lddmc_getvalue(temp) == 5);     temp = lddmc_getdown(temp);
    test_assert(lddmc_getvalue(temp) == 10);    temp = lddmc_getdown(temp);
    test_assert(lddmc_getvalue(temp) == 10);    temp = lddmc_getdown(temp);
    test_assert(temp == lddmc_true);
    temp = r1;
    test_assert(lddmc_getvalue(temp) == 5);     temp = lddmc_getdown(temp);
    test_assert(lddmc_getvalue(temp) == 10);    temp = lddmc_getright(temp);
    test_assert(lddmc_getvalue(temp) == 11);    temp = lddmc_getdown(temp);
    test_assert(lddmc_getvalue(temp) == 3);     temp = lddmc_getdown(temp);
    test_assert(temp == lddmc_true);

    // 3. Extend R, test R.S with image
    MDD rel_ext  = lddmc_extend_rel(rel, meta, 6);
    MDD meta_ext = lddmc_make_readwrite_meta(3, false);
    MDD t0 = lddmc_image(s0, rel_ext, meta_ext);
    MDD t1 = lddmc_image(s1, rel_ext, meta_ext);
    MDD t2 = lddmc_image(s2, rel_ext, meta_ext);
    test_assert(t0 == r0);
    test_assert(t1 == r1);
    test_assert(t2 == r2);

    return 0;
}

int test_image_vs_relprod6()
{
    // 1. Create a rel + set of states
    // Meta = [3, 1, 2], rel = {(<5,10> -> <5,20>)}
    MDD rel = stack_transition(10, 20, lddmc_true);
    rel = lddmc_makenode(5, rel, lddmc_false);
    MDD meta = lddmc_true;
    meta = lddmc_makenode(2, meta, lddmc_false);
    meta = lddmc_makenode(1, meta, lddmc_false);
    meta = lddmc_makenode(3, meta, lddmc_false);
    test_assert(lddmc_nodecount(rel) == 3);

    // s0 = {<5,10>}, s1 = {<5,10>,<5,12>}, s2 = {<6,10>}
    MDD s0 = lddmc_true;
    s0 = lddmc_makenode(10,s0, lddmc_false);
    s0 = lddmc_makenode(5, s0, lddmc_false);
    MDD s1 = lddmc_true;
    s1 = lddmc_makenode(12,s1, lddmc_false);
    s1 = lddmc_makenode(5, s1, lddmc_false);
    s1 = lddmc_union(s0, s1);
    MDD s2 = lddmc_true;
    s2 = lddmc_makenode(10,s2, lddmc_false);
    s2 = lddmc_makenode(6, s2, lddmc_false);
    test_assert(lddmc_satcount(s0) == 1);
    test_assert(lddmc_satcount(s1) == 2);
    test_assert(lddmc_satcount(s2) == 1);

    // 2. Test R.S with relprod
    MDD r0 = lddmc_relprod(s0, rel, meta); // {<5,20>}
    MDD r1 = lddmc_relprod(s1, rel, meta); // {<5,20>}
    MDD r2 = lddmc_relprod(s2, rel, meta); // {}
    test_assert(lddmc_satcount(r0) == 1);
    test_assert(lddmc_satcount(r1) == 1);
    test_assert(lddmc_satcount(r2) == 0);
    MDD temp = r0;
    test_assert(lddmc_getvalue(temp) == 5);     temp = lddmc_getdown(temp);
    test_assert(lddmc_getvalue(temp) == 20);    temp = lddmc_getdown(temp);
    test_assert(temp == lddmc_true);
    test_assert(r0 == r1);

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

int test_image_vs_relprod7()
{
    // 1. Create a rel + set of states
    // Meta = [4, 1, 2], rel = {(<*,10> -> <6, 20>)}
    MDD rel = stack_transition(10, 20, lddmc_true);
    rel = lddmc_makenode(6, rel, lddmc_false);
    MDD meta = lddmc_true;
    meta = lddmc_makenode(2, meta, lddmc_false);
    meta = lddmc_makenode(1, meta, lddmc_false);
    meta = lddmc_makenode(4, meta, lddmc_false);
    test_assert(lddmc_nodecount(rel) == 3);

    // s0 = {<3,10>}, s1 = {<3,10>,<7,10>}, s2 = {<3,11>}
    MDD s0 = lddmc_true;
    s0 = lddmc_makenode(10,s0, lddmc_false);
    s0 = lddmc_makenode(3, s0, lddmc_false);
    MDD s1 = lddmc_true;
    s1 = lddmc_makenode(10,s1, lddmc_false);
    s1 = lddmc_makenode(7, s1, lddmc_false);
    s1 = lddmc_union(s0, s1);
    MDD s2 = lddmc_true;
    s2 = lddmc_makenode(11,s2, lddmc_false);
    s2 = lddmc_makenode(3, s2, lddmc_false);
    test_assert(lddmc_satcount(s0) == 1);
    test_assert(lddmc_satcount(s1) == 2);
    test_assert(lddmc_satcount(s2) == 1);

    // 2. Test R.S with relprod
    MDD r0 = lddmc_relprod(s0, rel, meta); // {<6,20>}
    MDD r1 = lddmc_relprod(s1, rel, meta); // {<6,20>}
    MDD r2 = lddmc_relprod(s2, rel, meta); // {}
    test_assert(lddmc_satcount(r0) == 1);
    test_assert(lddmc_satcount(r1) == 1);
    test_assert(lddmc_satcount(r2) == 0);
    MDD temp = r0;
    test_assert(lddmc_getvalue(temp) == 6);     temp = lddmc_getdown(temp);
    test_assert(lddmc_getvalue(temp) == 20);    temp = lddmc_getdown(temp);
    test_assert(temp == lddmc_true);
    test_assert(r0 == r1);

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

int test_image_vs_relprod8()
{
    // 1. Create a rel + set of states
    // Meta = [1, 2, 1, 2], Rel = {<6,4> -> <7, 3>),(<*,20> -> <10,10>)}
    // (including a copy node for a read (1))
    MDD rel1 = lddmc_true;
    rel1 = stack_transition(4, 3, rel1);
    rel1 = stack_transition(6, 7, rel1);
    MDD rel2 = lddmc_true;
    rel2 = stack_transition(20, 10, rel2);
    rel2 = lddmc_makenode(10, rel2, lddmc_false);
    rel2 = lddmc_make_copynode(rel2, lddmc_false);
    MDD rel = lddmc_union(rel1, rel2);
    MDD meta = lddmc_make_readwrite_meta(2, false);

    // s0 = {<6,4>}, s1 = {<30,20>}, s2 = {<6,4>,<30,20>} 
    MDD s0 = lddmc_true;
    s0 = lddmc_makenode(4, s0, lddmc_false);
    s0 = lddmc_makenode(6, s0, lddmc_false);
    MDD s1 = lddmc_true;
    s1 = lddmc_makenode(20, s1, lddmc_false);
    s1 = lddmc_makenode(30, s1, lddmc_false);
    MDD s2 = lddmc_union(s0, s1);
    test_assert(lddmc_satcount(s0) == 1);
    test_assert(lddmc_satcount(s1) == 1);
    test_assert(lddmc_satcount(s2) == 2);

    // 2. Test R.S with relprod
    MDD r0 = lddmc_relprod(s0, rel, meta); // {<7,3>}
    MDD r1 = lddmc_relprod(s1, rel, meta); // {<10,10>}
    MDD r2 = lddmc_relprod(s2, rel, meta); // {<7,3>,<10,10>}
    test_assert(lddmc_satcount(r0) == 1);
    test_assert(lddmc_satcount(r1) == 1);
    test_assert(lddmc_satcount(r2) == 2);
    MDD temp = r0;
    test_assert(lddmc_getvalue(temp) == 7);     temp = lddmc_getdown(temp);
    test_assert(lddmc_getvalue(temp) == 3);     temp = lddmc_getdown(temp);
    test_assert(temp == lddmc_true);
    temp = r1;
    test_assert(lddmc_getvalue(temp) == 10);    temp = lddmc_getdown(temp);
    test_assert(lddmc_getvalue(temp) == 10);    temp = lddmc_getdown(temp);
    test_assert(temp == lddmc_true);
    test_assert(r2 == lddmc_union(r0, r1));

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

MDD test_rel_union(MDD rel0, MDD m0, MDD rel1, MDD m1, MDD s, MDD nvars)
{
    // extend rels
    MDD ext0 = lddmc_extend_rel(rel0, m0, 2*nvars);
    MDD ext1 = lddmc_extend_rel(rel1, m1, 2*nvars);
    MDD meta = lddmc_make_readwrite_meta(nvars, false);
    MDD t0, t1, rel_union, succ0, succ1, succ2;

    // compute successors seprately on original rels, then union of successors
    t0 = lddmc_relprod(s, rel0, m0);
    t1 = lddmc_relprod(s, rel1, m1);
    succ0 = lddmc_union(t0, t1);

    // compute successors separately on extended rels, then union of successors
    t0 = lddmc_image(s, ext0, meta);
    t1 = lddmc_image(s, ext1, meta);
    succ1 = lddmc_union(t0, t1);

    // compute union of extended rel, then successors
    rel_union = lddmc_union(ext0, ext1);
    succ2 = lddmc_image(s, rel_union, meta);

    test_assert(succ0 == succ1);
    test_assert(succ1 == succ2);

    return succ0;
}

int test_union_copy1()
{
    // {*,3,5} v {4,8} = {*,3,4,5,8}
    MDD a = lddmc_makenode(5, lddmc_true, lddmc_false);
    a = lddmc_makenode(3, lddmc_true, a);
    a = lddmc_make_copynode(lddmc_true, a);
    MDD b = lddmc_makenode(8, lddmc_true, lddmc_false);
    b = lddmc_makenode(4, lddmc_true, b);

    MDD u = lddmc_union(a, b);
    test_assert(lddmc_nodecount(u) == 5);
    test_assert(lddmc_iscopy(u));           u = lddmc_getright(u);
    test_assert(lddmc_getvalue(u) == 3);    u = lddmc_getright(u);
    test_assert(lddmc_getvalue(u) == 4);    u = lddmc_getright(u);
    test_assert(lddmc_getvalue(u) == 5);    u = lddmc_getright(u);
    test_assert(lddmc_getvalue(u) == 8);    u = lddmc_getright(u);

    return 0;
}

int test_union_copy2()
{
    // {*,3,5} v {*,4,8} = {*,3,4,5,8}
    MDD a = lddmc_makenode(5, lddmc_true, lddmc_false);
    a = lddmc_makenode(3, lddmc_true, a);
    a = lddmc_make_copynode(lddmc_true, a);
    MDD b = lddmc_makenode(8, lddmc_true, lddmc_false);
    b = lddmc_makenode(4, lddmc_true, b);
    b = lddmc_make_copynode(lddmc_true, b);

    MDD u = lddmc_union(a, b);
    test_assert(lddmc_nodecount(u) == 5);
    test_assert(lddmc_iscopy(u));           u = lddmc_getright(u);
    test_assert(lddmc_getvalue(u) == 3);    u = lddmc_getright(u);
    test_assert(lddmc_getvalue(u) == 4);    u = lddmc_getright(u);
    test_assert(lddmc_getvalue(u) == 5);    u = lddmc_getright(u);
    test_assert(lddmc_getvalue(u) == 8);    u = lddmc_getright(u);

    return 0;
}

int test_rel_union1()
{
    // union rels with meta [1,2,1,2] ; [1,2,1,2]
    // r0 = {<1,2> -> <11,12>}, r1 = {<1,3>, <5,5>}
    uint32_t nvars = 2;
    MDD r0 = lddmc_true;
    r0 = stack_transition(2, 12, r0);
    r0 = stack_transition(1, 11, r0);
    MDD r1 = lddmc_true;
    r1 = stack_transition(3, 5, r1);
    r1 = stack_transition(1, 5, r1);
    MDD m0 = lddmc_make_readwrite_meta(nvars, false);
    MDD m1 = lddmc_make_readwrite_meta(nvars, false);

    // s0 = {<1,2>}, s1 = {<1,3>}, s2 = {<1,2>, <1,3>}
    MDD s0 = lddmc_true;
    s0 = lddmc_makenode(2, s0, lddmc_false);
    s0 = lddmc_makenode(1, s0, lddmc_false);
    MDD s1 = lddmc_true;
    s1 = lddmc_makenode(3, s1, lddmc_false);
    s1 = lddmc_makenode(1, s1, lddmc_false);
    MDD s2 = lddmc_union(s0, s1);
    test_assert(lddmc_satcount(s0) == 1);
    test_assert(lddmc_satcount(s1) == 1);
    test_assert(lddmc_satcount(s2) == 2);
    
    MDD t0 = test_rel_union(r0, m0, r1, m1, s0, nvars); // {<11,12>}
    MDD t1 = test_rel_union(r0, m0, r1, m1, s1, nvars); // {<5,5>}
    MDD t2 = test_rel_union(r0, m0, r1, m1, s2, nvars); // {<5,5>,<11,12>}
    test_assert(lddmc_satcount(t0) == 1);
    test_assert(lddmc_satcount(t1) == 1);
    test_assert(lddmc_satcount(t2) == 2);
    MDD temp = t0;
    test_assert(lddmc_getvalue(temp) == 11);    temp = lddmc_getdown(temp);
    test_assert(lddmc_getvalue(temp) == 12);    temp = lddmc_getdown(temp);
    test_assert(temp == lddmc_true);
    temp = t1;
    test_assert(lddmc_getvalue(temp) == 5);     temp = lddmc_getdown(temp);
    test_assert(lddmc_getvalue(temp) == 5);     temp = lddmc_getdown(temp);
    test_assert(temp == lddmc_true);
    temp = t2;
    test_assert(lddmc_getvalue(temp) == 5);     temp = lddmc_getdown(temp);
    test_assert(lddmc_getvalue(temp) == 5);     temp = lddmc_getdown(temp);
    test_assert(temp == lddmc_true);
    temp = lddmc_getright(t2);
    test_assert(lddmc_getvalue(temp) == 11);    temp = lddmc_getdown(temp);
    test_assert(lddmc_getvalue(temp) == 12);    temp = lddmc_getdown(temp);
    test_assert(temp == lddmc_true);

    return 0;
}

int test_rel_union2()
{
    // union rels with meta [1,2] ; [0]
    // r0 = {<4> -> <5>, <4> -> <6>}, r1 = {<*> -> <*>}
    //  |                   |
    // [4]                 [*]
    //  |                   |
    // [5][6]              [*]
    uint32_t nvars = 1;
    MDD r0 = lddmc_true;
    r0 = lddmc_makenode(6, lddmc_true, lddmc_false);
    r0 = lddmc_makenode(5, lddmc_true, r0);
    r0 = lddmc_makenode(4, r0, lddmc_false);
    MDD r1 = lddmc_true; // skipped var
    MDD m0 = lddmc_make_readwrite_meta(nvars, false);
    MDD m1 = lddmc_makenode(0, lddmc_true, lddmc_false);

    // s0 = {<4>}, s1 = {<10>}, s2 = {<4>, <10>}
    MDD s0 = lddmc_makenode(4, lddmc_true, lddmc_false);
    MDD s1 = lddmc_makenode(10,lddmc_true, lddmc_false);
    MDD s2 = lddmc_union(s0, s1);
    test_assert(lddmc_satcount(s0) == 1);
    test_assert(lddmc_satcount(s1) == 1);
    test_assert(lddmc_satcount(s2) == 2);

    MDD t0 = test_rel_union(r0, m0, r1, m1, s0, nvars); // {<4>,<5>,<6>}
    MDD t1 = test_rel_union(r0, m0, r1, m1, s1, nvars); // {<10>}
    MDD t2 = test_rel_union(r0, m0, r1, m1, s2, nvars); // {<4>,<5>,<6>,<10>}
    test_assert(lddmc_satcount(t0) == 3);
    test_assert(lddmc_satcount(t1) == 1);
    test_assert(lddmc_satcount(t2) == 4);
    MDD temp = t0;
    test_assert(lddmc_getvalue(temp) == 4);     temp = lddmc_getright(temp);
    test_assert(lddmc_getvalue(temp) == 5);     temp = lddmc_getright(temp);
    test_assert(lddmc_getvalue(temp) == 6);     temp = lddmc_getright(temp);
    test_assert(temp == lddmc_false);
    temp = t1;
    test_assert(lddmc_getvalue(temp) == 10);    temp = lddmc_getdown(temp);
    test_assert(temp == lddmc_true);
    temp = t2;
    test_assert(lddmc_getvalue(temp) == 4);     temp = lddmc_getright(temp);
    test_assert(lddmc_getvalue(temp) == 5);     temp = lddmc_getright(temp);
    test_assert(lddmc_getvalue(temp) == 6);     temp = lddmc_getright(temp);
    test_assert(lddmc_getvalue(temp) == 10);    temp = lddmc_getright(temp);
    test_assert(temp == lddmc_false);

    return 0;
}

int test_rel_union3()
{
    // union rels with meta [0] ; [0]
    // r0 = {<*>,<*>}, r1 = {<*> -> <*>}
    //  |               |
    // [*]             [*]
    //  |               |
    // [*]             [*]
    uint32_t nvars = 1;
    MDD r0 = lddmc_true; // skipped var
    MDD r1 = lddmc_true;
    MDD m0 = lddmc_makenode(0, lddmc_true, lddmc_false);
    MDD m1 = lddmc_makenode(0, lddmc_true, lddmc_false);
    
    MDD ext0 = lddmc_extend_rel(r0, m0, 2*nvars);
    MDD ext1 = lddmc_extend_rel(r1, m0, 2*nvars);
    test_assert(ext0 == ext1);
    MDD rel_union = lddmc_union(ext0, ext1); // {<*>, <*>}
    test_assert(ext0 == rel_union);

    return 0;
}

int test_rel_union4()
{
    // union rels with meta [1,2] ; [4]
    // r0 = {<7> -> <8>}, r1 = {<*> -> <5>}
    //  |               |
    // [7]             [*]
    //  |               |
    // [8]             [5]
    uint32_t nvars = 1;
    MDD r0 = lddmc_true;
    r0 = lddmc_makenode(8, r0, lddmc_false);
    r0 = lddmc_makenode(7, r0, lddmc_false);
    MDD r1 = lddmc_makenode(5, lddmc_true, lddmc_false);
    MDD m0 = lddmc_make_readwrite_meta(nvars, false);
    MDD m1 = lddmc_makenode(4, lddmc_true, lddmc_false);
    
    MDD ext0 = lddmc_extend_rel(r0, m0, 2*nvars);
    MDD ext1 = lddmc_extend_rel(r1, m1, 2*nvars);
    MDD rel_union = lddmc_union(ext0, ext1);
    MDD temp = rel_union;
    test_assert(lddmc_iscopy(temp));            temp = lddmc_getdown(temp);
    test_assert(lddmc_getvalue(temp) == 5);     temp = lddmc_getdown(temp);
    test_assert(temp == lddmc_true);            temp = lddmc_getright(rel_union);
    test_assert(lddmc_getvalue(temp) == 7);     temp = lddmc_getdown(temp);
    test_assert(lddmc_getvalue(temp) == 8);     temp = lddmc_getdown(temp);
    test_assert(temp == lddmc_true);

    // s0 = {<7>}, s1 = {<3>}, s2 = {<7>,<3>}
    MDD s0 = lddmc_makenode(7, lddmc_true, lddmc_false);
    MDD s1 = lddmc_makenode(3, lddmc_true, lddmc_false);
    MDD s2 = lddmc_union(s0, s1);
    test_assert(lddmc_satcount(s0) == 1);
    test_assert(lddmc_satcount(s1) == 1);
    test_assert(lddmc_satcount(s2) == 2);

    MDD t0 = test_rel_union(r0, m0, r1, m1, s0, nvars); // {<8>,<5>}
    MDD t1 = test_rel_union(r0, m0, r1, m1, s1, nvars); // {<5>}
    MDD t2 = test_rel_union(r0, m0, r1, m1, s2, nvars); // {<8>,<5>}
    test_assert(lddmc_satcount(t0) == 2);
    test_assert(lddmc_satcount(t1) == 1);
    test_assert(lddmc_satcount(t2) == 2);
    test_assert(t0 == t2);
    temp = t0;
    test_assert(lddmc_getvalue(temp) == 5);     temp = lddmc_getright(temp);
    test_assert(lddmc_getvalue(temp) == 8);     temp = lddmc_getright(temp);
    test_assert(temp == lddmc_false);
    temp = t1;
    test_assert(lddmc_getvalue(temp) == 5);     temp = lddmc_getright(temp);
    test_assert(temp == lddmc_false);

    return 0;
}

int test_rel_union5()
{
    // union rels with meta [4] ; [4]
    // r0 = {<*> -> <5>}, r1 = {<*> -> <8>}
    //  |                  |
    // [*]                [*]
    //  |                  |
    // [5]                [8]
    uint32_t nvars = 1;
    MDD r0 = lddmc_makenode(5, lddmc_true, lddmc_false); 
    MDD r1 = lddmc_makenode(8, lddmc_true, lddmc_false);
    MDD m0 = lddmc_makenode(4, lddmc_true, lddmc_false);
    MDD m1 = lddmc_makenode(4, lddmc_true, lddmc_false);

    // s0 = {<7>}, s1 = {<8>}, s2 = {<7>, <8>}
    MDD s0 = lddmc_makenode(7, lddmc_true, lddmc_false);
    MDD s1 = lddmc_makenode(8, lddmc_true, lddmc_false);
    MDD s2 = lddmc_union(s0, s1);
    test_assert(lddmc_satcount(s0) == 1);
    test_assert(lddmc_satcount(s1) == 1);
    test_assert(lddmc_satcount(s2) == 2);

    // {<*> -> <4>, <*> -> <5>}
    MDD t0 = test_rel_union(r0, m0, r1, m1, s0, nvars); // {<>,<8>}
    MDD t1 = test_rel_union(r0, m0, r1, m1, s1, nvars); // {<5>,<8>}
    MDD t2 = test_rel_union(r0, m0, r1, m1, s2, nvars); // {<5>,<8>}
    test_assert(t0 == t1);
    test_assert(t1 == t2);
    test_assert(lddmc_satcount(t0) == 2);
    MDD temp = t0;
    test_assert(lddmc_getvalue(temp) == 5);     temp = lddmc_getright(temp);
    test_assert(lddmc_getvalue(temp) == 8);     temp = lddmc_getdown(temp);
    test_assert(temp == lddmc_true);

    return 0;
}

int test_rel_union6()
{
    // union rels with meta [4] ; [0]
    // r0 = {<*>,<*>}, r1 = {<*> -> <*>}
    //  |               |
    // [*]             [*]
    //  |               |
    // [5]             [*]
    uint32_t nvars = 1;
    MDD r0 = lddmc_makenode(5, lddmc_true, lddmc_false); 
    MDD r1 = lddmc_true; // skipped var
    MDD m0 = lddmc_makenode(4, lddmc_true, lddmc_false);
    MDD m1 = lddmc_makenode(0, lddmc_true, lddmc_false);

    // s0 = {<8>}, s1 = {<9>}, s2 = {<8>, <9>}
    MDD s0 = lddmc_makenode(8, lddmc_true, lddmc_false);
    MDD s1 = lddmc_makenode(9, lddmc_true, lddmc_false);
    MDD s2 = lddmc_union(s0, s1);
    test_assert(lddmc_satcount(s0) == 1);
    test_assert(lddmc_satcount(s1) == 1);
    test_assert(lddmc_satcount(s2) == 2);

    MDD ext0 = lddmc_extend_rel(r0, m0, 2*nvars);
    MDD ext1 = lddmc_extend_rel(r1, m1, 2*nvars);
    MDD rel_union = lddmc_union(ext0, ext1);
    MDD temp = rel_union;
    test_assert(lddmc_iscopy(temp));            temp = lddmc_getdown(temp);
    test_assert(lddmc_iscopy(temp));            temp = lddmc_getright(temp);
    test_assert(lddmc_getvalue(temp) == 5);     temp = lddmc_getdown(temp);
    test_assert(temp == lddmc_true);

    // {<*> -> <*>, <*> -> <5>}
    MDD t0 = test_rel_union(r0, m0, r1, m1, s0, nvars);
    MDD t1 = test_rel_union(r0, m0, r1, m1, s1, nvars);
    MDD t2 = test_rel_union(r0, m0, r1, m1, s2, nvars);

    return 0;
}

MDD generate_random_meta(uint32_t nvars)
{
    MDD meta = lddmc_true;

    MDD n_skips = 0;

    for (uint32_t i = 0; i < nvars; i++) {
        int r = rand() % 5;
        switch (r) {
        case 0: // skip var
            meta = lddmc_makenode(0, meta, lddmc_false);
            n_skips++;
            break;
        case 1: // read-write twice as likely as other options)
            meta = lddmc_makenode(2, meta, lddmc_false);
            meta = lddmc_makenode(1, meta, lddmc_false);
            break;
        case 2: // read-write
            meta = lddmc_makenode(2, meta, lddmc_false);
            meta = lddmc_makenode(1, meta, lddmc_false);
            break;
        case 3: // only-read
            meta = lddmc_makenode(3, meta, lddmc_false);
            break;
        case 4: // only-write
            meta = lddmc_makenode(4, meta, lddmc_false);
            break;
        default:
            assert(false && "r shouldn't be outside this range");
            break;
        }
    }

    if (n_skips == nvars) {
        // retry (we don't want to skipp all variables)
        meta = generate_random_meta(nvars);
    }

    return meta;
}

// Generates a random rel compatible with Sylvan's lddmc_relprod
MDD generate_random_rel(MDD meta, uint32_t max_val, MDD *s_init)
{
    if (meta == lddmc_true) {
        *s_init = lddmc_true;
        return lddmc_true;
    }

    uint32_t m_val = lddmc_getvalue(meta);
    MDD next_meta  = lddmc_getdown(meta);
    if (m_val == 1) {
        assert(lddmc_getvalue(next_meta) == 2);
    }
    
    MDD res;
    if (m_val == 0) {
         // skipped variable: no entry in rel, random entry in s_init
        res = generate_random_rel(next_meta, max_val, s_init);
        uint32_t r = rand() % max_val;
        *s_init = lddmc_makenode(r, *s_init, lddmc_false);
    }
    else if (m_val == 1 || m_val == 2 || m_val == 3 || m_val == 4) {
        // node in rel (1 for now)
        MDD child = generate_random_rel(next_meta, max_val, s_init);

        uint32_t r = rand() % max_val;
        if (r % 10 == 0) // 1/10 chance of * node
            res = lddmc_make_copynode(child, lddmc_false);
        else
            res = lddmc_makenode(r, child, lddmc_false);
        
        // add a starting state which is triggers the transition
        if (m_val == 1 || m_val == 3) {
            // relation reads (or only-reads) 'r'; add 'r' to s_init
            *s_init = lddmc_makenode(r, *s_init, lddmc_false);
        }
        else if (m_val == 4) {
            // relation only-writes 'r'; add random read to s_init
            uint32_t rr = rand() % max_val;
            *s_init = lddmc_makenode(rr, *s_init, lddmc_false);
        }
    }
    else {
        assert(false && "meta value not recognized");
    }

    return res;
}

int test_random_relations(uint32_t num_tests, uint32_t nvars, uint32_t n_rels)
{
    MDD *meta, *rel, *rel_ext, *s, states, merged_rels;
    MDD succ0, succ1, succ2, succ_temp;
    MDD r_w_meta = lddmc_make_readwrite_meta(nvars, false);
    
    meta    = malloc(n_rels * sizeof(MDD));
    rel     = malloc(n_rels * sizeof(MDD));
    rel_ext = malloc(n_rels * sizeof(MDD));
    s       = malloc(n_rels * sizeof(MDD));

    for (uint32_t i = 0; i < num_tests; i++) {
        // renerate random relation
        for (uint32_t j = 0; j < n_rels; j++) {
            meta[j] = generate_random_meta(nvars);
            rel[j] = generate_random_rel(meta[j], 128, &s[j]);
            assert(lddmc_satcount(s[j]) == 1);
        }

        // states := UNION_j (s[j])
        states = lddmc_false;
        for (uint32_t j = 0; j < n_rels; j++) {
            states = lddmc_union(states, s[j]);
        }

        // compute successors with relprod (one rel at a time)
        succ0 = lddmc_false;
        for (uint32_t j = 0; j < n_rels; j++) {
            succ_temp = lddmc_relprod(states, rel[j], meta[j]);
            succ0     = lddmc_union(succ0, succ_temp);
        }

        // compute successors with lddmc_image (one rel at a time)
        succ1 = lddmc_false;
        for (uint32_t j = 0; j < n_rels; j++) {
            rel_ext[j] = lddmc_extend_rel(rel[j], meta[j], 2*nvars);
            succ_temp  = lddmc_image(states, rel_ext[j], r_w_meta);
            succ1      = lddmc_union(succ1, succ_temp);
        }

        // compute successors with lddmc_image after merging rels
        merged_rels = lddmc_false;
        for (uint32_t j = 0; j < n_rels; j++) {
            merged_rels = lddmc_union(merged_rels, rel_ext[j]);
        }
        succ2 = lddmc_image(states, merged_rels, r_w_meta);

        assert(lddmc_satcount(succ0) >= 1);
        if (succ0 != succ1 || succ1 != succ2) {
            printf("Issue found, writing issue rels\n");
            for (uint32_t j = 0; j < n_rels; j++) {
                write_mdd(meta[j], "meta", j);
                write_mdd(rel[j], "rel", j);
                write_mdd(rel_ext[j], "rel_ext", j);
                write_mdd(s[j], "states", j);
            }
            write_mdd(states, "state_union", 0);
                write_mdd(merged_rels, "rel_ext_union", 0);
                write_mdd(succ0, "succ", 0);
                write_mdd(succ1, "succ", 1);
                write_mdd(succ2, "succ", 2);
            assert(succ0 == succ1);
            assert(succ1 == succ2);
        }
    }

    free(meta);
    free(rel);
    free(rel_ext);
    free(s);

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
    if (test_lddmc_extend_rel8()) return 1;
    if (test_lddmc_extend_rel9()) return 1;
    if (test_lddmc_extend_rel10()) return 1;
    if (test_lddmc_extend_rel11()) return 1;
    printf("OK\n");

    printf("Testing lddmc_image against lddmc_relprod...    "); fflush(stdout);
    if (test_image_vs_relprod1()) return 1;
    if (test_image_vs_relprod2()) return 1;
    if (test_image_vs_relprod3()) return 1;
    if (test_image_vs_relprod4()) return 1;
    if (test_image_vs_relprod5()) return 1;
    if (test_image_vs_relprod6()) return 1;
    if (test_image_vs_relprod7()) return 1;
    if (test_image_vs_relprod8()) return 1;
    printf("OK\n");

    printf("Testing union w/ copy nodes...                  "); fflush(stdout);
    if (test_union_copy1()) return 1;
    if (test_union_copy2()) return 1;
    printf("OK\n");

    printf("Testing lddmc_rel_union...                      "); fflush(stdout);
    if (test_rel_union1()) return 1;
    if (test_rel_union2()) return 1;
    if (test_rel_union3()) return 1;
    if (test_rel_union4()) return 1;
    if (test_rel_union5()) return 1;
    if (test_rel_union6()) return 1;
    printf("OK\n");

    srand(42);
    uint32_t n = 500;
    uint32_t max_vars = 5;
    uint32_t max_rels = 3;
    printf("Testing on randomly generated relations... \n");
    for (uint32_t nvars = 1; nvars <= max_vars; nvars++) {
        for (uint32_t n_rels = 1; n_rels <= max_rels; n_rels++) {
            printf("    *%dx union of %d random rels with %d vars... ", n, n_rels, nvars);
            fflush(stdout);
            test_random_relations(n, nvars, n_rels);
            printf("OK\n");
        }
    }

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