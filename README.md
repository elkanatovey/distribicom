# Distribicom: A communications library library that achieves low communication costs and high performance.

Distribicom is a research library and should not be used in production systems. 

Distribicom is a ```cmake``` project.
Set ```-DUSE_PREINSTALLED_GRPC=ON``` and  ```-DUSE_PRECOMPILED_SEAL=ON``` to use peinstalled versions of ```SEAL``` and ```GRPC```.
Give the compiled libraries path with ```-D CMAKE_PREFIX_PATH```. Note that Distibicom relies on SEAL compiled with ```-DSEAL_THROW_ON_TRANSPARENT_CIPHERTEXT=OFF```.

1. After compilation edit [`run_scripts/test_setting.json`](https://github.com/elkanatovey/distribicom/blob/de8ed7e3588a924704fb46206149d7e7fb40e8f8/scripts/run_scripts/test_setting.json) with binary locations and crypto settings
2. Run [`run_scripts/run.py`](https://github.com/elkanatovey/distribicom/blob/de8ed7e3588a924704fb46206149d7e7fb40e8f8/scripts/run_scripts/run.py) with the location of ```test_setting.json``` 
3. Then run ```main_server``` with the address of the output and ```num_queries```.
4. Then run ```worker``` with the address of the output.
