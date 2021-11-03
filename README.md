# Decision diagram operation for reachability

## Files
`TODO: reorganize repo first`

## Running the code
When running the code from the provided Docker image, step 1 can be skipped, as all dependencies are already installed.

### 1. Dependencies
For Ubuntu, the relevant dependencies can be installed with the following after running `sudo apt update`.
* C compilation tools (gcc, cmake): `sudo apt install cmake cmake-data build essential`
* GMP: `sudo apt install libgmp-dev`
* Python3: `sudo apt install python3`
* Python packages:
    * pip: `sudo apt install python3-pip`
    * numpy: `sudo pip install numpy`
    * pandas: `sudo pip install pandas`
    * matplotlib: `sudo pip install matplotlib`
    * scipy: `sudo pip install scipy`

### 2. Compilation
Run `./compile_sources.sh` to (re)compile. This creates, in `reach_algs/build/`, two programs: `bddmc` and `lddmc`, which have been extended from their [original implementation](https://github.com/trolando/sylvan/tree/master/examples) to include the REACH algorithms presented in the paper.

### 3a. Reproducing experiments (subset)
From the root folder folder:
* To generate (a small subset of) the data used for Figures 9 and 10 in the paper, run `./scripts/bench_subset.sh -n 10` and press enter. This runs (for each dataset; BEEM, Petri, Promela) a random selection of 10 small instances.
* To generate (a small subset of) the data used for Figure 11 in the paper, run `./scripts/bench_subset.sh -n 10 -w "1 2 4 8" test-par` and press enter. This runs (for each dataset; BEEM, Petri, Promela) a random selection of 10 small instances.

The results of these benchmarks are written to `bench_data/subset/` as csv data. Running these commands multiple times appends the results of the new run to that of earlier runs.

To generate plots from the generated data run `python3 scripts/generate_plots.py subset`. The plots are outputted to `plots/subset/`.

### 3b. Reproducing experiments (full)
* Full benchmarks:
    * To run the single core experiments, run `./scripts/bench_all.sh bdd ldd beem-sloan petri-sloan promela-sloan` and press enter.
    * To run the 1-8 core benchmarks, run `./scripts/bench_all.sh -w "1 2 4 8" test-par bdd ldd beem-sloan petri-sloan promela-sloan` and press enter.
    * To run the 16-96 core experiments, run `./scripts/bench_all.sh -w "16 32 64 96" -t 30m test-par bdd ldd beem-sloan petri-sloan promela-sloan`
* Quickly run benchmarks on random subsets of smaller instances: