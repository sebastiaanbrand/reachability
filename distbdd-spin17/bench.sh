#!/bin/bash

for filename in models/beem/*.bdd; do
    timeout 5m ./../build/distbdd-spin17/main $filename -a
    timeout 5m ./../build/distbdd-spin17/main $filename -a -s 1
    timeout 5m ./../build/distbdd-spin17/main $filename -a -s 2
done
