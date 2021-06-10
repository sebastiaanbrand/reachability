#include <vector>
#include <algorithm>

#include "relnext.h"
#include "sylvan.h"
#include "sylvan_obj.hpp"

using namespace sylvan;

sylvan::Bdd SmartExist(const sylvan::Bdd In, const sylvan::BddSet &Vars, int n) {
    sylvan::Bdd Set = In;
    sylvan::BddSet vars = Vars;
    while (!vars.isEmpty()) {

        // try all vars and store results as pairs (var, new-size) in vec
        typedef std::pair<sylvan::BddSet, size_t> my_pair;
        std::vector<my_pair> vec;
        for (sylvan::BddSet x = vars; !x.isEmpty() ; x = x.Next()) {
            sylvan::BddSet var(sylvan::Bdd(x.TopVar()));
            size_t count = Set.ExistAbstract(var).NodeCount();
            vec.push_back({var, count});
        }

        // sort vec according to new-size
        std::sort(vec.begin(), vec.end(), [](const my_pair &a, const my_pair &b) -> bool
                {
                    return a.second < b.second;
                });

        // quantify the best n variables away
        int i = 0;
        for (my_pair &p : vec) {
            Set = Set.ExistAbstract(p.first);
            vars.remove(p.first.TopVar());
            if (++i == n) break;
        }
    }
    return Set;
}

BDD my_relnext(BDD s, BDD r, BDDSET x, BDDSET xp, BDD unprime_map)
{
    BDD res;
    //BDDSET all_vars = mtbdd_set_union(x, xp);
    //res = sylvan_relnext(s, r, all_vars);

    // 1. s' = s ^ r (apply relation)
    //res = sylvan_and(s, r);
    res = sylvan_and(s, r);

    // 2. s' = \exists x: s' (quantify unprimed vars away)
    res = SmartExist(Bdd(res), BddSet(x), 1).GetBDD();

    // 3. s' = s'[x' := x] (relabel primed to unprimed)
    //res = res.Compose(Bdd(unprime_map));
    res = sylvan_compose(res, unprime_map);

    return res;
}
