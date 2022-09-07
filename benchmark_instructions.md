
```
bench_data/
├─ for_paper/
|  ├─ parallel/
|  |  ├─ 8_cores/
|  |  ├─ 96_cores/
|  ├─ reach-vs-its/
|  |  ├─ reach/
|  |  ├─ its/
|  ├─ reach-vs-saturation/
```


## REACH vs saturation

Generate data:
```
$ ./scripts/bench_all.sh -t 10m -o for_paper/reach-vs-saturation beem-sloan petri-sloan promela-sloan bdd ldd
```

Generate plots:
```
TODO
```


## Parallel benchmarks

Generate data:
```
$ bash scripts/bench_all.sh -t 10m -w "1 8" -o for_paper/parallel/8_cores bdd beem-sloan petri-sloan promela-sloan bdd test-par-only

$ bash scripts/bench_all.sh -t 10m -w "1 8" -o for_paper/parallel/8_cores bdd beem-sloan bdd test-par-only
$ bash scripts/bench_all.sh -t 10m -w "1 8" -o for_paper/parallel/8_cores bdd petri-sloan bdd test-par-only
$ bash scripts/bench_all.sh -t 10m -w "1 8" -o for_paper/parallel/8_cores bdd promela-sloan bdd test-par-only
```

Generate plots:
```
$ python scripts/generate_plots.py parallel-scatter for_paper/parallel/8_cores/
```


## REACH vs ITS-tools
(1) run benchmarks, (2) aggregate benchmark data, (3) plot (NOTE: paths for the plots are hardcoded atm)
```
$ [run bench for reach]
$ python scripts/aggregate_pnml-encode_time.py for_paper/reach-vs-its/reach/
$ python scripts/generate_plots.py its
```


Running and processing ITS benchmarks:
```
NOTE: currently using bench_itstools.sh + aggregate_its_data.py, 
TODO: replace with bench_its_mcc.sh + aggregate_mcc_data.py
```

