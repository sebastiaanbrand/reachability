#!/bin/bash

beem_selection="adding.1.bdd
                anderson.2.bdd
                anderson.4.bdd
                at.1.bdd
                at.2.bdd
                bakery.1.bdd
                bakery.2.bdd
                bakery.3.bdd
                blocks.2.bdd
                bopdp.1.bdd
                bridge.1.bdd
                bridge.2.bdd
                bridge.3.bdd
                brp2.1.bdd
                cambridge.1.bdd
                cambridge.2.bdd
                cyclic_scheduler.1.bdd
                cyclic_scheduler.2.bdd
                cyclic_scheduler.3.bdd
                cyclic_scheduler.4.bdd
                driving_phils.1.bdd
                driving_phils.2.bdd
                elevator.1.bdd
                elevator.2.bdd
                exit.1.bdd
                exit.2.bdd
                extinction.1.bdd
                extinction.2.bdd
                firewire_link.1.bdd
                firewire_link.2.bdd
                firewire_link.4.bdd
                fischer.1.bdd
                fischer.2.bdd
                gear.1.bdd
                gear.2.bdd
                hanoi.1.bdd
                iprotocol.1.bdd
                iprotocol.2.bdd
                krebs.1.bdd
                lamport.1.bdd
                lamport.3.bdd
                lann.1.bdd
                lann.2.bdd
                lifts.1.bdd
                lifts.2.bdd
                lup.1.bdd
                lup.2.bdd
                mcs.1.bdd
                mcs.2.bdd
                msmie.1.bdd
                msmie.2.bdd
                msmie.3.bdd
                needham.1.bdd
                needham.2.bdd
                peg_solitaire.1.bdd
                peterson.1.bdd
                phils.1.bdd
                phils.2.bdd
                pouring.1.bdd
                pouring.2.bdd
                protocols.1.bdd
                protocols.2.bdd
                public_subscribe.1.bdd
                reader_writer.1.bdd
                reader_writer.2.bdd
                reader_writer.3.bdd
                rushhour.1.bdd
                schedule_world.1.bdd
                szymanski.1.bdd
                szymanski.2.bdd
                telephony.1.bdd"

beem_vn_stats="bench_data/selection/beem_vanilla_stats.csv"
beem_ga_stats="bench_data/selection/beem_ga_stats.csv"

mkdir -p bench_data/selection

maxtime=5m

for filename in $beem_selection; do
    # LTSmin generated bdd files without any options
    timeout $maxtime ./../build/reachability/bddmc models/beem/bdds_vanilla/$filename --workers=1 --strategy=bfs --merge-relations --count-nodes --statsfile=$beem_vn_stats
    timeout $maxtime ./../build/reachability/bddmc models/beem/bdds_vanilla/$filename --workers=1 --strategy=sat --count-nodes --statsfile=$beem_vn_stats
    timeout $maxtime ./../build/reachability/bddmc models/beem/bdds_vanilla/$filename --workers=1 --strategy=rec --merge-relations --count-nodes --statsfile=$beem_vn_stats

    # LTSmin generated bdd files with -r ga option
    #timeout $maxtime ./../build/reachability/bddmc models/beem/bdds_ga/$filename --workers=1 --strategy=bfs --merge-relations --count-nodes --statsfile=$beem_ga_stats
    #timeout $maxtime ./../build/reachability/bddmc models/beem/bdds_ga/$filename --workers=1 --strategy=sat --count-nodes --statsfile=$beem_ga_stats
    #timeout $maxtime ./../build/reachability/bddmc models/beem/bdds_ga/$filename --workers=1 --strategy=rec --merge-relations --count-nodes --statsfile=$beem_ga_stats
done

