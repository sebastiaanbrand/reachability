#!/bin/bash

beem_selection="telephony.1.bdd 
                telephony.2.bdd
                telephony.3.bdd"

beem_vn_stats="bench_data/beem_vanilla_stats.csv"
beem_ga_stats="bench_data/beem_ga_stats.csv"

mkdir -p bench_data

maxtime=5m

for filename in $beem_selection; do
    #for i in {1..5}; do # repeat to average time ?
        # LTSmin generated bdd files without any options
        timeout $maxtime ./../build/reachability/bddmc spin17models/beem/bdds_vanilla/$filename --workers=1 --strategy=bfs --merge-relations --count-nodes --statsfile=$beem_vn_stats
        timeout $maxtime ./../build/reachability/bddmc spin17models/beem/bdds_vanilla/$filename --workers=1 --strategy=sat --count-nodes --statsfile=$beem_vn_stats
        timeout $maxtime ./../build/reachability/bddmc spin17models/beem/bdds_vanilla/$filename --workers=1 --strategy=rec --merge-relations --count-nodes --statsfile=$beem_vn_stats

        # LTSmin generated bdd files with -r ga option
        timeout $maxtime ./../build/reachability/bddmc spin17models/beem/bdds_ga/$filename --workers=1 --strategy=bfs --merge-relations --count-nodes --statsfile=$beem_ga_stats
        timeout $maxtime ./../build/reachability/bddmc spin17models/beem/bdds_ga/$filename --workers=1 --strategy=sat --count-nodes --statsfile=$beem_ga_stats
        timeout $maxtime ./../build/reachability/bddmc spin17models/beem/bdds_ga/$filename --workers=1 --strategy=rec --merge-relations --count-nodes --statsfile=$beem_ga_stats
    #done
done
