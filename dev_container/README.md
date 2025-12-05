# NEURON dev containers

_"It works on my machine." "Then we'll ship your machine!"_

This is a best-effort attempt at making NEURON development easier by using [containers](https://en.wikipedia.org/wiki/Containerization_(computing)). The point is to ship all the necessary (or as much of it possible) software that can be used in the NEURON development process.


## Basic workflow

1. install Docker or Podman
2. build an image by running the ``create_image.sh`` script. You can specify the name of the script by passing an argument to the script; the default image name is ``neuron_container``
3. create a persistent Docker volume which will be used for storing the various caches (currently Python packages and the compiler). This only needs to be done once. You can specify the name of the volume by passing an argument to the script; the default volume name is ``cache_volume``
4. (*required if using GPU capabilities*) create a persistent Docker volume which will be used for storing the NVHPC SDK. You can specify the name of the volume by passing an argument to the script; the default volume name is ``nvhpc_volume``
5. start the container by running the ``run_container_standard.sh`` script. Alternatively, run the ``run_container_nvhpc.sh`` script if you want to use GPU capabilities. The rest of the instructions will assume you are now inside of the container
6. install all of the NEURON Python dependencies: ``uv pip install -r ci/requirements.txt``. The first time may take a bit longer to do this, but any subsequent runs will be almost instant due to caching
7. build NEURON in any way you want. Note that the env variable ``CMAKE_C_COMPILER_LAUNCHER`` and ``CMAKE_CXX_COMPILER_LAUNCHER`` are set to ``ccache`` so all runs will be cached (inside of the persistent volume made in step 3)


## Limitations

* some versions of NVHPC (at least 25.3) do not currently work with ``NRN_ENABLE_BACKTRACE=ON`` due to a mismatching compiler. This should not present a big problem, since NMODL, the primary usecase for enabling that option in the first place, does not use any GPU components at runtime (though it *can* emit GPU-specific code via the code printer)
