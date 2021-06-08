#include "relnext.h"


BDD my_relnext(BDD s, BDD r, BDDSET x, BDDSET xp, MTBDDMAP unprime_map)
{
    BDD res;
    //BDDSET all_vars = mtbdd_set_union(x, xp);
    //res = sylvan_relnext(s, r, all_vars);

    // 1. s' = s ^ r (apply relation)
    res = sylvan_and(s, r);

    // 2. s' = \exists x: s' (quantify unprimed vars away)
    res = sylvan_exists(res, x);

    // 3. s' = s'[x' := x] (relabel primed to unprimed)
    res = sylvan_compose(res, unprime_map);

    return res;
}