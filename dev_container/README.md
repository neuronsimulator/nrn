# NEURON dev containers

> *"It works on my machine."*
> *"Then we'll ship your machine!"*

This is a best-effort attempt at creating a standard NEURON development environment by using [containers](https://en.wikipedia.org/wiki/Containerization_(computing)). The point is to ship all the necessary (or as much of it possible) software that can be used in the NEURON development process. The idea for this feature was inspired by [dev containers](https://containers.dev/).


## Basic workflow

1. install Docker or Podman
2. build an image by running the ``create_image.sh`` script. You can specify the name of the script by passing an argument to the script; the default image name is ``neuron_container``
3. create a persistent Docker volume which will be used for storing the various caches (currently Python packages and the compiler). This only needs to be done once. You can specify the name of the volume by passing an argument to the script; the default volume name is ``neuron_volume_cache``
4. (*required if using GPU capabilities*) create a persistent Docker volume which will be used for storing the NVHPC SDK. This only needs to be done once. You must specify the location of the downloaded ``.tar.gz`` file as the first argument to the script. You can specify the name of the volume by passing a second argument to the script; the default volume name is ``neuron_volume_nvhpc``
5. start the container by running the ``run_container_standard.sh`` script. Alternatively, run the ``run_container_nvhpc.sh`` script if you want to use GPU capabilities. The rest of the instructions will assume you are now inside of the container
6. install all of the NEURON Python dependencies: ``uv pip install -r ci/requirements.txt``. The first time may take a bit longer to do this, but any subsequent runs will be almost instant due to caching
7. build NEURON in any way you want. Note that the env variable ``CMAKE_C_COMPILER_LAUNCHER`` and ``CMAKE_CXX_COMPILER_LAUNCHER`` are set to ``ccache`` so all runs will be cached (inside of the persistent volume made in step 3)


## Limitations

* some versions of NVHPC (at least 25.3) do not currently work with ``NRN_ENABLE_BACKTRACE=ON`` due to a mismatching compiler. This should not present a big problem, since NMODL, the primary usecase for enabling that option in the first place, does not use any GPU components at runtime (though it *can* emit GPU-specific code via the code printer)
* on ARM-based MacOS, it's possible that the build of the NMODL Python bindings will fail due to out-of-memory (OOM) issues; if they are not critical to have, you can disable them using ``NMODL_ENABLE_PYTHON_BINDINGS=OFF``
