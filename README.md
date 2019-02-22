## NMODL
> NEURON MOdeling Language Code Generation Framework

NMODL is a code generation framework for [NEURON Modeling Language](https://www.neuron.yale.edu/neuron/static/py_doc/modelspec/programmatic/mechanisms/nmodl.html). The main goals of this framework are :

* Support for full NMODL specification
* Providing modular tools for parsing, analysis and optimization
* High level python interface
* Optimised code generation for modern CPU/GPU architectures
* Code generation backends compatible with existing simulators
* Flexibility to implement new simulator backend with minimal efforts

It is primarily designed to support optimised code generation backends but the underlying infrastructure can be used with high level python interface for model introspection and analysis.

### Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes.

#### Cloning Source

This project uses git submodules which must be cloned along with the repository itself:

```
git clone --recursive git@github.com:BlueBrain/nmodl.git
```

#### Prerequisites

To build the project from source, modern C++ compiler with c++11 support is necessary. Make sure you have following packages available:

- flex (>=2.6)
- bison (>=3.0)
- CMake (>=3.1)
- Python (>=3.6)
- Python packages : jinja2 (>=2.10), pyyaml (>=3.13), pytest (>=4.0.0), sympy (>=1.2), textwrap

##### On OS X

Often we have older version of flex and bison. We can get recent version of dependencies via brew/macport and pip:

```
brew install flex bison cmake python3
pip3 install Jinja2 PyYAML pytest sympy
```

Make sure to have latest flex/bison in $PATH :

```
export PATH=/usr/local/opt/flex:/usr/local/opt/bison:/usr/local/bin/:$PATH
```

##### On Ubuntu

On Ubuntu (>=16.04) flex/bison versions are recent enough. We can install Python3 and dependencies using:

```
apt-get install python3 python3-pip
pip3 install Jinja2 PyYAML pytest sympy
```

##### On BB5

On Blue Brain 5 system, we can load following modules :

```
module load cmake/3.12.0 bison/3.0.5 flex/2.6.3 gcc/6.4.0 python3-dev
```

#### Build Project

##### Using CMake

Once all dependencies are in place, build project as:

```
mkdir -p nmodl/build
cd nmodl/build
cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/nmodl
make -j && make install
```

And set PYTHONPATH as:

```
export PYTHONPATH=$HOME/nmodl/lib/python:$PYTHONPATH
```

##### Using pip

Alternatively, we can build the project using pip as:

```
pip3 install nmodl/. --target=$HOME/nmodl   # remove --target option if want to install 
```

And if --target option was used, set PYTHONPATH as:

```
export PYTHONPATH=$HOME/nmodl:$PYTHONPATH
```

#### Testing Installed Module

If you install NMODL using CMake, you can run tests from build directory as:

```
$ make test
Running tests...
Test project /home/kumbhar/nmodl/build
    Start 1: ModToken
1/7 Test #1: ModToken .........................   Passed    0.01 sec
...
```

We can use nmodl module from python as:

```
$ python3
>>> import nmodl.dsl as nmodl
>>> driver = nmodl.Driver()
>>> driver.parse_string("NEURON { SUFFIX hh }")
True
>>> modast = driver.ast()
>>> print ('%.100s' % modast)
{"Program":[{"NeuronBlock":[{"StatementBlock":[{"Suffix":[{"Name":[{"String":[{"name":"SUFFIX"}]}]},
```

NMODL is now setup correctly!

#### Understand Python API

The user documentation for NMODL is incomplete and not available on GitHub yet. The best way to understand the API and usage is using Jupyter notebooks provided in docs directory :

```
cd nmodl/doc/notebooks
jupyter notebook
```

You can look at [nmodl-python-tutorial.ipynb](doc/notebooks/nmodl-python-tutorial.ipynb) notebook for python interface tutorial. There is also [nmodl-python-sympy-examples.ipynb](doc/notebooks/nmodl-python-sympy-examples.ipynb)showing how [SymPy](https://www.sympy.org/en/index.html) is used in NMODL.


#### Using NMODL For Code Generation

Once you install project using CMake, you will have following binaries in the installation directory:

```
$ tree $HOME/nmodl/bin

|-- lexer
|   |-- c_lexer
|   `-- nmodl_lexer
|-- nmodl
|-- parser
|   |-- c_parser
|   `-- nmodl_parser
`-- visitor
    `-- nmodl_visitor
```

The `nmodl_lexer` and `nmodl_parser` are standalone tools for testing mod files. If you want to test if given mod file can be successfully parsed by NMODL then you can do:

```
nmodl_parser --file <path>/hh.mod
```

To see how NMODL will generate the code for given mod file, you can do:

```
nmodl <path>/hh.mod
```

This will generate hh.cpp in the current directory. There are different optimisation options for code generation that you can see using:

```
nmodl --help
```


#### Using NMODL With CoreNEURON

We can use NMODL instead of MOD2 for CoreNEURON. You have to simply use extra CMake argument `-DMOD2C` pointing to `nmodl` binary:

```
git clone --recursive https://github.com/BlueBrain/CoreNeuron.git coreneuron
mkdir coreneuron/build && cd coreneuron/build
cmake .. -DMOD2C=<path>/bin/nmodl
make -j
make test
```

> Note that the code generation backend is not complete yet.


### Development Conventions

If you are developing NMODL, make sure to enable both `NMODL_FORMATTING` and `NMODL_PRECOMMIT`
CMake variables to ensure that your contributions follow the coding conventions of this project:

```cmake
cmake -DNMODL_FORMATTING:BOOL=ON -DNMODL_PRECOMMIT:BOOL=ON <path>
```

The first variable provides the following additional targets to format
C, C++, and CMake files:

```
make clang-format cmake-format
```

The second option activates Git hooks that will discard commits that
do not comply with coding conventions of this project. These 2 CMake variables require additional utilities:

* [ClangFormat 7](https://releases.llvm.org/7.0.0/tools/clang/docs/ClangFormat.html)
* [cmake-format](https://github.com/cheshirekow/cmake_format) Python package
* [pre-commit](https://pre-commit.com/) Python package

_ClangFormat_ can be installed on Linux thanks
to [LLVM apt page](http://apt.llvm.org/). On MacOS, there is a
[brew recipe](https://gist.github.com/ffeu/0460bb1349fa7e4ab4c459a6192cbb25)
to install clang-format 7. _cmake-format_ and _pre-commit_ utilities can be installed with *pip*.


##### Memory Leaks and Clang Tidy

If you want to test for memory leaks, do :

```
valgrind --leak-check=full --track-origins=yes  ./bin/nmodl_lexer
```

Or using CTest as:

```
ctest -T memcheck
```

If you want to enable `clang-tidy` checks with CMake, make sure to have `CMake >= 3.5` and use following cmake option:

```
cmake .. -DENABLE_CLANG_TIDY=ON
```

##### Flex / Bison Paths

If flex / bison is not in default $PATH, you can provide path to cmake as:

```
cmake .. -DFLEX_EXECUTABLE=/usr/local/opt/flex/bin/flex \
         -DBISON_EXECUTABLE=/usr/local/opt/bison/bin/bison \
         -DCMAKE_INSTALL_PREFIX=$HOME/nmodl
```
