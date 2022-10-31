#!/bin/bash

# the below script installs all library dependencies of distribicom other than sealpir to $HOME/.distribicom_installs.
# If you wish to preinstall these libraries and use them to compile distribicom, than run the script and then call
# cmake with the below flags:
# #-DUSE_PREINSTALLED_GRPC=ON -DUSE_PRECOMPILED_SEAL=ON -D CMAKE_PREFIX_PATH=${HOME}/.distribicom_installs

# If you wish to change the install location of the librariesthan just change ${HOME}/.distribicom_installs to your
# preferred location in every place in this file and proceed according to the above instructions.

export MY_INSTALL_DIR=$HOME/.distribicom_installs
mkdir -p $MY_INSTALL_DIR
export PATH="$MY_INSTALL_DIR/bin:$PATH"

# install grpc
git clone --recurse-submodules -b v1.50.1 --depth 1 --shallow-submodules https://github.com/grpc/grpc
cd grpc
mkdir -p cmake/build
pushd cmake/build
cmake -DgRPC_INSTALL=ON \
      -DgRPC_BUILD_TESTS=OFF \
      -DCMAKE_INSTALL_PREFIX=$MY_INSTALL_DIR \
      ../..
make -j4
make install
popd

cd ..
rm -rf grpc

# install seal
git clone --recurse-submodules -b v4.0.0 --depth 1 --shallow-submodules https://github.com/microsoft/SEAL
cd SEAL
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=$MY_INSTALL_DIR -DSEAL_THROW_ON_TRANSPARENT_CIPHERTEXT=OFF -DSEAL_USE_INTEL_HEXL=ON
cmake --build build
cmake --install build
cd ..
rm -rf SEAL
