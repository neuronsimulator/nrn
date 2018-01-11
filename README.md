## NOCMODL

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
- Python2 (>=2.7)
- Python yaml

Make sure to have latest version of flex (>=2.6) and bison (>=3.0). For example, on OS X we typically install packages via brew or macport as:

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

#### Build
Build/Compile NOCMODL as:

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

 On Lugano BBP IV system we have to use newer versions installed in below path:

 ```
 export PATH=/gpfs/bbp.cscs.ch/project/proj16/software/viz/hpc/bison-3.0.4-/bin:$PATH
 export PATH=/gpfs/bbp.cscs.ch/project/proj16/software/viz/hpc/flex-2.6.4/bin:$PATH
 ```

If you want to enable `clang-tidy` checks with CMake, make sure to have `CMake >= 3.5` and use following cmake option:

```
cmake .. -DENABLE_CLANG_TIDY=ON
```


#### Running NOCMODL

You can independently run lexer, parser or visitors as:

```
./bin/nocmodl_lexer --file ../test/input/channel.mod
./bin/nocmodl_parser --file ../test/input/channel.mod
./bin/nocmodl_visitor --file ../test/input/channel.mod
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
valgrind --leak-check=full --track-origins=yes  ./bin/nocmodl_lexer
```

Or using CTest as:

```
ctest -T memcheck
```
