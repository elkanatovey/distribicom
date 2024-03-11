#!/bin/bash

tc qdisc add dev enp0s2 root netem delay 25ms 5ms distribution normal

cd /cs/labs/yossigi/elkanatovey/CLionProjects/distribicom/scripts/run_scripts/benchmark_scripts/scripts_mil_size
echo 'vm run start'
python3 run_worker.py location_configs.json pir_configs.json 4
echo 'vm run complete'
tc qdisc del dev enp0s2 root netem delay 25ms 5ms distribution normal