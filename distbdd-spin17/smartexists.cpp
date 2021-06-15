#include <vector>
#include <algorithm>

#include "smartexists.h"
#include "sylvan.h"
#include "sylvan_obj.hpp"

using namespace sylvan;

// eh...
static uint64_t peaknodes = 0;

enum smartexists {
    standard = 0,
    local_greedy = 1,
    greedy_oracle = 2
};

uint64_t smartexists_get_peaknodes() { return peaknodes; }

static void update_peaknodes_bdd(BDD bdd) {
    uint64_t count = sylvan_nodecount(bdd);
    if (count > peaknodes) peaknodes = count;
}
static void update_peaknodes(uint64_t count) {
    if (count > peaknodes) peaknodes = count;
}


sylvan::Bdd LocalGreedyExists(const sylvan::Bdd In, const sylvan::BddSet &Vars, int n) {
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
            update_peaknodes(count);
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
        update_peaknodes_bdd(Set.GetBDD());
    }
    return Set;
}

sylvan::Bdd GreedyOracleExists(const sylvan::Bdd In, const sylvan::BddSet &Vars, int n) {
    sylvan::Bdd Set = In;
    sylvan::BddSet vars = Vars;

    // greedily determine "oracle" order
    // try all vars and store results as pairs (var, new-size) in vec
    typedef std::pair<sylvan::BddSet, size_t> my_pair;
    std::vector<my_pair> vec;
    for (sylvan::BddSet x = vars; !x.isEmpty() ; x = x.Next()) {
        sylvan::BddSet var(sylvan::Bdd(x.TopVar()));
        size_t count = Set.ExistAbstract(var).NodeCount();
        vec.push_back({var, count});
        update_peaknodes(count);
    }

    // sort vec according to new-size
    std::sort(vec.begin(), vec.end(), [](const my_pair &a, const my_pair &b) -> bool
            {
                return a.second < b.second;
            });

    // quantify vars away in that order
    int i = vars.size();
    for (my_pair &p : vec) {
        Set = Set.ExistAbstract(p.first);
        vars.remove(p.first.TopVar());
        if (++i == n) break;
    }
    update_peaknodes_bdd(Set.GetBDD());

    return Set;
}

BDD my_relnext(BDD s, BDD r, BDDSET x, BDD unprime_map, int smartexists)
{
    BDD res;

    // 1. s' = s ^ r (apply relation)
    res = sylvan_and(s, r);
    update_peaknodes_bdd(res);

    // 2. s' = \exists x: s' (quantify unprimed vars away)
    switch (smartexists)
    {
    case standard:
        res = sylvan_exists(res, x);
        break;
    case local_greedy:
        res = LocalGreedyExists(Bdd(res), BddSet(x), 1).GetBDD();
        break;
    case greedy_oracle:
        res = GreedyOracleExists(Bdd(res), BddSet(x), 1).GetBDD();
        break;
    default:
        res = sylvan_exists(res, x);
        break;
    }
    update_peaknodes_bdd(res);
    // NOTE: peak nodes is not that insightful of a statistic to keep because 
    // res is always the same
    

    // 3. s' = s'[x' := x] (relabel primed to unprimed)
    res = sylvan_compose(res, unprime_map);

    return res;
}
