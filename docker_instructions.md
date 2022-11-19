# A decision diagram operation for reachability

The included Docker image `fm23_artefact_img_v1.tar` contains source code and scripts to reproduce the plots in "A decision diagram operation for reachability". This README contains instructions on how to reproduce the experiments with this Docker image.

Instructions on how to install and run the code outside of the provided Docker image can be found here: https://github.com/sebastiaanbrand/reachability

## Docker

In the following instructions commands which are supposed to be run on the host machine are prefaced with `$`, while all commands inside the docker container are prefaced with `#`. These `$` and `#` are not part of the commands.

Assuming Docker is installed, loading the Docker image and starting a container can be done with:

1. `$ sudo docker load < fm23_artefact_img_v1.tar`  (this can take a few minutes)
2. `$ docker run --name fm23_container -it fm23_artefact_img:v1.0 bash`

After loading the Docker image into a container and running this container, the code can be found in `~/reachability/`

3. `# cd ~/reachability/`

All paths and commands refered to in this file assume the current directory is `~/reachability/` inside the Docker container.


## Files
* In `reach_algs/`, `bddmc.c` and `lddmc.c + ldd_custom.c` contain implementations of multiple reachability algorithms for BDDs and LDDs. The original implementations come from [here](https://github.com/trolando/sylvan/tree/master/examples), to which we have added the REACH algorithms presented in the paper. The BDD and LDD versions of REACH are internally called `go_rec` (line 847 in `bddmc.c` and line 631 in `ldd_custom.c`.
* `scripts/` contains a number of scripts for benchmarking and plotting.
* `sylvan/` contains the source of Sylvan. (For compatibility reasons we include a specific version of Sylvan rather than using an installed version.)


## Running the code

Steps 1 and 2 concern installing dependencies and downloading the benchmark models. Both of these have already been done for the Docker image.


### 3. Compilation
Run `./compile_sources.sh` to (re)compile. This creates, in `reach_algs/build/`, two programs: `bddmc` and `lddmc`, which contain several reachability algorithms for BDDs and LDDs.

### 4. Running on individual models
From the root of the repository, the following runs REACH and saturation on the `adding.1.bdd` model with 1 worker/thread. Note that REACH requires `--merge-relations` to merge the partial relations.
```
# ./reach_algs/build/bddmc models/beem/bdds/sloan/adding.1.bdd -s rec --merge-relations -w 1

# ./reach_algs/build/bddmc models/beem/bdds/sloan/adding.1.bdd -s sat -w 1
```

And similarly for running REACH for the LDDs:
```
# ./reach_algs/build/lddmc models/beem/ldds/sloan/adding.1.ldd -s rec --merge-relations -w 1

# ./reach_algs/build/lddmc models/beem/ldds/sloan/adding.1.ldd -s sat -w 1
```

### 5. Reproducing experiments

The instructions in this README are set up to produce output data in the following directory structure.

```
bench_data/
├─ full/
|  ├─ parallel/                 # Fig. 5
|  ├─ reach-vs-its/             # Fig. 7
|  |  ├─ reach/
|  |  ├─ its/
|  ├─ reach-vs-saturation/      # Fig. 4, 6
├─ subset/
|  ├─ parallel/
|  ├─ reach-vs-saturation/
```


#### 5a. Reproducing experiments (subset)

* To generate a small subset of the data used for Figures 4 and 6 in the paper, run the command below. This runs (for each dataset; BEEM, Petri, Promela) a random selection of 10 small instances.
```
# ./scripts/bench_subset.sh -n 10 -o subset/reach-vs-saturation small
``` 

* To reproduce the data in Figure 5 in the paper, a machine with (at least) 64 cores is required. Here we provide a command to run a small scale version of this (a subset of small instances and fewer (4) cores). Note that running for 1 core is required, since the plots show "1 core vs n cores".
```
# ./scripts/bench_subset.sh -n 10 -w "1 4" -o subset/parallel test-par-only small
```

*  The current organization of different benchmark scripts makes it very difficult to run a subset of experiments for Figure 7. As of now, only instructions for running the full experiments for this figure are available.

The results of these benchmarks are written to `bench_data/subset/*/` as csv data. Running these commands multiple times appends the results of the new run to that of earlier runs.

#### 5b. Reproducing experiments (full)


* To generate the data used for Figures 4 and 6 in the paper, run 
```
# ./scripts/bench_all.sh -t 10m -o full/reach-vs-saturation bdd ldd beem-sloan petri-sloan promela-sloan
```


* To reproduce the data in Figure 5 in the paper, a machine with (at least) 64 cores is required. To generate the data, run
```
# ./scripts/bench_all.sh -t 10m -w "1 16 64" -o full/parallel test-par-only bdd beem-sloan petri-sloan promela-sloan
```


* To reproduce the data in Figure 7 in the paper, run
```
# ./scripts/bench_itstools.sh -o full/reach-vs-its/its_tools
# ./scripts/bench_all.sh -t 10m -o full/reach-vs-its/reach -t 10m ldd-static petri-sloan
```

The results of these benchmarks are written to `bench_data/full/*/` as csv data.


### 6. Generating plots
The following can be used to generate plots. Commands are given both for plotting full data and subset.

```
// Fig. 4: REACH vs saturation (full)
# python scripts/generate_plots.py saturation bench_data/full/reach-vs-saturation/

// Fig. 4: REACH vs saturation (subset)
# python scripts/generate_plots.py saturation bench_data/subset/reach-vs-saturation/

// Fig. 6: Effect of locality (full)
# python scripts/generate_plots.py locality bench_data/full/reach-vs-saturation/

// Fig. 6: Effect of locality (subset)
# python scripts/generate_plots.py locality bench_data/subset/reach-vs-saturation/

// Fig. 5: Parallel performance scatter plots (full)
# python scripts/generate_plots.py parallel-scatter bench_data/full/parallel/ 16 64

// Fig. 5: Parallel performance scatter plots (subset for 4 cores)
# python scripts/generate_plots.py parallel-scatter bench_data/subset/parallel/ 4

// Fig. 7: comparison with ITS (full)
# python scripts/aggregate_pnml-encode_time.py bench_data/full/reach-vs-its/reach/
# python scripts/aggregate_its_data.py bench_data/full/reach-vs-its/
# python scripts/generate_plots.py its bench_data/full/reach-vs-its/
```

The generated plots can be copied from the Docker container to the host machine with
```
$ sudo docker cp fm23_container:/root/reachability/plots/ plots/
```




