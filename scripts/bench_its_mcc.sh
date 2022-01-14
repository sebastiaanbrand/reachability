#!/bin/bash

# script to run reachability benchmarks the ITSTools MCC'21 submission
# (run from root of repo, i.e. outside of scripts/ folder)

# usage: bash scripts/bench_its_mcc.sh

maxtime=10m

its_mcc_folder=../ITS-Tools-MCC

outputfolder_deadlocks=bench_data/its_tools/petrinets_deadlocks
mkdir -p $outputfolder_deadlocks


# some info before starting benchmarks
echo "timeout per run is $maxtime"
read -p "Press enter to start"

cd $its_mcc_folder

for filename in mcc2016oracle/*RD.out ; do 
    outfile=../reachability/$outputfolder_deadlocks/$(basename ${filename::-7}).out
    echo "benchmarking $filename"
    timeout $maxtime ./run_test.pl $filename &> $outfile
done


