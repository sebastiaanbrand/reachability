#!/bin/bash

beem_selection="adding.1.ldd
                anderson.2.ldd
                anderson.4.ldd
                at.1.ldd
                at.2.ldd
                bakery.1.ldd
                bakery.2.ldd
                bakery.3.ldd
                blocks.2.ldd
                bopdp.1.ldd
                bridge.1.ldd
                bridge.2.ldd
                bridge.3.ldd
                brp2.1.ldd
                cambridge.1.ldd
                cambridge.2.ldd
                cyclic_scheduler.1.ldd
                cyclic_scheduler.2.ldd
                cyclic_scheduler.3.ldd
                cyclic_scheduler.4.ldd
                driving_phils.1.ldd
                driving_phils.2.ldd
                elevator.1.ldd
                elevator.2.ldd
                exit.1.ldd
                exit.2.ldd
                extinction.1.ldd
                extinction.2.ldd
                firewire_link.1.ldd
                firewire_link.2.ldd
                firewire_link.4.ldd
                fischer.1.ldd
                fischer.2.ldd
                gear.1.ldd
                gear.2.ldd
                hanoi.1.ldd
                iprotocol.1.ldd
                iprotocol.2.ldd
                krebs.1.ldd
                lamport.1.ldd
                lamport.3.ldd
                lann.1.ldd
                lann.2.ldd
                lifts.1.ldd
                lifts.2.ldd
                lup.1.ldd
                lup.2.ldd
                mcs.1.ldd
                mcs.2.ldd
                msmie.1.ldd
                msmie.2.ldd
                msmie.3.ldd
                needham.1.ldd
                needham.2.ldd
                peg_solitaire.1.ldd
                peterson.1.ldd
                phils.1.ldd
                phils.2.ldd
                pouring.1.ldd
                pouring.2.ldd
                protocols.1.ldd
                protocols.2.ldd
                public_subscribe.1.ldd
                reader_writer.1.ldd
                reader_writer.2.ldd
                reader_writer.3.ldd
                rushhour.1.ldd
                schedule_world.1.ldd
                szymanski.1.ldd
                szymanski.2.ldd
                telephony.1.ldd"

beem_vn_stats="bench_data/selection/beem_vanilla_stats_ldd.csv"
beem_ga_stats="bench_data/selection/beem_ga_stats_ldd.csv"

mkdir -p bench_data/selection

maxtime=5m

for filename in $beem_selection; do
    # LTSmin generated ldd files without any options for regrouping (other than to extend relations to full domain)
    timeout $maxtime ./../build/reachability/lddmc models/beem/ldds_vanilla/overapprox/$filename --workers=1 --strategy=bfs --merge-relations --count-nodes --statsfile=$beem_vn_stats
    timeout $maxtime ./../build/reachability/lddmc models/beem/ldds_vanilla/$filename --workers=1 --strategy=sat --count-nodes --statsfile=$beem_vn_stats
    timeout $maxtime ./../build/reachability/lddmc models/beem/ldds_vanilla/overapprox/$filename --workers=1 --strategy=rec --merge-relations --count-nodes --statsfile=$beem_vn_stats
done

