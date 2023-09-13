#!/bin/bash

#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=16
#SBATCH --nodes=8
#SBATCH --time=00:10:00
#SBATCH --mem=64G

srun -n 8 run.py -c test_setting.json -s 8 --cpus 16

srun -n1 -c16 --mem-per-cpu=1gb server : -n16 --mem-per-cpu=1gb client
