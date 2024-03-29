#!/bin/bash

# usage:
# bash bench_all.sh [-w workers] [beem|petri|promela]-[all|vanilla|sloan|force] [bdd|ldd] [test-par]

maxtime=2m # todo: add as command line argument
num_workers=1 # can pass e.g. -w "1 2 4 8" to run with 1, 2, 4, and 8 workers

bddmc=reach_algs/build/bddmc
lddmc=reach_algs/build/lddmc

while getopts "w:t:m:o:" opt; do
  case $opt in
    w) num_workers="$OPTARG"
    ;;
    t) maxtime="$OPTARG"
    ;;
    o) custom_folder=bench_data/"$OPTARG"; overwrite_folder=true;
    ;;
    \?) echo "Invalid option -$OPTARG" >&1; exit 1;
    ;;
  esac
done


# some defaults
test_sat=true
test_reach=true
confirm=true

# process arguments for running subset of all benchmarks
for var in "$@"; do
    if [[ $var == 'beem-all' ]]; then bench_beem_vn=true; bench_beem_sl=true; bench_beem_f=true; fi
    if [[ $var == 'petri-all' ]]; then bench_ptri_vn=true; bench_ptri_sl=true; bench_ptri_f=true; fi
    if [[ $var == 'promela-all' ]]; then bench_prom_vn=true; bench_prom_sl=true; bench_prom_f=true; fi
    if [[ $var == 'beem-vanilla' ]]; then bench_beem_vn=true; fi
    if [[ $var == 'beem-force' ]]; then bench_beem_f=true; fi
    if [[ $var == 'beem-sloan' ]]; then bench_beem_sl=true; fi
    if [[ $var == 'petri-vanilla' ]]; then bench_ptri_vn=true; fi
    if [[ $var == 'petri-sloan' ]]; then bench_ptri_sl=true; fi
    if [[ $var == 'petri-force' ]]; then bench_ptri_f=true; fi
    if [[ $var == 'promela-vanilla' ]]; then bench_prom_vn=true; fi
    if [[ $var == 'promela-sloan' ]]; then bench_prom_sl=true; fi
    if [[ $var == 'promela-force' ]]; then bench_prom_f=true; fi
    if [[ $var == 'awari' ]]; then bench_awari=true; fi
    if [[ $var == 'bdd' ]]; then bench_bdd=true; fi
    if [[ $var == 'ldd' ]]; then bench_ldd=true; fi
    if [[ $var == 'bdd-static' ]]; then bench_bdd_static=true; fi
    if [[ $var == 'ldd-static' ]]; then bench_ldd_static=true; fi
    if [[ $var == 'test-par' ]]; then test_par=true; fi
    if [[ $var == 'test-par-only' ]]; then test_par=true; test_reach=false; fi
    if [[ $var == 'bfs' ]]; then test_bfs=true; fi
    if [[ $var == 'no-confirm' ]]; then confirm=false; fi
    if [[ $var == 'overapprox' ]]; then bench_overapprox=true; fi
    if [[ $var == 'deadlocks' ]]; then deadlocks="--deadlocks"; fi
    if [[ $var == 'all' ]]; then
        bench_beem_vn=true; bench_ptri_vn=true; bench_prom_vn=true
        bench_beem_sl=true; bench_ptri_sl=true; bench_prom_sl=true
        bench_beem_f=true;  bench_ptri_f=true;  bench_prom_f=true
        bench_bdd=true; bench_ldd=true
        bench_awari=true;
    fi
done

# output folder
if [[ $num_workers == 1 ]]; then
  outputfolder=bench_data/all/single_worker/$maxtime
elif [[ $num_workers == "1 2 4 8" || $num_workers == "1 8" ]]; then
  outputfolder=bench_data/all/par_8/$maxtime
elif [[ $num_workers == "1 16 32 64 96" ]]; then
  outputfolder=bench_data/all/par_96/$maxtime
elif [[ $num_workers == "1 64" ]]; then
  outputfolder=bench_data/all/par_64/$maxtime
else
  outputfolder=bench_data/all/par/$maxtime
fi

dt=$(date '+%Y%m%d_%H%M%S');
outputfolder=$outputfolder/$dt

if [[ $overwrite_folder ]]; then
    outputfolder=$custom_folder
fi

if [[ $deadlocks ]]; then
    filename_app="_deadlocks"
fi

