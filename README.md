# Distribicom: A communications library library that achieves low communication costs and high performance.

Distribicom is a research library and should not be used in production systems. 

-DUSE_PREINSTALLED_GRPC=ON -DUSE_PRECOMPILED_SEAL=ON options for precompiled main library use. Give compiled 
libraries path in -D CMAKE_PREFIX_PATH Note that Distibicom relies on SEAL compiled with -DSEAL_THROW_ON_TRANSPARENT_CIPHERTEXT=OFF
