# Decision diagram operation for reachability

## Docker
Loading the Docker image and starting a container:

1. `docker load < tacas_126_docker_image.tar`
2. `docker run -it tacas_126:v1.0 bash`

After loading the Docker image into a container and running this container, the code can be found in `~/tacas_126/`

3. `cd ~/tacas_126/`

From there, the remaining instructions in this README file can be followed.

## Files
* In `reach_algs/`, `bddmc.c` and `lddmc.c` contain the implementations of our reachability algorithms for BDDs and LDDs. The original implementations come from  [here](https://github.com/trolando/sylvan/tree/master/examples), to which we have added the REACH algorithms presented in the paper.
* `models/` contains the models we used for benchmarking. Included are the original models, as well as `.bdd` and `.ldd` files generated by LTSmin. These `.bdd`/`.ldd` files contain decision diagrams for the transition relations and starting states and serve as the input for the reachability algorithms.
* `sylvan/` contains the source of Sylvan (for compatibility reasons we include a specific version of Sylvan rather than using an installed version.)

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
Run `./compile_sources.sh` to (re)compile. This creates, in `reach_algs/build/`, two programs: `bddmc` and `lddmc`, which contain several reachability algorithms for BDDs and LDDs.

### 3a. Reproducing experiments (subset)
From the `~/tacas_126/` folder:

* To generate (a small subset of) the data used for Figures 9 and 10 in the paper, run `./scripts/bench_subset.sh -n 10` and press enter. This runs (for each dataset; BEEM, Petri, Promela) a random selection of 10 small instances.
* To generate (a small subset of) the data used for Figure 11 in the paper, run `./scripts/bench_subset.sh -n 10 -w "1 2 4 8" test-par` and press enter. This runs (for each dataset; BEEM, Petri, Promela) a random selection of 10 small instances.

The results of these benchmarks are written to `bench_data/subset/` as csv data. Running these commands multiple times appends the results of the new run to that of earlier runs.

To generate plots from the generated data run `python3 scripts/generate_plots.py subset`. The plots are outputted to `plots/subset/`.

### 3b. Reproducing experiments (full)
From the `~/tacas_126/` folder:

* To generate the data used for Figures 9 and 10 in the paper, run `./scripts/bench_all.sh bdd ldd beem-sloan petri-sloan promela-sloan` and press enter.
* To generate the data used for Figure 11 (1-8 core) in the paper, run `./scripts/bench_all.sh -w "1 2 4 8" test-par bdd ldd beem-sloan petri-sloan promela-sloan` and press enter.
* To generate the data used for Figure 11 (16-96 core), run `./scripts/bench_all.sh -w "16 32 64 96" -t 30m test-par bdd ldd beem-sloan petri-sloan promela-sloan` and press enter.

To generate plots from the generated data run `python3 scripts/generate_plots.py all`. The plots are outputted to `plots/all/`.

## Important information about plots
During the preparation of this artifact we uncovered a mistake with the plots in the paper. Specifically: the text in the paper mentions that the time it takes to merge a number of partial transition relations into a single global transition relation is included in the total reachability time, whereas the plots in the paper do in fact not include this "merge time". This is primarily relevant for Figure 9 in the paper, where the runtime of saturation is compared against our REACH algorithms. 

Regarding the overhead of this "merge time", we see a trend that for larger models this overhead is relatively small compared to the actual time to compute reachability, although for some (smaller) models it can add significantly to the total time. Before any potential acceptance of the paper the plots would have to be updated. However, because of the trend in this "merge time" we believe our conclusions could remain largely the same.

Running the plotting scripts with `python3 scripts/generate_plots.py subset no-merge-time` for the subset experiments or `python3 scripts/generate_plots.py all no-merge-time` for the full experiments generates the plots as they are currently in the paper. The same commands without `no-merge-time` generate the plots including this "merge time" (as stated in the text of the paper).
