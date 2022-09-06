#!/bin/bash

# usage:
# bash bench_subset.sh [-w workers] [-t maxtime] [-n amount_per_dataset]

num_workers=1
maxtime=10s
amount=10

bddmc=reach_algs/build/bddmc
lddmc=reach_algs/build/lddmc

while getopts "w:t:n:" opt; do
  case $opt in
    w) num_workers="$OPTARG"
    ;;
    t) maxtime="$OPTARG"
    ;;
    n) amount="$OPTARG"
    ;;
    \?) echo "Invalid option -$OPTARG" >&1; exit 1;
    ;;
  esac
done

test_sat=true
test_reach=true
test_bdd=true
test_ldd=true

for var in "$@"; do
  if [[ $var == 'test-par' ]]; then test_par=true; fi
  if [[ $var == 'test-par-only' ]]; then test_par=true; test_reach=false; fi
  if [[ $var == 'bfs' ]]; then test_bfs=true; fi
  if [[ $var == 'deadlocks' ]]; then deadlocks="--deadlocks"; fi
done

if [[ $test_par ]]; then
  test_ldd=false
fi

echo "Running following benchmarks with [$num_workers] workers:"
if [[ $test_bdd == true ]]; then echo "  - BEEM BDDs Sloan ($amount random small instances)"; fi
if [[ $test_ldd == true ]]; then echo "  - BEEM LDDs Sloan ($amount random small instances)"; fi
if [[ $test_bdd == true ]]; then echo "  - Petri Nets BDDs Sloan ($amount random small instances)"; fi
if [[ $test_ldd == true ]]; then echo "  - Petri Nets LDDs Sloan ($amount random small instances)"; fi
if [[ $test_bdd == true ]]; then echo "  - Promela BDDs Sloan ($amount random small instances)"; fi
if [[ $test_ldd == true ]]; then echo "  - Promela LDDs Sloan ($amount random small instances)"; fi
if [[ $test_par ]]; then echo "  * Testing parallelism for BDD rec-reach"; fi
if [[ $test_reach == false ]]; then echo "    (only testing par version of reach)"; fi
if [[ $deadlocks ]]; then echo "  * Testing for deadlocks for BDD algs"; fi

# output folder
if [[ $num_workers == 1 ]]; then
  outputfolder=bench_data/subset/single_worker
elif [[ $num_workers == "1 2 4 8" || $num_workers == "1 8" ]]; then
  outputfolder=bench_data/subset/par_8
elif [[ $num_workers == "1 16 32 64 96" ]]; then
  outputfolder=bench_data/subset/par_96
elif [[ $num_workers == "1 64" ]]; then
  outputfolder=bench_data/subset/par_64
else
  outputfolder=bench_data/subset/par
fi
mkdir -p $outputfolder

if [[ $deadlocks ]]; then
    filename_app="_deadlocks"
fi

echo "timeout per run is $maxtime"
echo "writing .csv output files to folder $outputfolder"
read -p "Press enter to start"

# output files
beem_sl_stats="$outputfolder/beem_sloan_stats_bdd$filename_app.csv"
petri_sl_stats="$outputfolder/petrinets_sloan_stats_bdd$filename_app.csv"
promela_sl_stats="$outputfolder/promela_sloan_stats_bdd$filename_app.csv"
beem_sl_stats_ldd="$outputfolder/beem_sloan_stats_ldd.csv"
petri_sl_stats_ldd="$outputfolder/petrinets_sloan_stats_ldd.csv"
promela_sl_stats_ldd="$outputfolder/promela_sloan_stats_ldd.csv"


if [[ $test_bdd == true ]]; then

# BEEM, Sloan BDDs
shuf -n $amount models/beem/small_bdd.txt | while read filename; do
  filepath=models/beem/bdds/sloan/$filename
  for nw in $num_workers; do
      if [[ $test_bfs ]]; then
        timeout $maxtime ./$bddmc $filepath --workers=$nw --strategy=bfs --merge-relations --count-nodes $deadlocks --statsfile=$beem_sl_stats
      fi
      if [[ $test_sat == true ]]; then
        timeout $maxtime ./$bddmc $filepath --workers=$nw --strategy=sat --count-nodes $deadlocks --statsfile=$beem_sl_stats
      fi
      if [[ $test_reach == true ]]; then
        timeout $maxtime ./$bddmc $filepath --workers=$nw --strategy=rec --merge-relations --count-nodes $deadlocks --statsfile=$beem_sl_stats
      fi
      if [[ $test_par ]]; then
          timeout $maxtime ./$bddmc $filepath --workers=$nw --strategy=rec --loop-order=par --merge-relations --count-nodes $deadlocks --statsfile=$beem_sl_stats
      fi
  done
done

