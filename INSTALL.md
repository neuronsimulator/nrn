# Installing the NMODL Framework

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes.

## Cloning Source

The NMODL Framework is maintained on github. The best way to get the sources is to simply clone the repository.

**Note**: This project uses git submodules which must be cloned along with the repository itself:

```sh
git clone --recursive https://github.com/BlueBrain/nmodl.git
cd nmodl
```

## Prerequisites

To build the project from source, a modern C++ compiler with C++14 support is necessary. Make sure you have following packages available:

- flex (>=2.6)
- bison (>=3.0)
- CMake (>=3.15)
- Python (>=3.7)
- Python packages : jinja2 (>=2.10), pyyaml (>=3.13), pytest (>=4.0.0), sympy (>=1.3), textwrap

### On OS X

Typically the versions of bison and flex provided by the system are outdated and not compatible with our requirements.
To get recent version of all dependencies we recommend using [homebrew](https://brew.sh/):

```sh
brew install flex bison cmake python3
```

The necessary Python packages can then easily be added using the pip3 command.

```sh
pip3 install Jinja2 PyYAML pytest sympy
```

Make sure to have latest flex/bison in $PATH :

```sh
export PATH=/usr/local/opt/flex/bin:/usr/local/opt/bison/bin:/usr/local/bin/:$PATH
```

On Apple M1, corresponding brew paths are under `/opt/homebrew/opt/`:

```sh
export PATH=/opt/homebrew/opt/flex/bin:/opt/homebrew/opt/bison/bin:$PATH
```

### On Ubuntu

On Ubuntu (>=18.04) flex/bison versions are recent enough and are installed along with the system toolchain:

```sh
apt-get install flex bison gcc python3 python3-pip
```

The Python dependencies are installed using:

```sh
pip3 install Jinja2 PyYAML pytest sympy
```

## Build Project

### Using CMake

Once all dependencies are in place, build project as:

```sh
mkdir -p nmodl/build
cd nmodl/build
cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/nmodl
make -j && make install
```

And set PYTHONPATH as:

```sh
export PYTHONPATH=$HOME/nmodl/lib:$PYTHONPATH
```

#### Flex / Bison Paths

If flex / bison are not in your default $PATH, you can provide the path to cmake as:

```sh
cmake .. -DFLEX_EXECUTABLE=/usr/local/opt/flex/bin/flex \
         -DBISON_EXECUTABLE=/usr/local/opt/bison/bin/bison \
         -DCMAKE_INSTALL_PREFIX=$HOME/nmodl
```

### Using Python setuptools

If you are mainly interested in the NMODL Framework parsing and analysis tools and wish to use them from Python, we
recommend building and installing using Python.

```sh
pip3 install --user .
```

This should build the NMODL framework and install it into your pip user `site-packages` folder such that it becomes
available as a Python module.

### When building without linking against libpython

NMODL uses an embedded python to symbolically evaluate differential equations. For this to work we would usually link
against libpython, which is automatically taken care of by pybind11. In some cases, for instance when building a
python wheel, we cannot link against libpython, because we cannot know where it will be at runtime. Instead, we load
the python library (along with a wrapper library that manages calls to embedded python) at runtime.
To disable linking against python and enabling dynamic loading of libpython at runtime we need to configure the build 
with the cmake option `-DLINK_AGAINST_PYTHON=False`.

In order for NMODL binaries to know where to find libpython and our own libpywrapper two environment variables need to
be present:

* `NMODL_PYLIB`: This variable should point to the libpython shared-object (or dylib) file. On macos this could be
for example:
````sh
export NMODL_PYLIB=/usr/local/Cellar/python/3.7.7/Frameworks/Python.framework/Versions/3.7/Python
````
* 'NMODL_WRAPLIB': This variable should point to the `libpywrapper.so` built as part of NMODL, for example:
```sh
export NMODL_WRAPLIB=/opt/nmodl/lib/libpywrapper.so
```

**Note**: In order for all unit tests to function correctly when building without linking against libpython we must
set `NMODL_PYLIB` before running cmake!


## Testing the Installed Module

If you have installed the NMODL Framework using CMake, you can now run tests from the build directory as:

```bash
$ make test
Running tests...
Test project /Users/kumbhar/workarena/repos/bbp/incubator/nocmodl/cmake-build-debug
      Start  1: testmodtoken/NMODL Lexer returning valid ModToken object
 1/60 Test  #1: testmodtoken/NMODL Lexer returning valid ModToken object ...................................   Passed    0.01 sec
      Start  2: testlexer/NMODL Lexer returning valid token types
 2/60 Test  #2: testlexer/NMODL Lexer returning valid token types ..........................................   Passed    0.00 sec
      Start  3: testparser/Scenario: NMODL can define macros using DEFINE keyword
 3/60 Test  #3: testparser/Scenario: NMODL can define macros using DEFINE keyword ..........................   Passed    0.01 sec
      Start  4: testparser/Scenario: Macros can be used anywhere in the mod file
 4/60 Test  #4: testparser/Scenario: Macros can be used anywhere in the mod file ...........................   Passed    0.01 sec
      Start  5: testparser/Scenario: NMODL parser accepts empty unit specification
 5/60 Test  #5: testparser/Scenario: NMODL parser accepts empty unit specification .........................   Passed    0.01 sec
      Start  6: testparser/Scenario: NMODL parser running number of valid NMODL constructs
 6/60 Test  #6: testparser/Scenario: NMODL parser running number of valid NMODL constructs .................   Passed    0.04 sec
      Start  7: testparser/Scenario: NMODL parser running number of invalid NMODL constructs
 7/60 Test  #7: testparser/Scenario: NMODL parser running number of invalid NMODL constructs ...............   Passed    0.01 sec
      Start  8: testparser/Scenario: Legacy differential equation solver from NEURON solve number of ODE
 8/60 Test  #8: testparser/Scenario: Legacy differential equation solver from NEURON solve number of ODE ...   Passed    0.00 sec
 ...
```

To test the NMODL Framework python bindings, you can try a minimal example in your Python 3 interpeter as follows:

```python
>>> import nmodl.dsl as nmodl
>>> driver = nmodl.NmodlDriver()
>>> modast = driver.parse_string("NEURON { SUFFIX hh }")
>>> print ('%s' % modast)
{"Program":[{"NeuronBlock":[{"StatementBlock":[{"Suffix":[{"Name":[{"String":[{"name":"SUFFIX"}]}]},{"Name":[{"String":[{"name":"hh"}]}]}]}]}]}]}
>>> print (nmodl.to_nmodl(modast))
NEURON {
    SUFFIX hh
}
```

NMODL is now setup correctly!


## Generating Documentation

In order to build the documentation you must have additionally `pandoc` installed. Use your
system's package manager to do this (e.g. `sudo apt-get install pandoc`).

You can build the entire documentation simply by using sphinx from `setup.py`:

```sh
python3 setup.py build_ext --inplace docs -G "Unix Makefiles"
```
