#!/bin/bash

beem_selection="adding.1.bdd 
                blocks.2.bdd"

for filename in $beem_selection; do
    timeout 10m ./../build/distbdd-spin17/main models/beem/$filename -a -t 25 -s 0
    timeout 10m ./../build/distbdd-spin17/main models/beem/$filename -a -t 25 -s 1
    timeout 10m ./../build/distbdd-spin17/main models/beem/$filename -a -t 25 -s 2
done
