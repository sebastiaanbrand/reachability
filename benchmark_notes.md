

### Folders:
```
bench_data/
├─ for_paper/
|  ├─ parallel/
|  ├─ reach-vs-its-tools/
|  ├─ reach-vs-saturation/
```

### Reach ITS-Tools

1. generate static ldds:
```
$ cd models/petrinets/
$ ./generate_static_dds.sh ldd sloan
$ cd ../../
```

2. run Reach experiments
```
./scripts/bench_all.sh -t 10m -o for_paper/reach-vs-its-tools/reach petri-sloan ldd-static
```

3. run ITS experiments
```

```


### Reach vs saturation
```
$ ./scripts/bench_all.sh -t 10m -o for_paper/reach-vs-saturation beem-sloan petri-sloan promela-sloan bdd ldd
```