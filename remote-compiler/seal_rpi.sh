
# If you wish to change the install location of the librariesthan just change ${HOME}/.distribicom_installs to your
# preferred location in every place in this file and proceed according to the above instructions.

export CC=/usr/bin/clang
export CXX=/usr/bin/clang++

export MY_INSTALL_DIR=$HOME/.distribicom_installs
mkdir -p $MY_INSTALL_DIR
export PATH="$MY_INSTALL_DIR/bin:$PATH"

# install seal
git clone --recurse-submodules -b v4.0.0 --depth 1 --shallow-submodules https://github.com/microsoft/SEAL
cd SEAL
cmake -S . -B build -DSEAL_THROW_ON_TRANSPARENT_CIPHERTEXT=OFF
cmake --build build
cmake --install build
cd ..
rm -rf SEAL
