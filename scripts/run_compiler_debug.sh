#!/bin/bash

cd /cs/labs/yossigi/elkanatovey/CLionProjects/distribicom

module load singularity
singularity exec --bind /cs/labs/yossigi/elkanatovey/.distribicom_installs \
                 --bind /cs/labs/yossigi/elkanatovey/CLionProjects/distribicom \
                 /cs/labs/yossigi/elkanatovey/images/ubuntu.simg \
                  cmake -S . -B cmake-build-debug \
                  -D CMAKE_PREFIX_PATH=/cs/labs/yossigi/elkanatovey/.distribicom_installs \
                   -D USE_PRECOMPILED_SEAL=ON -D USE_PREINSTALLED_GRPC=ON -DCMAKE_BUILD_TYPE=Debug
                   
singularity exec --bind /cs/labs/yossigi/elkanatovey/.distribicom_installs \
                 --bind /cs/labs/yossigi/elkanatovey/CLionProjects/distribicom \
                 /cs/labs/yossigi/elkanatovey/images/ubuntu.simg \
                  cmake --build /cs/labs/yossigi/elkanatovey/CLionProjects/distribicom/cmake-build-debug --target all -j 4

