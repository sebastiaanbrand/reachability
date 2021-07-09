#!/bin/bash

beem_selection="adding.1.bdd 
                blocks.2.bdd"

beem_vn_stats="beem_vanilla_stats.csv"
beem_ga_stats="beem_ga_stats.csv"

for filename in $beem_selection; do
    for i in {1..5}; do # repeat to average time ?
        # LTSmin generated bdd files without any options
        timeout 5m ./../build/distbdd-spin17/bddmc models/beem/bdds_vanilla/$filename --workers=1 --strategy=bfs --merge-relations --count-nodes --statsfile=$beem_vn_stats
        timeout 5m ./../build/distbdd-spin17/bddmc models/beem/bdds_vanilla/$filename --workers=1 --strategy=sat --count-nodes --statsfile=$beem_vn_stats
        timeout 5m ./../build/distbdd-spin17/bddmc models/beem/bdds_vanilla/$filename --workers=1 --strategy=rec --merge-relations --count-nodes --statsfile=$beem_vn_stats

        # LTSmin generated bdd files with -r ga option
        timeout 5m ./../build/distbdd-spin17/bddmc models/beem/bdds_ga/$filename --workers=1 --strategy=bfs --merge-relations --count-nodes --statsfile=$beem_ga_stats
        timeout 5m ./../build/distbdd-spin17/bddmc models/beem/bdds_ga/$filename --workers=1 --strategy=sat --count-nodes --statsfile=$beem_ga_stats
        timeout 5m ./../build/distbdd-spin17/bddmc models/beem/bdds_ga/$filename --workers=1 --strategy=rec --merge-relations --count-nodes --statsfile=$beem_ga_stats
    done
done
