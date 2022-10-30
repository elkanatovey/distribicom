#!/bin/bash

export MY_INSTALL_DIR=$HOME/.distribicom_installs
mkdir -p $MY_INSTALL_DIR
export PATH="$MY_INSTALL_DIR/bin:$PATH"

# install grpc
git clone --recurse-submodules -b v1.48.0 --depth 1 --shallow-submodules https://github.com/grpc/grpc
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

git clone --recurse-submodules -b v4.0.0 --depth 1 --shallow-submodules https://github.com/microsoft/SEAL
cd SEAL
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=$MY_INSTALL_DIR
cmake --build build
cmake --install build
cd ..
rm -rf SEAL

