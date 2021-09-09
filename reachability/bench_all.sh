#!/bin/bash

# usage:
# bash bench_all.sh [-w workers] [beem|petri|promela]

beem_vn_stats="bench_data/beem_vanilla_stats.csv"
beem_ga_stats="bench_data/beem_ga_stats.csv"
petri_vn_stats="bench_data/petrinets_vanilla_stats.csv"
petri_ga_stats="bench_data/petrinets_ga_stats.csv"
promela_vn_stats="bench_data/promela_vanilla_stats.csv"
promela_ga_stats="bench_data/promela_ga_stats.csv"
awari_stats="bench_data/awari_stats.csv"

mkdir -p bench_data

maxtime=2m # todo: add as command line argument
num_workers=1 # can pass e.g. -w "1 2 4 8" to run with 1, 2, 4, and 8 workers

while getopts ":w:" opt; do
  case $opt in
    w) num_workers="$OPTARG"
    ;;
    \?) echo "Invalid option -$OPTARG" >&1; exit 1;
    ;;
  esac
done

# process arguments for running subset of all benchmarks
for var in "$@"; do
    if [[ $var == 'beem' ]]; then bench_beem_vn=true; bench_beem_ga=true; fi
    if [[ $var == 'petri' ]]; then bench_ptri_vn=true; bench_ptri_ga=true; fi
    if [[ $var == 'promela' ]]; then bench_prom_vn=true; bench_prom_ga=true; fi
    if [[ $var == 'beem-vanilla' ]]; then bench_beem_vn=true; fi
    if [[ $var == 'beem-ga' ]]; then bench_beem_ga=true; fi
    if [[ $var == 'petri-vanilla' ]]; then bench_ptri_vn=true; fi
    if [[ $var == 'petri-ga' ]]; then bench_ptri_ga=true; fi
    if [[ $var == 'promela-vanilla' ]]; then bench_prom_vn=true; fi
    if [[ $var == 'promela-ga' ]]; then bench_prom_ga=true; fi
    if [[ $var == 'awari' ]]; then bench_awari=true; fi
    if [[ $var == 'all' ]]; then
        bench_beem_vn=true; bench_ptri_vn=true; bench_prom_vn=true
        bench_beem_ga=true; bench_ptri_ga=true; bench_prom_ga=true
        bench_awari=true;
    fi
done

if [[ $bench_beem_vn ]]; then
    for filename in models/beem/bdds_vanilla/*.bdd; do
        for nw in $num_workers; do
            timeout $maxtime ./../build/reachability/bddmc $filename --workers=$nw --strategy=bfs --merge-relations --count-nodes --statsfile=$beem_vn_stats
            timeout $maxtime ./../build/reachability/bddmc $filename --workers=$nw --strategy=sat --count-nodes --statsfile=$beem_vn_stats
            timeout $maxtime ./../build/reachability/bddmc $filename --workers=$nw --strategy=rec --merge-relations --count-nodes --statsfile=$beem_vn_stats
        done
    done
fi

if [[ $bench_beem_ga ]]; then
    for filename in models/beem/bdds_ga/*.bdd; do
        for nw in $num_workers; do
            timeout $maxtime ./../build/reachability/bddmc $filename --workers=$nw --strategy=bfs --merge-relations --count-nodes --statsfile=$beem_ga_stats
            timeout $maxtime ./../build/reachability/bddmc $filename --workers=$nw --strategy=sat --count-nodes --statsfile=$beem_ga_stats
            timeout $maxtime ./../build/reachability/bddmc $filename --workers=$nw --strategy=rec --merge-relations --count-nodes --statsfile=$beem_ga_stats
        done
    done
fi

if [[ $bench_ptri_vn ]]; then
    for filename in models/petrinets/bdds_vanilla/*.bdd; do
        for nw in $num_workers; do
            timeout $maxtime ./../build/reachability/bddmc $filename --workers=$nw --strategy=bfs --merge-relations --count-nodes --statsfile=$petri_vn_stats
            timeout $maxtime ./../build/reachability/bddmc $filename --workers=$nw --strategy=sat --count-nodes --statsfile=$petri_vn_stats
            timeout $maxtime ./../build/reachability/bddmc $filename --workers=$nw --strategy=rec --merge-relations --count-nodes --statsfile=$petri_vn_stats
        done
    done
fi

if [[ $bench_ptri_ga ]]; then
    for filename in models/petrinets/bdds_ga/*.bdd; do
        for nw in $num_workers; do
            timeout $maxtime ./../build/reachability/bddmc $filename --workers=$nw --strategy=bfs --merge-relations --count-nodes --statsfile=$petri_ga_stats
            timeout $maxtime ./../build/reachability/bddmc $filename --workers=$nw --strategy=sat --count-nodes --statsfile=$petri_ga_stats
            timeout $maxtime ./../build/reachability/bddmc $filename --workers=$nw --strategy=rec --merge-relations --count-nodes --statsfile=$petri_ga_stats
        done
    done
fi

if [[ $bench_prom_vn ]]; then
    for filename in models/promela/bdds_vanilla/*.bdd; do
        for nw in $num_workers; do
            timeout $maxtime ./../build/reachability/bddmc $filename --workers=$nw --strategy=bfs --merge-relations --count-nodes --statsfile=$promela_vn_stats
            timeout $maxtime ./../build/reachability/bddmc $filename --workers=$nw --strategy=sat --count-nodes --statsfile=$promela_vn_stats
            timeout $maxtime ./../build/reachability/bddmc $filename --workers=$nw --strategy=rec --merge-relations --count-nodes --statsfile=$promela_vn_stats
        done
    done
fi

if [[ $bench_prom_ga ]]; then
    for filename in models/promela/bdds_ga/*.bdd; do
        for nw in $num_workers; do
            timeout $maxtime ./../build/reachability/bddmc $filename --workers=$nw --strategy=bfs --merge-relations --count-nodes --statsfile=$promela_ga_stats
            timeout $maxtime ./../build/reachability/bddmc $filename --workers=$nw --strategy=sat --count-nodes --statsfile=$promela_ga_stats
            timeout $maxtime ./../build/reachability/bddmc $filename --workers=$nw --strategy=rec --merge-relations --count-nodes --statsfile=$promela_ga_stats
        done
    done
fi

if [[ $bench_awari ]]; then
    for filename in models/awari/*.bdd; do
        for nw in $num_workers; do
            timeout $maxtime ./../build/reachability/bddmc $filename --workers=$nw --strategy=bfs --merge-relations --count-nodes --statsfile=$awari_stats
            timeout $maxtime ./../build/reachability/bddmc $filename --workers=$nw --strategy=rec --merge-relations --count-nodes --statsfile=$awari_stats
        done
    done
fi
