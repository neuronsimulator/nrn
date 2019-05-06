### Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes.

#### Cloning Source

This project uses git submodules which must be cloned along with the repository itself:

```
git clone --recursive https://github.com/BlueBrain/nmodl.git
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

> On Blue Brain B5 Supercomputer, use : module load cmake/3.12.0 bison/3.0.5 flex/2.6.3 gcc/6.4.0 python3-dev

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

#### Testing Installed Module

If you install NMODL using CMake, you can run tests from build directory as:

```
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

We can use nmodl module from python as:

```python
$ python3
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


#### Generating Documentation

In order to build the documentation you must have additionally `pandoc` installed. Use your
system's package manager to do this (e.g. `apt install pandoc`).

Once you have installed NMODL and setup the correct PYTHONPATH, you can build the documentation locally from the docs folder as:

```
cd docs
doxygen   # for API documentation
make html # for user documentation
```


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
* [cmake-format](https://github.com/cheshirekow/cmake_format)
* [pre-commit](https://pre-commit.com/)

clang-format can be installed on Linux thanks
to [LLVM apt page](http://apt.llvm.org/). On MacOS, there is a
[brew recipe](https://gist.github.com/ffeu/0460bb1349fa7e4ab4c459a6192cbb25)
to install clang-format 7. _cmake-format_ and _pre-commit_ utilities can be installed with *pip*.


##### Memory Leaks and clang-tidy

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