# Petri nets, Sloan BDDs
shuf -n $amount models/petrinets/small_bdd.txt | while read filename; do
  filepath=models/petrinets/bdds/sloan/$filename
  for nw in $num_workers; do
      if [[ $test_bfs ]]; then
        timeout $maxtime ./$bddmc $filepath --workers=$nw --strategy=bfs --merge-relations --count-nodes $deadlocks --statsfile=$petri_sl_stats
      fi
      if [[ $test_sat == true ]]; then
        timeout $maxtime ./$bddmc $filepath --workers=$nw --strategy=sat --count-nodes $deadlocks --statsfile=$petri_sl_stats
      fi
      if [[ $test_reach == true ]]; then
        timeout $maxtime ./$bddmc $filepath --workers=$nw --strategy=rec --merge-relations --count-nodes $deadlocks --statsfile=$petri_sl_stats
      fi
      if [[ $test_par ]]; then
          timeout $maxtime ./$bddmc $filepath --workers=$nw --strategy=rec --loop-order=par --merge-relations --count-nodes $deadlocks --statsfile=$petri_sl_stats
      fi
  done
done

# Promela, Sloan BDDs
shuf -n $amount models/promela/small_bdd.txt | while read filename; do
  filepath=models/promela/bdds/sloan/$filename
  for nw in $num_workers; do
      if [[ $test_bfs ]]; then
        timeout $maxtime ./$bddmc $filepath --workers=$nw --strategy=bfs --merge-relations --count-nodes $deadlocks --statsfile=$promela_sl_stats
      fi
      if [[ $test_sat == true ]]; then
        timeout $maxtime ./$bddmc $filepath --workers=$nw --strategy=sat --count-nodes $deadlocks --statsfile=$promela_sl_stats
      fi
      if [[ $test_reach == true ]]; then
        timeout $maxtime ./$bddmc $filepath --workers=$nw --strategy=rec --merge-relations --count-nodes $deadlocks --statsfile=$promela_sl_stats
      fi
      if [[ $test_par ]]; then
          timeout $maxtime ./$bddmc $filepath --workers=$nw --strategy=rec --loop-order=par --merge-relations --count-nodes $deadlocks --statsfile=$promela_sl_stats
      fi
  done
done

fi # $test_bdd == true




if [[ $test_ldd == true ]]; then

# BEEM, Sloan LDDs (merging relation for BFS and REC requires overapproximated LDDs)
if [[ $num_workers == 1 ]]; then
  shuf -n $amount models/beem/small_ldd.txt | while read filename; do
    filepath=models/beem/ldds/sloan/overapprox/$filename
    for nw in $num_workers; do
        if [[ $test_bfs ]]; then
          timeout $maxtime ./$lddmc $filepath --workers=$nw --strategy=bfs --merge-relations --count-nodes --statsfile=$beem_sl_stats_ldd
        fi
        timeout $maxtime ./$lddmc models/beem/ldds/sloan/$(basename $filepath) --workers=$nw --strategy=sat --count-nodes --statsfile=$beem_sl_stats_ldd
        timeout $maxtime ./$lddmc $filepath --workers=$nw --strategy=rec --merge-relations --count-nodes --statsfile=$beem_sl_stats_ldd
    done
  done
fi

# Petri nets, Sloan LDDs (merging relation for BFS and REC requires overapproximated LDDs)
if [[ $num_workers == 1 ]]; then
  shuf -n $amount models/petrinets/small_ldd.txt | while read filename; do
    filepath=models/petrinets/ldds/sloan/overapprox/$filename
    for nw in $num_workers; do
        if [[ $test_bfs ]]; then
          timeout $maxtime ./$lddmc $filepath --workers=$nw --strategy=bfs --merge-relations --count-nodes --statsfile=$petri_sl_stats_ldd
        fi
        timeout $maxtime ./$lddmc models/petrinets/ldds/sloan/$(basename $filepath) --workers=$nw --strategy=sat --count-nodes --statsfile=$petri_sl_stats_ldd
        timeout $maxtime ./$lddmc $filepath --workers=$nw --strategy=rec --merge-relations --count-nodes --statsfile=$petri_sl_stats_ldd
    done
  done
fi

# Promela, Sloan LDDs (merging relation for BFS and REC requires overapproximated LDDs)
if [[ $num_workers == 1 ]]; then
  shuf -n $amount models/promela/small_ldd.txt | while read filename; do
    filepath=models/promela/ldds/sloan/overapprox/$filename
      for nw in $num_workers; do
          if [[ $test_bfs ]]; then
            timeout $maxtime ./$lddmc $filepath --workers=$nw --strategy=bfs --merge-relations --count-nodes --statsfile=$promela_sl_stats_ldd
          fi
          timeout $maxtime ./$lddmc models/promela/ldds/sloan/$(basename $filepath) --workers=$nw --strategy=sat --count-nodes --statsfile=$promela_sl_stats_ldd
          timeout $maxtime ./$lddmc $filepath --workers=$nw --strategy=rec --merge-relations --count-nodes --statsfile=$promela_sl_stats_ldd
      done
  done
fi

fi # $test_ldd == true
