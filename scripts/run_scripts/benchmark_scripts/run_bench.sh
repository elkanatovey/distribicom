#!/bin/bash

#SBATCH --time=00:10:00
#SBATCH --nodes=1 --ntasks=1 --cpus-per-task=24 --mem-per-cpu=4G
#SBATCH hetjob
#SBATCH --ntasks=5 --nodes=5 --mem-per-cpu=4G

srun --het-group=0 --nodes=1 --ntasks=1 --cpus-per-task=24 --mem-per-cpu=4G python3 run_server.py location_configs.json pir_configs.json 30 5 12 : --het-group=1 --ntasks=5 --nodes=5 --mem-per-cpu=4G python3 run_worker.py location_configs.json pir_configs.json 2

srun --het-group=0 --nodes=1 --ntasks=1 --cpus-per-task=24 --mem-per-cpu=4G python3 run_server.py location_configs.json pir_configs.json 30 2 12 : --het-group=1 --ntasks=2 --nodes=2 --mem-per-cpu=4G python3 run_worker.py location_configs.json pir_configs.json 2
