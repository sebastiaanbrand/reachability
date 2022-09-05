# Decision diagram operation for reachability

## Files
* In `reach_algs/`, `bddmc.c` and `lddmc.c` contain implementations of multiple reachability algorithms for BDDs and LDDs. The original implementations come from [here](https://github.com/trolando/sylvan/tree/master/examples), to which we have added the REACH algorithms presented in the paper. The BDD and LDD versions of REACH are internally called `go_rec` (line 847 in [bddmc.c](reach_algs/bddmc.c) and line 631 in [ldd_custom.c](reach_algs/ldd_custom.c)).
* `scripts/` contains a number of scripts for benchmarking and plotting.
* `sylvan/` contains the source of Sylvan. (For compatibility reasons we include a specific version of Sylvan rather than using an installed version.)

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

### 2. Benchmark data
The models used to benchmark can be downloaded from [here](https://surfdrive.surf.nl/files/index.php/s/W38OBT78zEZM9MN). The `models/` folder should be placed at the root of the repository folder.

* `models/` contains the models we used for benchmarking. Included are the original models, as well as `.bdd` and `.ldd` files generated by LTSmin. These `.bdd`/`.ldd` files contain decision diagrams for the transition relations and starting states and serve as the input for the reachability algorithms.

### 3. Compilation
Run `./compile_sources.sh` to (re)compile. This creates, in `reach_algs/build/`, two programs: `bddmc` and `lddmc`, which contain several reachability algorithms for BDDs and LDDs.

### 4. Running on individual models
From the root of the repository, the following runs REACH and saturation on the `adding.1.bdd` model with 1 worker/thread. Note that REACH requires `--merge-relations` to merge the partial relations.
```bash
$ ./reach_algs/build/bddmc models/beem/bdds/sloan/adding.1.bdd -s rec --merge-relations -w 1

$ ./reach_algs/build/bddmc models/beem/bdds/sloan/adding.1.bdd -s sat -w 1
```

And similarly for running REACH for the LDDs:
```bash
$ ./reach_algs/build/lddmc models/beem/ldds/sloan/adding.1.ldd -s rec --merge-relations -w 1

$ ./reach_algs/build/lddmc models/beem/ldds/sloan/adding.1.ldd -s sat -w 1
```

### 5a. Reproducing experiments (subset)

* To generate (a small subset of) the data used for Figures 4 and 5 in the paper, run the command below. This runs (for each dataset; BEEM, Petri, Promela) a random selection of 10 small instances.
```
$ ./scripts/bench_subset.sh -n 10
``` 

* To generate (a small subset of) the data used for Figure 6 in the paper, run the command below. This runs (for each dataset; BEEM, Petri, Promela) a random selection of 10 small instances.
```
$ ./scripts/bench_subset.sh -n 10 -w "1 2 4 8" test-par
```

The results of these benchmarks are written to `bench_data/subset/*/` as csv data. Running these commands multiple times appends the results of the new run to that of earlier runs.

### 5b. Reproducing experiments (full)
From the root of the repository:

* To generate the data used for Figures 4 and 5 in the paper, run 
```
$ ./scripts/bench_all.sh -t 10m bdd ldd beem-sloan petri-sloan promela-sloan
```


* To generate the data used for Figure 6 (1-8 core) in the paper, run 
```
$ ./scripts/bench_all.sh -w "1 2 4 8" test-par bdd ldd beem-sloan petri-sloan promela-sloan`
```

* To generate the data used for Figure 6 (16-96 core), run 
```
$ ./scripts/bench_all.sh -w "16 32 64 96" -t 30m test-par bdd ldd beem-sloan petri-sloan promela-sloan
```

The results of these benchmarks are written to `bench_data/all/*/` as csv data.

### 6. Generating plots
The following can be used to generate plots. The `path/to/data/` argument is the path to the data generated in step 5. (The benchmark script lets you know which folder this is.)

```bash
# Fig. 4: REACH vs saturation
$ python scripts/generate_plots.py saturation path/to/data/

# Fig. 5: Effect of locality
$ python scripts/generate_plots.py locality path/to/data/

# Fig. 6: Parallel performance of REACH vs saturation
$ python scripts/generate_plots.py parallel path/to/data/

# Fig. ?: Parallel performance scatter plots
$ python scripts/generate_plots.py parallel-scatter path/to/data/
```
