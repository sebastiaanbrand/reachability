#!/bin/bash

beem_vn_stats="bench_data/beem_vanilla_stats.csv"
beem_ga_stats="bench_data/beem_ga_stats.csv"
petri_vn_stats="bench_data/petrinets_vanilla_stats.csv"
petri_ga_stats="bench_data/petrinets_ga_stats.csv"
promela_vn_stats="bench_data/promela_vanilla_stats.csv"
promela_ga_stats="bench_data/promela_ga_stats.csv"

for filename in spin17models/beem/bdds_vanilla/*.bdd; do
    #for i in {1..5}; do # repeat to average time ?
        timeout 5m ./../build/distbdd-spin17/bddmc $filename --workers=1 --strategy=bfs --merge-relations --count-nodes --statsfile=$beem_vn_stats
        timeout 5m ./../build/distbdd-spin17/bddmc $filename --workers=1 --strategy=sat --count-nodes --statsfile=$beem_vn_stats
        timeout 5m ./../build/distbdd-spin17/bddmc $filename --workers=1 --strategy=rec --merge-relations --count-nodes --statsfile=$beem_vn_stats
    #done
done

for filename in spin17models/beem/bdds_ga/*.bdd; do
    #for i in {1..5}; do # repeat to average time ?
        timeout 5m ./../build/distbdd-spin17/bddmc $filename --workers=1 --strategy=bfs --merge-relations --count-nodes --statsfile=$beem_ga_stats
        timeout 5m ./../build/distbdd-spin17/bddmc $filename --workers=1 --strategy=sat --count-nodes --statsfile=$beem_ga_stats
        timeout 5m ./../build/distbdd-spin17/bddmc $filename --workers=1 --strategy=rec --merge-relations --count-nodes --statsfile=$beem_ga_stats
    #done
done

for filename in spin17models/petrinets/bdds_vanilla/*.bdd; do
    #for i in {1..5}; do # repeat to average time ?
        timeout 5m ./../build/distbdd-spin17/bddmc $filename --workers=1 --strategy=bfs --merge-relations --count-nodes --statsfile=$petri_vn_stats
        timeout 5m ./../build/distbdd-spin17/bddmc $filename --workers=1 --strategy=sat --count-nodes --statsfile=$petri_vn_stats
        timeout 5m ./../build/distbdd-spin17/bddmc $filename --workers=1 --strategy=rec --merge-relations --count-nodes --statsfile=$petri_vn_stats
    #done
done

for filename in spin17models/petrinets/bdds_ga/*.bdd; do
    #for i in {1..5}; do # repeat to average time ?
        timeout 5m ./../build/distbdd-spin17/bddmc $filename --workers=1 --strategy=bfs --merge-relations --count-nodes --statsfile=$petri_ga_stats
        timeout 5m ./../build/distbdd-spin17/bddmc $filename --workers=1 --strategy=sat --count-nodes --statsfile=$petri_ga_stats
        timeout 5m ./../build/distbdd-spin17/bddmc $filename --workers=1 --strategy=rec --merge-relations --count-nodes --statsfile=$petri_ga_stats
    #done
done

for filename in spin17models/promela/bdds_vanilla/*.bdd; do
    #for i in {1..5}; do # repeat to average time ?
        timeout 5m ./../build/distbdd-spin17/bddmc $filename --workers=1 --strategy=bfs --merge-relations --count-nodes --statsfile=$promela_vn_stats
        timeout 5m ./../build/distbdd-spin17/bddmc $filename --workers=1 --strategy=sat --count-nodes --statsfile=$promela_vn_stats
        timeout 5m ./../build/distbdd-spin17/bddmc $filename --workers=1 --strategy=rec --merge-relations --count-nodes --statsfile=$promela_vn_stats
    #done
done

for filename in spin17models/promela/bdds_ga/*.bdd; do
    #for i in {1..5}; do # repeat to average time ?
        timeout 5m ./../build/distbdd-spin17/bddmc $filename --workers=1 --strategy=bfs --merge-relations --count-nodes --statsfile=$promela_ga_stats
        timeout 5m ./../build/distbdd-spin17/bddmc $filename --workers=1 --strategy=sat --count-nodes --statsfile=$promela_ga_stats
        timeout 5m ./../build/distbdd-spin17/bddmc $filename --workers=1 --strategy=rec --merge-relations --count-nodes --statsfile=$promela_ga_stats
    #done
done
