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

##### Using setuptools

Finally, we can also build the project using setuptools:

```
python3 setup.py install --prefix=<pathInstallationDir>
```

This will install nmodl on your system, run all the tests and generate the documentation.

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
>>> driver = nmodl.NmodlDriver()
>>> driver.parse_string("NEURON { SUFFIX hh }")
True
>>> modast = driver.ast()
>>> print ('%.100s' % modast)
{"Program":[{"NeuronBlock":[{"StatementBlock":[{"Suffix":[{"Name":[{"String":[{"name":"SUFFIX"}]}]},
```

NMODL is now setup correctly!

#### Using Python API

The best way to understand the API and usage is using Jupyter notebooks provided in docs directory :

```
cd nmodl/docs/notebooks
jupyter notebook
```

You can look at [nmodl-python-tutorial.ipynb](docs/notebooks/nmodl-python-tutorial.ipynb) notebook for python interface tutorial. There is also [nmodl-python-sympy-examples.ipynb](docs/notebooks/nmodl-python-sympy-examples.ipynb)showing how [SymPy](https://www.sympy.org/en/index.html) is used in NMODL.

##### Documentation

If you installed nmodl using setuptools as shown in the previous section, you will have all the documentation generated on build/sphinx.

Otherwise, if you have already installed nmodl and setup the correct PYTHONPATH, you can build the documentation locally from the docs/ folder.

```
cd docs
make html
```

Or using setuptools

```
python3 setup.py install_doc
```

If you dont want to change the PYTHONPATH, make sure that you have the shared library on the nmodl/ folder. It is copied there automatically by compiling and running the tests:

```
python3 setup.py test
```

Now you will have a new folder docs/_build/html or build/sphinx/html, depending if you run the first or the second command,
where you can open the index.html with your favourite browser.

Another option is to create an httpServer on this folder and open the browser on localhost:

```
cd docs/_build/html
python2 -m SimpleHTTPServer 8080
http://localhost:8080
```

To check the coverage of your documentation you can run:

```
cd nmodl/docs
make coverage
```

The results will be stored on the docs/_build/coverage

To run the code snippets on the documentation you can do:

```
cd nmodl/docs
make doctest
```

Or with setuptools

```
python3 setup.py doctest
```

The output will be stored on the docs/_build/doctest or build/sphinx/doctest depending on the command used.

To add new modules you could call sphinx-apidoc.

```
sphinx-apidoc -o docs/ nmodl/
```

This will generate "stubs" rst files for the new modules.
The file will look like something like this:

```
.. automodule:: mymodule
   :members:
   :no-undoc-members:
   :show-inheritance:
```

If you want to generate documentation for all the symbols of the module, you could add :imported-members:

```
.. automodule:: mymodule
   :members:
   :imported-members:
   :no-undoc-members:
   :show-inheritance:
```

After that you can run the "make html" command to generate documentation for the new modules.

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

Main code generation program is `nmodl`. You can see all sub-commands supported using:

```
$ ./bin/nmodl -h

NMODL : Source-to-Source Code Generation Framework
Usage: ./bin/nmodl [OPTIONS] file... [SUBCOMMAND]

Positionals:
  file TEXT:FILE ... REQUIRED           One or more MOD files to process

Options:
  -h,--help                             Print this help message and exit
  -H,--help-all                         Print this help message including all sub-commands
  -v,--verbose                          Verbose logger output
  -o,--output TEXT=.                    Directory for backend code output
  --scratch TEXT=tmp                    Directory for intermediate code output

Subcommands:
  host                                  HOST/CPU code backends
  acc                                   Accelerator code backends
  sympy                                 SymPy based analysis and optimizations
  passes                                Analyse/Optimization passes
  codegen                               Code generation options
```

To see all command line options from every sub-command, you can do:

```
$ ./bin/nmodl -H

NMODL : Source-to-Source Code Generation Framework
Usage: ./bin/nmodl [OPTIONS] file... [SUBCOMMAND]

Positionals:
  file TEXT:FILE ... REQUIRED           One or more MOD files to process

Options:
  -h,--help                             Print this help message and exit
  -H,--help-all                         Print this help message including all sub-commands
  -v,--verbose                          Verbose logger output
  -o,--output TEXT=.                    Directory for backend code output
  --scratch TEXT=tmp                    Directory for intermediate code output

Subcommands:
host
  HOST/CPU code backends
  Options:
    --c                                   C/C++ backend
    --omp                                 C/C++ backend with OpenMP

acc
  Accelerator code backends
  Options:
    --oacc                                C/C++ backend with OpenACC
    --cuda                                C/C++ backend with CUDA

sympy
  SymPy based analysis and optimizations
  Options:
    --analytic                            Solve ODEs using SymPy analytic integration
    --pade                                Pade approximation in SymPy analytic integration
    --cse                                 CSE (Common Subexpression Elimination) in SymPy analytic integration
    --conductance                         Add CONDUCTANCE keyword in BREAKPOINT

passes
  Analyse/Optimization passes
  Options:
    --inline                              Perform inlining at NMODL level
    --localize                            Convert RANGE variables to LOCAL
    --localize-verbatim                   Convert RANGE variables to LOCAL even if verbatim block exist
    --local-rename                        Rename LOCAL variable if variable of same name exist in global scope
    --verbatim-inline                     Inline even if verbatim block exist
    --verbatim-rename                     Rename variables in verbatim block
    --json-ast                            Write AST to JSON file
    --nmodl-ast                           Write AST to NMODL file
    --json-perf                           Write performance statistics to JSON file
    --show-symtab                         Write symbol table to stdout

codegen
  Code generation options
  Options:
    --layout TEXT:{aos,soa}=soa           Memory layout for code generation
    --datatype TEXT:{float,double}=soa    Data type for floating point variables
```

To use code generation capability you can do:

```
$ nmodl <path>/hh.mod \
		host --c \
		sympy --analytic --pade --conductance --cse \
		passes --inline --localize --localize-verbatim \
		--local-rename --verbatim-inline --verbatim-rename
```

This will generate hh.cpp in the current directory.

##### Lexer and Parser

The `nmodl_parser` is a standalone parsing tool for NMODL that one can use to check if NMODL construct is valid or if it can be correctly parsed by NMODL. You can parse a mod file as:

```
$ nmodl_parser <path>/file.mod
```

Or, pass NMODL construct on the command line as:

```
$ nmodl_parser --text "NEURON{ SUFFIX hh }"

[NMODL] [info] :: Processing text : NEURON{ SUFFIX hh }
```

The `nmodl_lexer` is a standaline lexer tool for NMODL. You can test a mod file as:

```
$ nmodl_lexer <path>/file.mod
```

Or, pass NMODL construct on the command line as:

```
$ nmodl_lexer --text "NEURON{ SUFFIX hh }"

[NMODL] [info] :: Processing text : NEURON{ SUFFIX hh }
         NEURON at [1.1-6] type 332
              { at [1.7] type 368
         SUFFIX at [1.9-14] type 358
             hh at [1.16-17] type 356
              } at [1.19] type 369
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

Note that the latest master has changed command line option. Use released version **`0.1`** for now.


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
