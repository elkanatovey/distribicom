#!/bin/bash

#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=2
#SBATCH --nodes=5
#SBATCH --time=00:10:00
#SBATCH --mem=64G


#SBATCH hetjob
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=4
#SBATCH --nodes=1
#SBATCH --mem=64G



srun -n 5 python3 run_worker.py location_configs.json pir_configs.json 2 : -n 1 python3 run_server.py location_configs.json pir_configs.json 30 5 12

srun -n 5 hostname : -n 1 hostname
