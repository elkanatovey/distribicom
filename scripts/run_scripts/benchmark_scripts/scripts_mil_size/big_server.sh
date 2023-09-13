#!/bin/bash

#SBATCH --ntasks-per-node=50
#SBATCH --cpus-per-task=2
#SBATCH --nodes=17
#SBATCH --time=04:10:00
#SBATCH --mem=800G


#SBATCH hetjob
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=24
#SBATCH --nodes=1
#SBATCH --mem=800G



srun -n 492 python3 run_worker.py location_configs.json pir_configs.json 2 : -n 1 python3 run_server.py location_configs.json pir_configs.json 492 492 12

srun -n 656 python3 run_worker.py location_configs.json pir_configs.json 2 : -n 1 python3 run_server.py location_configs.json pir_configs.json 656 656 12

srun -n 820 python3 run_worker.py location_configs.json pir_configs.json 2 : -n 1 python3 run_server.py location_configs.json pir_configs.json 820 820 12



srun -n 5 hostname : -n 1 hostname
