## NOCMODL

This is prototype implementation of code generation framework for NMODL.

#### Cloning Source

```
git clone ssh://bbpcode.epfl.ch:22/incubator/nocmodl
```

#### Dependencies

Make sure to have latest version of flex (>=2.6) and bison (>=3.0). For example, on OS X we do:

```
brew install flex bison
```
This will install flex and bison in:

```
/usr/local/opt/flex
/usr/local/opt/bison
```

On Ubuntu 16.04 you should already have recent version of flex/bison.

#### Build
Install NOCMODL as:

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

 #### Running NOCMODL

 You can independently run lexer, parser as:

 ```
./bin/nocmodl_lexer --file ../test/input/channel.mod
./bin/nocmodl_parser --file ../test/input/channel.mod
 ````


 #### Running Test

 You can run unit tests as:

 ```
 make test
 ```

 Or individual binaries with verbode output:

 ```
 ./bin/testlexer -s
 ./bin/testmodtoken -s
 ```


#### Memory Leaks

Test memory leaks using :

```
valgrind --leak-check=full --track-origins=yes  ./bin/nocmodl_lexer
```
