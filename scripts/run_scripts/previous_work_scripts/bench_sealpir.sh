#!/bin/bash

#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=128


#SBATCH --nodes=1


#SBATCH --time=01:50:00
#SBATCH --mem=128G

for i in {122..128}
do
/cs/labs/yossigi/elkanatovey/CLionProjects/distribicom/bin/run_sealpir $i $i /cs/labs/yossigi/elkanatovey/distribicom_benches/sealpir_timing_logs.txt
/cs/labs/yossigi/elkanatovey/CLionProjects/distribicom/bin/run_sealpir $i $i /cs/labs/yossigi/elkanatovey/distribicom_benches/sealpir_timing_logs.txt
/cs/labs/yossigi/elkanatovey/CLionProjects/distribicom/bin/run_sealpir $i $i /cs/labs/yossigi/elkanatovey/distribicom_benches/sealpir_timing_logs.txt
/cs/labs/yossigi/elkanatovey/CLionProjects/distribicom/bin/run_sealpir $i $i /cs/labs/yossigi/elkanatovey/distribicom_benches/sealpir_timing_logs.txt
/cs/labs/yossigi/elkanatovey/CLionProjects/distribicom/bin/run_sealpir $i $i /cs/labs/yossigi/elkanatovey/distribicom_benches/sealpir_timing_logs.txt
done

