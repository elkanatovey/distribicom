#!/bin/bash

#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=24


#SBATCH --nodes=1


#SBATCH --time=01:50:00
#SBATCH --mem=128G

/cs/labs/yossigi/elkanatovey/CLionProjects/Addra/execs/worker -s 256 -m 65536 -a 32 -r 10 -w 1  -l 127.0.0.1 -c 127.0.0.1 -t 10 -i 0

/cs/labs/yossigi/elkanatovey/CLionProjects/Addra/execs/worker -s 256 -m 65536 -a 64 -r 10 -w 1  -l 127.0.0.1 -c 127.0.0.1 -t 10 -i 0


/cs/labs/yossigi/elkanatovey/CLionProjects/Addra/execs/worker -s 256 -m 65536 -a 128 -r 10 -w 1  -l 127.0.0.1 -c 127.0.0.1 -t 10 -i 0


/cs/labs/yossigi/elkanatovey/CLionProjects/Addra/execs/worker -s 256 -m 65536 -a 256 -r 10 -w 1  -l 127.0.0.1 -c 127.0.0.1 -t 10 -i 0


/cs/labs/yossigi/elkanatovey/CLionProjects/Addra/execs/worker -s 256 -m 65536 -a 512 -r 10 -w 1  -l 127.0.0.1 -c 127.0.0.1 -t 10 -i 0



/cs/labs/yossigi/elkanatovey/CLionProjects/Addra/execs/worker -s 256 -m 65536 -a 1024 -r 10 -w 1  -l 127.0.0.1 -c 127.0.0.1 -t 10 -i 0
echo "sixth size finished"
