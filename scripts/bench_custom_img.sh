#!/bin/bash

maxtime=2m
num_workers=1 # can pass e.g. -w "1 2 4 8" to run with 1, 2, 4, and 8 workers

lddmc=reach_algs/build/lddmc

outputfolder=bench_data/all/custom_image_test
mkdir -p $outputfolder

beem_sl_stats_ldd="$outputfolder/beem_sloan_stats_ldd.csv"
petri_sl_stats_ldd="$outputfolder/petrinets_sloan_stats_ldd.csv"
promela_sl_stats_ldd="$outputfolder/promela_sloan_stats_ldd.csv"

while getopts "w:t:m:" opt; do
  case $opt in
    w) num_workers="$OPTARG"
    ;;
    t) maxtime="$OPTARG"
    ;;
    \?) echo "Invalid option -$OPTARG" >&1; exit 1;
    ;;
  esac
done

# process arguments for running subset of all benchmarks
for var in "$@"; do
    if [[ $var == 'beem-sloan' ]]; then bench_beem_sl=true; fi
    if [[ $var == 'petri-sloan' ]]; then bench_ptri_sl=true; fi
    if [[ $var == 'promela-sloan' ]]; then bench_prom_sl=true; fi
    if [[ $var == 'ldd' ]]; then bench_ldd=true; fi
    if [[ $var == 'extend-rels' ]]; then extend_rels=true; fi
done


echo "Running following benchmarks with [$num_workers] workers:"
if [[ $bench_beem_sl && $bench_ldd ]]; then echo "  - BEEM LDDs Sloan"; fi
if [[ $bench_ptri_sl && $bench_ldd ]]; then echo "  - Petri Nets LDDs Sloan"; fi
if [[ $bench_prom_sl && $bench_ldd ]]; then echo "  - Promela LDDs Sloan"; fi

echo "timeout per run is $maxtime"
echo "writing .csv output files to folder $outputfolder"
read -p "Press enter to start"

# BEEM, Sloan LDDs (merging relation for BFS and REC requires overapproximated LDDs)
if [[ $bench_beem_sl && $bench_ldd ]]; then
    if [[ $extend_rels ]]; then
        for filename in models/beem/ldds/sloan/*.ldd; do
            for nw in $num_workers; do
                timeout $maxtime ./$lddmc $filename --workers=$nw --strategy=bfs-plain --custom-image --merge-relations --count-nodes --statsfile=$beem_sl_stats_ldd
                timeout $maxtime ./$lddmc $filename --workers=$nw --strategy=sat --count-nodes --statsfile=$beem_sl_stats_ldd
            done
        done
    else
        for filename in models/beem/ldds/sloan/overapprox/*.ldd; do
            for nw in $num_workers; do
                timeout $maxtime ./$lddmc $filename --workers=$nw --strategy=rec --merge-relations --count-nodes --statsfile=$beem_sl_stats_ldd
                timeout $maxtime ./$lddmc $filename --workers=$nw --strategy=rec --custom-image --merge-relations --count-nodes --statsfile=$beem_sl_stats_ldd
            done
        done
    fi
fi

# Petri, Sloan LDDs (merging relation for BFS and REC requires overapproximated LDDs)
if [[ $bench_ptri_sl && $bench_ldd ]]; then
    if [[ $extend_rels ]]; then
        for filename in models/petrinets/ldds/sloan/*.ldd; do
            for nw in $num_workers; do
                timeout $maxtime ./$lddmc $filename --workers=$nw --strategy=bfs-plain --custom-image --merge-relations --count-nodes --statsfile=$petri_sl_stats_ldd
                timeout $maxtime ./$lddmc $filename --workers=$nw --strategy=sat --count-nodes --statsfile=$petri_sl_stats_ldd
            done
        done
    else
        for filename in models/petrinets/ldds/sloan/overapprox/*.ldd; do
            for nw in $num_workers; do
                timeout $maxtime ./$lddmc $filename --workers=$nw --strategy=rec --merge-relations --count-nodes --statsfile=$petri_sl_stats_ldd
                timeout $maxtime ./$lddmc $filename --workers=$nw --strategy=rec --custom-image --merge-relations --count-nodes --statsfile=$petri_sl_stats_ldd
            done
        done
    fi
fi

# Promela, Sloan LDDs (merging relation for BFS and REC requires overapproximated LDDs)
if [[ $bench_prom_sl && $bench_ldd ]]; then
    if [[ $extend_rels ]]; then
        for filename in models/promela/ldds/sloan/*.ldd; do
            for nw in $num_workers; do
                timeout $maxtime ./$lddmc $filename --workers=$nw --strategy=bfs-plain --custom-image --merge-relations --count-nodes --statsfile=$promela_sl_stats_ldd
                timeout $maxtime ./$lddmc $filename --workers=$nw --strategy=sat --count-nodes --statsfile=$promela_sl_stats_ldd
            done
        done
    else
        for filename in models/promela/ldds/sloan/overapprox/*.ldd; do
            for nw in $num_workers; do
                timeout $maxtime ./$lddmc $filename --workers=$nw --strategy=rec --merge-relations --count-nodes --statsfile=$promela_sl_stats_ldd
                timeout $maxtime ./$lddmc $filename --workers=$nw --strategy=rec --custom-image --merge-relations --count-nodes --statsfile=$promela_sl_stats_ldd
            done
        done
    fi
fi

