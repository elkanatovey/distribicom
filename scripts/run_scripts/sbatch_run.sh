#!/bin/bash

#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=2


#SBATCH --nodes=6


#SBATCH --time=00:01:00
#SBATCH --mem=4G

srun -n 6 run.py -c test_setting.json