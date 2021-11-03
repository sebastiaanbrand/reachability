#!/bin/bash

# (re)compile Sylvan
cd sylvan
rm -r -f build
mkdir -p build
cd build
cmake ..
make
cd ..
cd ..

# (re)compile reachability code
cd reach_algs
rm -r -f build
mkdir -p build
cd build
cmake ..
make
cd ..
cd ..
