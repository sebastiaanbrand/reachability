#!/bin/bash

# usage:
# bash bench_all.sh [-w workers] [beem|petri|promela] [bdd|ldd]

# output files
beem_vn_stats="bench_data/beem_vanilla_stats_bdd.csv"
beem_sl_stats="bench_data/beem_sloan_stats_bdd.csv"
petri_vn_stats="bench_data/petrinets_vanilla_stats_bdd.csv"
petri_sl_stats="bench_data/petrinets_sloan_stats_bdd.csv"
promela_vn_stats="bench_data/promela_vanilla_stats_bdd.csv"
promela_sl_stats="bench_data/promela_sloan_stats_bdd.csv"
awari_stats="bench_data/awari_stats_bdd.csv"
beem_vn_stats_ldd="bench_data/beem_vanilla_stats_ldd.csv"
beem_sl_stats_ldd="bench_data/beem_sloan_stats_ldd.csv"

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
    if [[ $var == 'beem' ]]; then bench_beem_vn=true; bench_beem_sl=true; fi
    if [[ $var == 'petri' ]]; then bench_ptri_vn=true; bench_ptri_sl=true; fi
    if [[ $var == 'promela' ]]; then bench_prom_vn=true; bench_prom_sl=true; fi
    if [[ $var == 'beem-vanilla' ]]; then bench_beem_vn=true; fi
    if [[ $var == 'beem-sloan' ]]; then bench_beem_sl=true; fi
    if [[ $var == 'petri-vanilla' ]]; then bench_ptri_vn=true; fi
    if [[ $var == 'petri-sloan' ]]; then bench_ptri_sl=true; fi
    if [[ $var == 'promela-vanilla' ]]; then bench_prom_vn=true; fi
    if [[ $var == 'promela-sloan' ]]; then bench_prom_sl=true; fi
    if [[ $var == 'awari' ]]; then bench_awari=true; fi
    if [[ $var == 'bdd' ]]; then bench_bdd=true; fi
    if [[ $var == 'ldd' ]]; then bench_ldd=true; fi
    if [[ $var == 'all' ]]; then
        bench_beem_vn=true; bench_ptri_vn=true; bench_prom_vn=true
        bench_beem_sl=true; bench_ptri_sl=true; bench_prom_sl=true
        bench_bdd=true; bench_ldd=true
        bench_awari=true;
    fi
done

