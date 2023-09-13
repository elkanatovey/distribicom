#!/bin/bash

cd /sci/nosnap/yossigi/mcboomy/distribicom

module load singularity
singularity exec --bind /cs/labs/yossigi/elkanatovey/.distribicom_installs \
                 --bind /sci/nosnap/yossigi/mcboomy/distribicom \
                 /cs/labs/yossigi/elkanatovey/images/ubuntu.simg \
                  cmake -S . -B build \
                  -D CMAKE_PREFIX_PATH=/cs/labs/yossigi/elkanatovey/.distribicom_installs \
                   -D USE_PRECOMPILED_SEAL=ON -D USE_PREINSTALLED_GRPC=ON
                   
singularity exec --bind /cs/labs/yossigi/elkanatovey/.distribicom_installs \
                 --bind /sci/nosnap/yossigi/mcboomy/distribicom \
                 /cs/labs/yossigi/elkanatovey/images/ubuntu.simg \
                  cmake --build /sci/nosnap/yossigi/mcboomy/distribicom/build --target all -j 12


