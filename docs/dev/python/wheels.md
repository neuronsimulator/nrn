# Building NEURON wheels

## pyproject.toml

NEURON has several Python extensions:

* the HOC module
* three Rx3D extensions (Cython extensions)
* MUSIC (Cython extension)

Since version 9, these extensions are all built using CMake itself. However, when building a Python wheel, we use a build tool compatible with the modern Python standards, notably, PEP 517, and use [`scikit-build-core`](https://scikit-build-core.readthedocs.io/) as a thin wrapper around CMake.

Furthermore, we use [`cibuildwheel`](https://cibuildwheel.pypa.io/) to build redistributable wheels on Linux and MacOS.

## Build details

Here is a rough outline of how the build procedure works for NEURON wheels. It is composed of two parts, a _build frontend_, and a _build backend_.
In our case, the build frontend is `pip`, and the build backend is `scikit-build-core`.

### Build frontend

A Python build frontend is in charge of the following tasks:

1. It parses the `pyproject.toml` file, and reports any errors in the configuration
2. It creates an isolated virtual environment, and installs all of the Python **build dependencies** there
3. It parses any command-line options (which could also override options in `pyproject.toml`), and passes them to the build backend

### Build backend

A Python build backend is in charge of the following tasks:

1. It creates a build directory, and calls CMake from that build directory, while adding some custom CMake variables of its own (for a complete list, see [here](https://scikit-build-core.readthedocs.io/en/latest/guide/cmakelists.html#accessing-information))
2. Once the CMake build finishes, it installs the package in another directory
3. Finally, it creates a `.whl` file which contains the NEURON installation, wheel metadata (author information, compatible Python versions, runtime dependencies, etc.), and any additional data (such as scripts) that should be distributed, but is not part of the regular CMake build

## Distributable wheels

`cibuildwheel` is a tool for making distributable Python wheels. In addition to the above, it adds the following steps:

### Before the build

- If building on Linux, it starts up a Docker container, and copies the project to the container. We use the [`neuronsimulator/neuron_wheel`](https://hub.docker.com/r/neuronsimulator/neuron_wheel) image for building the wheel, which is based on `manylinux_2_28`.
- If building on MacOS, it checks that an [official MacOS Python installation](https://www.python.org/downloads/macos/) is available on the machine, and uses that as the Python version.

### After the build

In order to make a redistributable wheel, we need to set the correct [rpath](https://en.wikipedia.org/wiki/Rpath) for the libraries and executables, as well as copy any libraries to the wheel that would not be found on the user's system at run-time. `cibuildwheel` does this using [`auditwheel`](https://github.com/pypa/auditwheel/) on Linux, and [`delocate`](https://pypi.org/project/delocate/) on MacOS.

### Directory layout

The directory layout used by NEURON wheels is slightly different from the one used by an ordinary CMake install. This is controlled by two internal CMake variables, `NRN_INSTALL_PYTHON_PREFIX`, and `NRN_INSTALL_DATA_PREFIX`.

When building a wheel, `NRN_INSTALL_PYTHON_PREFIX` is set to `neuron`, and all of the Python-related files (the above mentioned modules + any pure Python ones) are placed there, while `NRN_INSTALL_DATA_PREFIX` is set to `neuron/.data`, and everything else (for instance, HOC scripts, NEURON libraries and executables, etc.) is installed there, under its corresponding subdirectories (`lib`, `bin`, etc.).

On the other hand, when not building a wheel, `NRN_INSTALL_PYTHON_PREFIX` is set to `lib/python`, while `NRN_INSTALL_DATA_PREFIX` is left empty, so all of the files are installed in their standard locations, prepended by `CMAKE_INSTALL_PREFIX`.

## Creating a Development Python Package

**NOTE**: it is recommended to update `pip` to a recent version by running `python -m pip install --upgrade pip` before running any of the below `pip` commands.

To see how all of the above works in action, you can clone the git repository, and then run `pip` via:

```sh
python -m pip wheel --no-deps .
```

from the top-level directory of the repository. This will create a wheel called `neuron_nightly-[version]-[platform_specifier].whl` in your current directory, which can then be installed via `python -m pip install [filename]`. Note that any non-Python dependencies (such as bison, MPI, etc.) must be installed beforehand.

A simple wrapper for the above is available at `packaging/python/build_wheels.bash`. The wrapper can be run in two modes: in `pip` mode, and in `cibuildwheel` mode.

To use the `pip` mode, run it via:

```sh
packaging/python/build_wheels.bash CI [python_interp]
```

where `[python_interp]` should be the full path to a working Python interpreter. This will create a wheel in the `wheelhouse` subdirectory.

To use the `cibuildwheel` mode, run it via:

```sh
packaging/python/build_wheels.bash [platform] [python_version(s)]
```

where `[platform]` is one of {linux, Linux, osx, Darwin}, and `[python_version(s)]` is a list of Python versions for which to build the wheel. The above will create a redistributable wheel under the `wheelhouse` subdirectory. To build wheels for all Python versions, use `'3*'` (note the quotation marks). To build wheels for only selected versions, use a space-separated list of versions surrounded by quotation marks, without any dots between the major and minor version (for example, `'39 310'`).

Note that, for the `cibuildwheel` mode, on Linux you must have either Docker or podman installed (by default, `cibuildwheel` uses Docker, and to use podman one must set the environmental variable `CIBW_CONTAINER_ENGINE=podman`), and on MacOS you must have the official MacOS Python installation for that particular Python version.

## Customizing the wheel

The built wheel can be customized using either environmental variables which have the same names as their CMake define counterparts, or using `pip`s `--config-settings` flag. For instance, if you want to enable coreNEURON support for the wheel, you can run the wrapper using:

```sh
export NRN_ENABLE_CORENEURON=ON
bash packaging/python/build_wheels.bash linux 39
```

The equivalent using `pip` is:

```sh
python -m pip wheel --no-deps . --config-settings=cmake.define.NRN_ENABLE_CORENEURON=ON
```

For multiple options, one can add additional `--config-settings` flags. For a list of default values for building a wheel, have a look at the `tool.scikit-build.cmake.define` section of the `pyproject.toml` file. Note that the `build_wheels.bash` script uses `cibuildwheel` behind the scenes, and overrides some of the variables (as the script is used in the CI); for a list of those, have a look under the `tool.cibuildwheel.linux.environment` and `tool.cibuildwheel.macos.environment` sections of the `pyproject.toml` file (for Linux and MacOS, respectively).

The build files are by default placed in `build_wheel`, however one can customize this using `--config-settings=build-dir=/some/build/dir`.

### Testing

Once built, the NEURON Python package may be imported and used normally. You might, however, need to set up
`PYTHONPATH` accordingly for the import to work:

```
export PYTHONPATH="<NRNDIR>/build_wheel/lib/python:$PYTHONPATH"

# Run Neuron base tests
python -c "import neuron; neuron.test()"
```

Note that if installing the (locally built) wheel, changing the `PYTHONPATH` is not necessary.
