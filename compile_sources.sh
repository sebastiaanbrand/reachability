#!/bin/bash

# TODO: install gmp (and other dependencies)

# Sylvan and the reachability experiments all compile as one
rm -r -f build
mkdir -p build
cd build
cmake ..
make
cd ..
