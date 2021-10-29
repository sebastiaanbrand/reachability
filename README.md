# Decision diagram operation for reachability

## 1. Dependencies
(Docker container already has relevant dependencies installed )

## 2. Compilation
Run `./compile_sources.sh` to (re)compile 

## 3. Reproducing experiments
From the `reachability/` folder:
* Full benchmarks:
    * To run the single core experiments, run `./bench_all.sh bdd ldd beem-sloan petri-sloan promela-sloan` and press enter.
    * To run the 1-8 core benchmarks, run `./bench_all.sh -w "1 2 4 8" test-par bdd ldd beem-sloan petri-sloan promela-sloan` and press enter.
    * To run the 16-96 core experiments, run `./bench_all.sh -w "16 32 64 96" -t 30m test-par bdd ldd beem-sloan petri-sloan promela-sloan`
* Quickly run benchmarks on random subsets of smaller instances:
    * For single core experiments, run `./bench_subset.sh -n 10` and press enter to run (for each dataset) a random selection of 10 small instances.
    * For 1-8 core experiments, run `TODO` and press enter.

To plot the results run `TODO`
