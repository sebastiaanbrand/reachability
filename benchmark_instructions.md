
```
bench_data/
├─ for_paper/
|  ├─ reach-vs-its/
|  |  ├─ reach/
|  |  ├─ its/
```


### REACH vs saturation
```
TODO
```


### Parallel benchmarks
```
TODO
```


### REACH vs ITS-tools
(1) run benchmarks, (2) aggregate benchmark data, (3) plot (NOTE: paths for the plots are hardcoded atm)
```
$ [run bench for reach]
$ python scripts/aggregate_pnml-encode_time.py bench_data/for_paper/reach-vs-its/reach/
$ python scripts/generate_plots.py its
```


Running and processing ITS benchmarks:
```
NOTE: currently using bench_itstools.sh + aggregate_its_data.py, 
TODO: replace with bench_its_mcc.sh + aggregate_mcc_data.py
```

