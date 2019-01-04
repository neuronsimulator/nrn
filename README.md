## NMODL

This is prototype implementation of code generation framework for NMODL.

#### Cloning Source

```
git clone ssh://bbpcode.epfl.ch:22/incubator/nocmodl
```

#### Dependencies

- flex (>=2.6)
- bison (>=3.0)
- CMake (>=3.1)
- C++ compiler (with c++11 support)
- Python2/3 (>=2.7)
- Python yaml (pyyaml)

Make sure to have latest version of flex (>=2.6) and bison (>=3.0).


On OS X we typically install packages via brew or macport as:

```
brew install flex bison
```

This will install flex and bison in:

```
/usr/local/opt/flex
/usr/local/opt/bison
```

On Ubuntu (>=16.04) you should already have recent version of flex/bison:

```
$ flex --version
flex 2.6.4

$ bison --version
bison (GNU Bison) 3.0.4
```

Python yaml can be installed on Ubuntu using:

```
sudo apt-get install python-yaml
```

On OS X:

```
pip install pyyaml
pip3 install pyyaml

```


#### Build
Build/Compile NMODL as:

```
mkdir build
cd build
cmake ..
make -j
```

If flex / bison is installed in non-standard location then set `PATH` env variable to have latest flex/bison in `PATH` or use `CMAKE_PREFIX_PATH`:

```
 cmake .. -DCMAKE_PREFIX_PATH="/usr/local/opt/bison/;/usr/local/opt/flex"
```

On BB5, you can do:

```
module load cmake/3.12.0 bison/3.0.5 flex/2.6.3 gcc/6.4.0

mkdir build && cd build
cmake ..
make VERBOSE=1 -j
make test
```

If you want to enable `clang-tidy` checks with CMake, make sure to have `CMake >= 3.5` and use following cmake option:

```
cmake .. -DENABLE_CLANG_TIDY=ON
```


#### Running NMODL

You can independently run lexer, parser or visitors as:

```
./bin/nmodl_lexer --file ../test/input/channel.mod
./bin/nmodl_parser --file ../test/input/channel.mod
./bin/nmodl_visitor --file ../test/input/channel.mod
```

To run code generator, you can do:

```
./bin/nmodl ../test/input/channel.mod
```


#### Running Test

 You can run unit tests as:

```
 make test
```

 Or individual test binaries with verbode output:

 ```
 ./bin/test/testlexer -s
 ./bin/test/testmodtoken -s
 ./bin/test/testprinter -s
 ./bin/test/testsymtab -s
 ./bin/test/testvisitor -s
 ```


#### Memory Leaks

Test memory leaks using :

```
valgrind --leak-check=full --track-origins=yes  ./bin/nmodl_lexer
```

Or using CTest as:

```
ctest -T memcheck
```