# BEEM, vanilla BDDs
if [[ $bench_beem_vn && $bench_bdd ]]; then
    for filename in models/beem/bdds/vanilla/*.bdd; do
        for nw in $num_workers; do
            timeout $maxtime ./../build/reachability/bddmc $filename --workers=$nw --strategy=bfs --merge-relations --count-nodes --statsfile=$beem_vn_stats
            timeout $maxtime ./../build/reachability/bddmc $filename --workers=$nw --strategy=sat --count-nodes --statsfile=$beem_vn_stats
            timeout $maxtime ./../build/reachability/bddmc $filename --workers=$nw --strategy=rec --merge-relations --count-nodes --statsfile=$beem_vn_stats
        done
    done
fi

# BEEM, Sloan BDDs
if [[ $bench_beem_sl && $bench_bdd ]]; then
    for filename in models/beem/bdds/sloan/*.bdd; do
        for nw in $num_workers; do
            timeout $maxtime ./../build/reachability/bddmc $filename --workers=$nw --strategy=bfs --merge-relations --count-nodes --statsfile=$beem_sl_stats
            timeout $maxtime ./../build/reachability/bddmc $filename --workers=$nw --strategy=sat --count-nodes --statsfile=$beem_sl_stats
            timeout $maxtime ./../build/reachability/bddmc $filename --workers=$nw --strategy=rec --merge-relations --count-nodes --statsfile=$beem_sl_stats
        done
    done
fi

# Petri nets, vanilla BDDs
if [[ $bench_ptri_vn && $bench_bdd ]]; then
    for filename in models/petrinets/bdds/vanilla/*.bdd; do
        for nw in $num_workers; do
            timeout $maxtime ./../build/reachability/bddmc $filename --workers=$nw --strategy=bfs --merge-relations --count-nodes --statsfile=$petri_vn_stats
            timeout $maxtime ./../build/reachability/bddmc $filename --workers=$nw --strategy=sat --count-nodes --statsfile=$petri_vn_stats
            timeout $maxtime ./../build/reachability/bddmc $filename --workers=$nw --strategy=rec --merge-relations --count-nodes --statsfile=$petri_vn_stats
        done
    done
fi

# Petri nets, Sloan BDDs
if [[ $bench_ptri_sl && $bench_bdd ]]; then
    for filename in models/petrinets/bdds/sloan/*.bdd; do
        for nw in $num_workers; do
            timeout $maxtime ./../build/reachability/bddmc $filename --workers=$nw --strategy=bfs --merge-relations --count-nodes --statsfile=$petri_sl_stats
            timeout $maxtime ./../build/reachability/bddmc $filename --workers=$nw --strategy=sat --count-nodes --statsfile=$petri_sl_stats
            timeout $maxtime ./../build/reachability/bddmc $filename --workers=$nw --strategy=rec --merge-relations --count-nodes --statsfile=$petri_sl_stats
        done
    done
fi

# Promela, vanilla BDDs
if [[ $bench_prom_vn && $bench_bdd ]]; then
    for filename in models/promela/bdds/vanilla/*.bdd; do
        for nw in $num_workers; do
            timeout $maxtime ./../build/reachability/bddmc $filename --workers=$nw --strategy=bfs --merge-relations --count-nodes --statsfile=$promela_vn_stats
            timeout $maxtime ./../build/reachability/bddmc $filename --workers=$nw --strategy=sat --count-nodes --statsfile=$promela_vn_stats
            timeout $maxtime ./../build/reachability/bddmc $filename --workers=$nw --strategy=rec --merge-relations --count-nodes --statsfile=$promela_vn_stats
        done
    done
fi

# Promela, Sloan BDDs
if [[ $bench_prom_sl && $bench_bdd ]]; then
    for filename in models/promela/bdds/sloan/*.bdd; do
        for nw in $num_workers; do
            timeout $maxtime ./../build/reachability/bddmc $filename --workers=$nw --strategy=bfs --merge-relations --count-nodes --statsfile=$promela_sl_stats
            timeout $maxtime ./../build/reachability/bddmc $filename --workers=$nw --strategy=sat --count-nodes --statsfile=$promela_sl_stats
            timeout $maxtime ./../build/reachability/bddmc $filename --workers=$nw --strategy=rec --merge-relations --count-nodes --statsfile=$promela_sl_stats
        done
    done
fi

# Awari (global BDD relations, predefined variable order)
if [[ $bench_awari && $bench_bdd ]]; then
    for filename in models/awari/*.bdd; do
        for nw in $num_workers; do
            timeout $maxtime ./../build/reachability/bddmc $filename --workers=$nw --strategy=bfs --merge-relations --count-nodes --statsfile=$awari_stats
            timeout $maxtime ./../build/reachability/bddmc $filename --workers=$nw --strategy=rec --merge-relations --count-nodes --statsfile=$awari_stats
        done
    done
fi

# BEEM, vanilla LDDs (merging relation for BFS and REC requires overapproximated LDDs)
if [[ $bench_beem_vn && $bench_ldd ]]; then
    for filename in models/beem/ldds/vanilla/overapprox/*.ldd; do
        for nw in $num_workers; do
            timeout $maxtime ./../build/reachability/lddmc $filename --workers=$nw --strategy=bfs --merge-relations --count-nodes --statsfile=$beem_vn_stats_ldd
            timeout $maxtime ./../build/reachability/lddmc models/beem/ldds/vanilla/$(basename $filename) --workers=$nw --strategy=sat --count-nodes --statsfile=$beem_vn_stats_ldd
            timeout $maxtime ./../build/reachability/lddmc $filename --workers=$nw --strategy=rec --merge-relations --count-nodes --statsfile=$beem_vn_stats_ldd
        done
    done
fi

# BEEM, Sloan LDDs (merging relation for BFS and REC requires overapproximated LDDs)
if [[ $bench_beem_sl && $bench_ldd ]]; then
    for filename in models/beem/ldds/sloan/overapprox/*.ldd; do
        for nw in $num_workers; do
            timeout $maxtime ./../build/reachability/lddmc $filename --workers=$nw --strategy=bfs --merge-relations --count-nodes --statsfile=$beem_sl_stats_ldd
            timeout $maxtime ./../build/reachability/lddmc models/beem/ldds/sloan/$(basename $filename) --workers=$nw --strategy=sat --count-nodes --statsfile=$beem_sl_stats_ldd
            timeout $maxtime ./../build/reachability/lddmc $filename --workers=$nw --strategy=rec --merge-relations --count-nodes --statsfile=$beem_sl_stats_ldd
        done
    done
fi
