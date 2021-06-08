#include "relnext.h"

int fourtytwo()
{
    return 42;
}


BDD my_relnext(BDD s, BDD r, BDDSET x, BDDSET xp, MTBDDMAP xp_to_x)
{
    BDD res;
 //   BDDSET all_vars = mtbdd_set_union(x, xp);
 //   res = sylvan_relnext(s, r, all_vars);

 //   /*
    // 1. s' = s ^ r (apply relation)
    res = sylvan_and(s, r);

    // 2. s' = \exists x: s' (quantify unprimed vars away)
    res = sylvan_exists(res, x);

    FILE *f;
    f = fopen("primed.dot", "w");
    sylvan_fprintdot(f, res);
    fclose(f);

    printf("states: %0.f\n", sylvan_satcount(res, xp));

    // 3. s' = s'[x' := x] (relabel primed to unprimed)
    res = sylvan_compose(res, xp_to_x);

    
    f = fopen("unprimed.dot", "w");
    sylvan_fprintdot(f, res);
    fclose(f);

    exit(1);
//    */


    return res;
}