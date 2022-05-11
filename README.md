# Decision diagram operation for reachability

## Files
* In `reach_algs/`, `bddmc.c` and `lddmc.c` contain the implementations of our reachability algorithms for BDDs and LDDs. The original implementations come from  [here](https://github.com/trolando/sylvan/tree/master/examples), to which we have added the REACH algorithms presented in the paper.
* `models/` contains the models we used for benchmarking. Included are the original models, as well as `.bdd` and `.ldd` files generated by LTSmin. These `.bdd`/`.ldd` files contain decision diagrams for the transition relations and starting states and serve as the input for the reachability algorithms.
* `sylvan/` contains the source of Sylvan (for compatibility reasons we include a specific version of Sylvan rather than using an installed version.)

## Running the code

### 1. Dependencies
For Ubuntu, the relevant dependencies can be installed with the following after running `sudo apt update`.

* C compilation tools (gcc, cmake): `sudo apt install cmake cmake-data build-essential`
* GMP: `sudo apt install libgmp-dev`
* Python3: `sudo apt install python3`
* Python packages:
    * pip: `sudo apt install python3-pip`
    * numpy: `sudo pip install numpy`
    * pandas: `sudo pip install pandas`
    * matplotlib: `sudo pip install matplotlib`
    * scipy: `sudo pip install scipy`

### 2. Compilation
Run `./compile_sources.sh` to (re)compile. This creates, in `reach_algs/build/`, two programs: `bddmc` and `lddmc`, which contain several reachability algorithms for BDDs and LDDs.

### 3a. Reproducing experiments (subset)
From the root of the repository:

* To generate (a small subset of) the data used for Figures 9 and 10 in the paper, run `./scripts/bench_subset.sh -n 10` and press enter. This runs (for each dataset; BEEM, Petri, Promela) a random selection of 10 small instances.
* To generate (a small subset of) the data used for Figure 11 in the paper, run `./scripts/bench_subset.sh -n 10 -w "1 2 4 8" test-par` and press enter. This runs (for each dataset; BEEM, Petri, Promela) a random selection of 10 small instances.

The results of these benchmarks are written to `bench_data/subset/` as csv data. Running these commands multiple times appends the results of the new run to that of earlier runs.

To generate plots from the generated data run `python3 scripts/generate_plots.py subset`. The plots are outputted to `plots/subset/`.

### 3b. Reproducing experiments (full)
From the root of the repository:

* To generate the data used for Figures 9 and 10 in the paper, run `./scripts/bench_all.sh bdd ldd beem-sloan petri-sloan promela-sloan` and press enter.
* To generate the data used for Figure 11 (1-8 core) in the paper, run `./scripts/bench_all.sh -w "1 2 4 8" test-par bdd ldd beem-sloan petri-sloan promela-sloan` and press enter.
* To generate the data used for Figure 11 (16-96 core), run `./scripts/bench_all.sh -w "16 32 64 96" -t 30m test-par bdd ldd beem-sloan petri-sloan promela-sloan` and press enter.

To generate plots from the generated data run `python3 scripts/generate_plots.py all`. The plots are outputted to `plots/all/`.

