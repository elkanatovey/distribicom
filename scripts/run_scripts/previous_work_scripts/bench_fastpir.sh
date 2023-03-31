#!/bin/bash

#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=128

#SBATCH --nodes=1


#SBATCH --time=04:50:00
#SBATCH --mem=256G

for i in {1..1024}
do
/cs/labs/yossigi/elkanatovey/CLionProjects/distribicom/bin/fastpir -n 1024 -s 256 -q $i -t 12
done

for i in {1..1024}
do
/cs/labs/yossigi/elkanatovey/CLionProjects/distribicom/bin/fastpir -n 2048 -s 256 -q $i -t 12
done

for i in {1..1024}
do
/cs/labs/yossigi/elkanatovey/CLionProjects/distribicom/bin/fastpir -n 4096 -s 256 -q $i -t 12
done

for i in {1..1024}
do
/cs/labs/yossigi/elkanatovey/CLionProjects/distribicom/bin/fastpir -n 8192 -s 256 -q $i -t 12
done

for i in {1..1024}
do
/cs/labs/yossigi/elkanatovey/CLionProjects/distribicom/bin/fastpir -n 16384 -s 256 -q $i -t 12
done

for i in {1..1024}
do
/cs/labs/yossigi/elkanatovey/CLionProjects/distribicom/bin/fastpir -n 32768 -s 256 -q $i -t 12
done

for i in {1..1024}
do
/cs/labs/yossigi/elkanatovey/CLionProjects/distribicom/bin/fastpir -n 65536 -s 256 -q $i -t 12
done

for i in {1..1024}
do
/cs/labs/yossigi/elkanatovey/CLionProjects/distribicom/bin/fastpir -n 131072 -s 256 -q $i -t 12
done

for i in {1..1024}
do
/cs/labs/yossigi/elkanatovey/CLionProjects/distribicom/bin/fastpir -n 262144 -s 256 -q $i -t 12
done

for i in {1..1024}
do
/cs/labs/yossigi/elkanatovey/CLionProjects/distribicom/bin/fastpir -n 524288 -s 256 -q $i -t 12
done

for i in {1..1024}
do
/cs/labs/yossigi/elkanatovey/CLionProjects/distribicom/bin/fastpir -n 1048576 -s 256 -q $i -t 12
done