# output files
beem_vn_stats="$outputfolder/beem_vanilla_stats_bdd$filename_app.csv"
beem_sl_stats="$outputfolder/beem_sloan_stats_bdd$filename_app.csv"
beem_f_stats="$outputfolder/beem_force_stats_bdd$filename_app.csv"
petri_vn_stats="$outputfolder/petrinets_vanilla_stats_bdd$filename_app.csv"
petri_sl_stats="$outputfolder/petrinets_sloan_stats_bdd$filename_app.csv"
petri_f_stats="$outputfolder/petrinets_force_stats_bdd$filename_app.csv"
promela_vn_stats="$outputfolder/promela_vanilla_stats_bdd$filename_app.csv"
promela_sl_stats="$outputfolder/promela_sloan_stats_bdd$filename_app.csv"
promela_f_stats="$outputfolder/promela_force_stats_bdd$filename_app.csv"
awari_stats="$outputfolder/awari_stats_bdd_$filename_app.csv"

beem_vn_stats_ldd="$outputfolder/beem_vanilla_stats_ldd.csv"
beem_sl_stats_ldd="$outputfolder/beem_sloan_stats_ldd.csv"
beem_f_stats_ldd="$outputfolder/beem_force_stats_ldd.csv"
petri_vn_stats_ldd="$outputfolder/petrinets_vanilla_stats_ldd.csv"
petri_sl_stats_ldd="$outputfolder/petrinets_sloan_stats_ldd.csv"
petri_f_stats_ldd="$outputfolder/petrinets_force_stats_ldd.csv"
promela_vn_stats_ldd="$outputfolder/promela_vanilla_stats_ldd.csv"
promela_sl_stats_ldd="$outputfolder/promela_sloan_stats_ldd.csv"
promela_f_stats_ldd="$outputfolder/promela_force_stats_ldd.csv"

petri_sl_stats_bdd_static="$outputfolder/petrinets_sloan_stats_bdd_static.csv"
petri_sl_stats_ldd_static="$outputfolder/petrinets_sloan_stats_ldd_static.csv"

echo "Running following benchmarks with [$num_workers] workers:"

if [[ $bench_beem_vn && $bench_bdd ]]; then echo "  - BEEM BDDs vanilla"; fi
if [[ $bench_beem_sl && $bench_bdd ]]; then echo "  - BEEM BDDs Sloan"; fi
if [[ $bench_beem_f  && $bench_bdd ]]; then echo "  - BEEM BDDs FORCE"; fi
if [[ $bench_beem_vn && $bench_ldd ]]; then echo "  - BEEM LDDs vanilla"; fi
if [[ $bench_beem_sl && $bench_ldd ]]; then echo "  - BEEM LDDs Sloan"; fi
if [[ $bench_beem_f  && $bench_ldd ]]; then echo "  - BEEM LDDs FORCE"; fi

if [[ $bench_ptri_vn && $bench_bdd ]]; then echo "  - Petri Nets BDDs vanilla"; fi
if [[ $bench_ptri_sl && $bench_bdd ]]; then echo "  - Petri Nets BDDs Sloan"; fi
if [[ $bench_ptri_f  && $bench_bdd ]]; then echo "  - Petri Nets BDDs FORCE"; fi
if [[ $bench_ptri_vn && $bench_ldd ]]; then echo "  - Petri Nets LDDs vanilla"; fi
if [[ $bench_ptri_sl && $bench_ldd ]]; then echo "  - Petri Nets LDDs Sloan"; fi
if [[ $bench_ptri_f  && $bench_ldd ]]; then echo "  - Petri Nets LDDs Force"; fi

if [[ $bench_prom_vn && $bench_bdd ]]; then echo "  - Promela BDDs vanilla"; fi
if [[ $bench_prom_sl && $bench_bdd ]]; then echo "  - Promela BDDs Sloan"; fi
if [[ $bench_prom_f  && $bench_bdd ]]; then echo "  - Promela BDDs FORCE"; fi
if [[ $bench_prom_vn && $bench_ldd ]]; then echo "  - Promela LDDs vanilla"; fi
if [[ $bench_prom_sl && $bench_ldd ]]; then echo "  - Promela LDDs Sloan"; fi
if [[ $bench_prom_f  && $bench_ldd ]]; then echo "  - Promela LDDs FORCE"; fi

if [[ $bench_ptri_sl && $bench_bdd_static ]]; then echo "  - Petri Nets Static BDDs Sloan"; fi
if [[ $bench_ptri_sl && $bench_ldd_static ]]; then echo "  - Petri Nets Static LDDs Sloan"; fi

if [[ $bench_awari && $bench_bdd ]]; then echo "  - Awari BDDs"; fi

if [[ $test_par && $bench_bdd ]]; then echo "  * Testing parallelism for BDD rec-reach"; fi
if [[ $test_reach == false ]]; then echo "    (only testing reach-par and sat-par, not basic reach)"; fi
if [[ $deadlocks && $bench_bdd ]]; then echo "  * Computing deadlocks with BDD algs"; fi
if [[ $bench_ldd ]]; then echo "  * LDDs will be benchmarked on LTSmin's non-overapproximated partial rels"; fi
if [[ $bench_overapprox && $bench_ldd ]]; then echo "  * Also benchmarking on LTSmin's overapproximated (-2r,r2+,w2+) LDDs"; fi
if [[ $bench_beem_vn || $bench_beem_f || $bench_ptri_vn || $bench_ptri_f || $bench_prom_vn || $bench_prom_f ]]; then
    echo "  * WARNING: this script hasn't been updated to handle vanilla or force DDs with all other params of this script"
    exit 1
