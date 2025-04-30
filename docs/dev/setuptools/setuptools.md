# Building NEURON wheels

## pyproject.toml

 NEURON has several Python extensions
* HOC module (setuptools Extension with our CMake sauce on top)
* three Rx3D extensions (Cython extensions)
* MUSIC (Cython extension)

Since version 9, these extensions are all built using CMake itself. However, when building a Python wheel, we use a build tool compatible with the modern Python standards, notably, PEP 517, and use [`scikit-build-core`](https://scikit-build-core.readthedocs.io/) as a thin wrapper around CMake.

Furthermore, we use [`cibuildwheel`](https://cibuildwheel.pypa.io/) to build redistributable wheels on Linux and MacOS.

## Build details

Here is roughly how `scikit-build-core` builds a NEURON wheel:

1. It parses the `pyproject.toml` file, and reports any errors in the configuration
2. It creates a virtual environment, and installs all of the Python **build dependencies** there
3. It creates a build directory, and calls CMake from that build directory, while adding some custom CMake variables of its own (for a complete list, see [here](https://scikit-build-core.readthedocs.io/en/latest/guide/cmakelists.html#accessing-information))
4. Once the CMake build finishes, it installs the package in another directory
5. Finally, it creates a `.whl` file which contains the NEURON installation, wheel metadata (author information, compatible Python versions, runtime dependencies, etc.), and any additional data (such as scripts) that should be distributed, but is not part of the regular CMake build

`cibuildwheel` adds the following to the above procedure:

### Before the build

- If building on Linux, it starts up a Docker container, and copies the project to the container. We use the [`neuronsimulator/neuron_wheel`](https://hub.docker.com/r/neuronsimulator/neuron_wheel) image for building the wheel, which is based on `manylinux_2_28`.
- If building on MacOS, it checks that an [official MacOS Python installation](https://www.python.org/downloads/macos/) is available on the machine, and uses that as the Python version.

### After the build

In order to make a redistributable wheel, we need to set the correct [rpath](https://en.wikipedia.org/wiki/Rpath) for the libraries and executables, as well as copy any libraries to the wheel that would not be found on the user's system at run-time. `cibuildwheel` does this using [`auditwheel`](https://github.com/pypa/auditwheel/) on Linux, and [`delocate`](https://pypi.org/project/delocate/) on MacOS.

## Creating a Development Python Package

To see how this works in action, you can clone the git repository, and then run:

```sh
python -m pip wheel --no-deps .
```

from the top-level directory of the repository. This will create a wheel called `neuron_nightly-[version]-[platform_specifier].whl` in your current directory, which can then be installed via `python -m pip install [filename]`. Note that any non-Python dependencies (such as bison, MPI, etc.) must be installed beforehand.

A simple wrapper for the above is available at `packaging/python/build_wheels.bash`. The wrapper can be run in two modes: in `pip` mode, and in `cibuildwheel` mode.

To use the `pip` mode, run it via:

```sh
packaging/python/build_wheels.bash CI /path/to/python/interp
```

This will place the wheel in the `wheelhouse` subdirectory.

To use the `cibuildwheel` mode, run it via:

```sh
packaging/python/build_wheels.bash [platform] 39
```

where `[platform]` is one of (linux, Linux, osx, Darwin). The above will create a redistributable wheel for your platform. To build wheels for all Python versions, use `'3*'` (note the quotation marks). To build wheels for only selected platforms, use a space-separated list of versions surrounded by quotation marks, without any dots between the major and minor version (for example, `'39 310'`).

Note that, for the `cibuildwheel` mode, on Linux you must have either Docker or podman installed (by default, `cibuildwheel` uses Docker, and to use podman one must set the environmental variable `CIBW_CONTAINER_ENGINE=podman`), and on MacOS you must have the official MacOS Python installation for that particular Python version.

## Customizing the wheel

The built wheel can be customized using either environmental variables which have the same names as their CMake define counterparts, or using `pip`s `--config-settings` flag. For instance, if one wants to enable coreNEURON support for the wheel, one can run:

```sh
export NRN_ENABLE_CORENEURON=ON
bash packaging/python/build_wheels.bash linux 39
```

The equivalent using `pip` is:

```sh
python -m pip wheel --no-deps . --config-settings=cmake.define.NRN_ENABLE_CORENEURON=ON
```

For multiple options, one can add additional `--config-settings` flags. For a list of default values for building a wheel, have a look at the `tool.scikit-build.cmake.define` section of the `pyproject.toml` file. Note that the `build_wheels.bash` script uses `cibuildwheel` behind the scenes, and overrides some of the variables (as the script is used in the CI); for a list of those, have a look under the `tool.cibuildwheel.linux.environment` and `tool.cibuildwheel.macos.environment` sections of the `pyproject.toml` file (for Linux and MacOS, respectively).

The build files are by default in placed in `build_wheel`, however one can customize this using `--config-settings=build-dir=/some/build/dir`.

### Testing

Once built, the NEURON Python package may be imported and used normally. You might, however, need to set up
`PYTHONPATH` accordingly for the import to work:

```
export PYTHONPATH="<NRNDIR>/build_wheel/lib/python:$PYTHONPATH"

# Run Neuron base tests
python -c "import neuron; neuron.test()"
```

Note that if installing the (locally built) wheel, changing the `PYTHONPATH` is not necessary.
