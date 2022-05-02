#!/bin/bash

# script to run reachability benchmarks with ITSTools
# (run from root of repo, i.e. outside of scripts/ folder)

# usage: bash scripts/bench_itstools.sh [beem|petri]

maxtime=10m

itstools=scripts/bin/its-reach-linux64

outputfolder_petri=bench_data/its_tools/petrinets

dt=$(date '+%Y%m%d_%H%M%S');
outputfolder_petri=$outputfolder_petri/$dt

# process arguments for running subset of all benchmarks
for var in "$@"; do
    if [[ $var == 'petri' ]]; then bench_petri=true; fi
    if [[ $var == 'beem' ]]; then bench_beem=true; fi
done

# some info before starting benchmarks
echo "timeout per run is $maxtime"
if [[ $bench_beem ]]; then
    echo "- no .gal files for DVE models yet"
fi
if [[ $bench_petri ]]; then
    echo "- benching petri; writing results to $outputfolder_petri"
fi
read -p "Press enter to start"
mkdir -p $outputfolder_petri

# Petri nets, Sloan
if [[ $bench_petri ]]; then
    for filename in models/petrinets/gal/*.gal; do
        out_file=$outputfolder_petri/$(basename $filename).out
        echo "benchmarking $filename"
        (time timeout $maxtime $itstools -i $filename -t CGAL) |& tee $out_file
    done
fi