fi


echo "timeout per run is $maxtime"
echo "writing .csv output files to folder $outputfolder"
if [[ $confirm == true ]]; then
    read -p "Press enter to start"
fi

mkdir -p $outputfolder


# BEEM, Sloan BDDs
if [[ $bench_beem_sl && $bench_bdd ]]; then
    for filename in models/beem/bdds/sloan/*.bdd; do
        for nw in $num_workers; do
            if [[ $test_bfs ]]; then
                timeout $maxtime ./$bddmc $filename --workers=$nw --strategy=bfs --merge-relations --count-nodes $deadlocks --statsfile=$beem_sl_stats
            fi
            if [[ $test_sat == true ]]; then
                timeout $maxtime ./$bddmc $filename --workers=$nw --strategy=sat --count-nodes $deadlocks --statsfile=$beem_sl_stats
            fi
            if [[ $test_reach == true ]]; then
                timeout $maxtime ./$bddmc $filename --workers=$nw --strategy=rec --merge-relations --count-nodes $deadlocks --statsfile=$beem_sl_stats
            fi
            if [[ $test_par ]]; then
                timeout $maxtime ./$bddmc $filename --workers=$nw --strategy=rec --loop-order=par --merge-relations --count-nodes $deadlocks --statsfile=$beem_sl_stats
            fi
        done
    done
fi

# Petri nets, Sloan BDDs
if [[ $bench_ptri_sl && $bench_bdd ]]; then
    for filename in models/petrinets/bdds/sloan/*.bdd; do
        for nw in $num_workers; do
            if [[ $test_bfs ]]; then
                timeout $maxtime ./$bddmc $filename --workers=$nw --strategy=bfs --merge-relations --count-nodes $deadlocks --statsfile=$petri_sl_stats
            fi
            if [[ $test_sat == true ]]; then
                timeout $maxtime ./$bddmc $filename --workers=$nw --strategy=sat --count-nodes $deadlocks --statsfile=$petri_sl_stats
            fi
            if [[ $test_reach == true ]]; then
                timeout $maxtime ./$bddmc $filename --workers=$nw --strategy=rec --merge-relations --count-nodes $deadlocks --statsfile=$petri_sl_stats
            fi
            if [[ $test_par ]]; then
                timeout $maxtime ./$bddmc $filename --workers=$nw --strategy=rec --loop-order=par --merge-relations --count-nodes $deadlocks --statsfile=$petri_sl_stats
            fi
        done
    done
fi

# Petri nets, Sloan static BDDs
if [[ $bench_ptri_sl && $bench_bdd_static ]]; then
    for filename in models/petrinets/static_bdds/sloan/*.bdd; do
        for nw in $num_workers; do
            timeout $maxtime ./$bddmc $filename --workers=$nw --strategy=rec --merge-relations --count-nodes --statsfile=$petri_sl_stats_bdd_static
        done
    done
fi

# Promela, Sloan BDDs
if [[ $bench_prom_sl && $bench_bdd ]]; then
    for filename in models/promela/bdds/sloan/*.bdd; do
        for nw in $num_workers; do
            if [[ $test_bfs ]]; then
                timeout $maxtime ./$bddmc $filename --workers=$nw --strategy=bfs --merge-relations --count-nodes $deadlocks --statsfile=$promela_sl_stats
            fi
            if [[ $test_sat == true ]]; then
                timeout $maxtime ./$bddmc $filename --workers=$nw --strategy=sat --count-nodes $deadlocks --statsfile=$promela_sl_stats
            fi
            if [[ $test_reach == true ]]; then
                timeout $maxtime ./$bddmc $filename --workers=$nw --strategy=rec --merge-relations --count-nodes $deadlocks --statsfile=$promela_sl_stats
            fi
            if [[ $test_par ]]; then
                timeout $maxtime ./$bddmc $filename --workers=$nw --strategy=rec --loop-order=par --merge-relations --count-nodes $deadlocks --statsfile=$promela_sl_stats
            fi
        done
    done
fi

# Awari (global BDD relations, predefined variable order)
if [[ $bench_awari && $bench_bdd ]]; then
    for filename in models/awari/*.bdd; do
        for nw in $num_workers; do
            if [[ $test_bfs ]]; then
                timeout $maxtime ./$bddmc $filename --workers=$nw --strategy=bfs --merge-relations --count-nodes $deadlocks --statsfile=$awari_stats
            fi
            timeout $maxtime ./$bddmc $filename --workers=$nw --strategy=rec --merge-relations $deadlocks --count-nodes --statsfile=$awari_stats
            if [[ $test_par ]]; then
                timeout $maxtime ./$bddmc $filename --workers=$nw --strategy=rec --loop-order=par --merge-relations --count-nodes $deadlocks --statsfile=$awari_stats
            fi
        done
    done
fi

# BEEM, Sloan LDDs (merging relation requires either overapproximated LDDs or --custom-image)
if [[ $bench_beem_sl && $bench_ldd ]]; then
    for filename in models/beem/ldds/sloan/*.ldd; do
        for nw in $num_workers; do
            if [[ $test_bfs ]]; then
                timeout $maxtime ./$lddmc $filename --workers=$nw --strategy=bfs --merge-relations --custom-image --count-nodes --statsfile=$beem_sl_stats_ldd
            fi
            timeout $maxtime ./$lddmc $filename --workers=$nw --strategy=sat --count-nodes --statsfile=$beem_sl_stats_ldd
            timeout $maxtime ./$lddmc $filename --workers=$nw --strategy=rec --merge-relations --custom-image --count-nodes --statsfile=$beem_sl_stats_ldd
            if [[ $bench_overapprox ]]; then
                timeout $maxtime ./$lddmc models/beem/ldds/sloan/overapprox/$(basename $filename) --workers=$nw --strategy=rec --merge-relations --count-nodes --statsfile=$beem_sl_stats_ldd
            fi
        done
    done
fi

# Petri nets, Sloan LDDs (merging relation requires either overapproximated LDDs or --custom-image)
if [[ $bench_ptri_sl && $bench_ldd ]]; then
    for filename in models/petrinets/ldds/sloan/*.ldd; do
        for nw in $num_workers; do
            if [[ $test_bfs ]]; then
                timeout $maxtime ./$lddmc $filename --workers=$nw --strategy=bfs --merge-relations --custom-image --count-nodes --statsfile=$petri_sl_stats_ldd
            fi
            timeout $maxtime ./$lddmc $filename --workers=$nw --strategy=sat --count-nodes --statsfile=$petri_sl_stats_ldd
            timeout $maxtime ./$lddmc $filename --workers=$nw --strategy=rec --merge-relations --custom-image --count-nodes --statsfile=$petri_sl_stats_ldd
            if [[ $bench_overapprox ]]; then
                timeout $maxtime ./$lddmc models/petrinets/ldds/sloan/overapprox/$(basename $filename) --workers=$nw --strategy=rec --merge-relations --count-nodes --statsfile=$petri_sl_stats_ldd
            fi
        done
    done
fi

# Petri nets, Sloan static LDDs
if [[ $bench_ptri_sl && $bench_ldd_static ]]; then
    mkdir -p $outputfolder/logs/
    for filename in models/petrinets/static_ldds/sloan/*.ldd; do
        for nw in $num_workers; do
            fname=$(basename $filename)
            log_file=$outputfolder/logs/${fname%.*}_w${nw}_rec_ldd.log
            (time timeout $maxtime ./$lddmc $filename --workers=$nw --strategy=rec --merge-relations --custom-image --count-nodes --statsfile=$petri_sl_stats_ldd_static) |& tee $log_file
            #timeout $maxtime ./$lddmc $filename --workers=$nw --strategy=bfs-plain --merge-relations --custom-image --count-nodes --statsfile=$petri_sl_stats_ldd_static
        done
    done
fi

# Promela, Sloan LDDs (merging relation requires either overapproximated LDDs or --custom-image)
if [[ $bench_prom_sl && $bench_ldd ]]; then
    #for filename in models/promela/ldds/sloan/overapprox/*.ldd; do
    for filename in models/promela/ldds/sloan/*.ldd; do
        for nw in $num_workers; do
            if [[ $test_bfs ]]; then
                timeout $maxtime ./$lddmc $filename --workers=$nw --strategy=bfs --merge-relations --custom-image --count-nodes --statsfile=$promela_sl_stats_ldd
            fi
            timeout $maxtime ./$lddmc $filename --workers=$nw --strategy=sat --count-nodes --statsfile=$promela_sl_stats_ldd
            timeout $maxtime ./$lddmc $filename --workers=$nw --strategy=rec --merge-relations --custom-image --count-nodes --statsfile=$promela_sl_stats_ldd
            if [[ $bench_overapprox ]]; then
                timeout $maxtime ./$lddmc models/promela/ldds/sloan/overapprox/$(basename $filename) --workers=$nw --strategy=rec --merge-relations --count-nodes --statsfile=$promela_sl_stats_ldd
            fi
        done
    done
fi
