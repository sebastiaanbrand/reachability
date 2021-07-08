#!/bin/bash

beem_vn_stats="been_vanilla_stats.csv"
beem_ga_stats="beem_ga_stats.csv"

for filename in models/beem/bdds_vanilla/*.bdd; do
    #for i in {1..5}; do # repeat to average time ?
        timeout 5m ./../build/distbdd-spin17/bddmc $filename --workers=1 --strategy=bfs --merge-relations --count-nodes --statsfile=$beem_vn_stats
        timeout 5m ./../build/distbdd-spin17/bddmc $filename --workers=1 --strategy=sat --count-nodes --statsfile=$beem_vn_stats
        timeout 5m ./../build/distbdd-spin17/bddmc $filename --workers=1 --strategy=rec --merge-relations --count-nodes --statsfile=$beem_vn_stats
    #done
done

for filename in models/beem/bdds_ga/*.bdd; do
    #for i in {1..5}; do # repeat to average time ?
        timeout 5m ./../build/distbdd-spin17/bddmc $filename --workers=1 --strategy=bfs --merge-relations --count-nodes --statsfile=$beem_ga_stats
        timeout 5m ./../build/distbdd-spin17/bddmc $filename --workers=1 --strategy=sat --count-nodes --statsfile=$beem_ga_stats
        timeout 5m ./../build/distbdd-spin17/bddmc $filename --workers=1 --strategy=rec --merge-relations --count-nodes --statsfile=$beem_ga_stats
    #done
done