
# Code Coverage

## Dependencies (Linux)
```
sudo apt install lcov
```

## Instructions

Clone the nrn repository and get ready to build.
```
    git clone https://github.com/neuronsimulator/nrn nrn
    cd nrn
    mkdir build
    cd build
```

Note: A simplified workflow is supported via the cmake option
[-DNRN_ENABLE_COVERAGE=ON](../cmake_doc/options.rst#nrn-enable-coverage-bool-off)
that removes the need to be concerned with COVERAGE_FLAGS and explicit
use of lcov and genhtml by providing the make targets ``make cover_begin``
and ``make cover_html``. See [Simplified Workflow](#simplified-workflow) below.

In addition to the COVERAGE_FLAGS use whatever cmake options you desire.
But you will generally want ```-DNRN_ENABLE_TESTS=ON``` to see what
effect your new tests have on coverage.
```
COVERAGE_FLAGS="--coverage -O0 -fno-inline -g"
cmake .. -DCMAKE_INSTALL_PREFIX=install -DCMAKE_C_FLAGS="${COVERAGE_FLAGS}" -DCMAKE_CXX_FLAGS="${COVERAGE_FLAGS}" -DNRN_ENABLE_TESTS=ON
make -j install
```

Set proper PATH (and PYTHONPATH if needed).
```
export PATH=`pwd`/install/bin
export PYTHONPATH=`pwd`/install/lib/python
```

Create a baseline report in coverage-base.info

A baseline report is recommended in order to
ensure that the percentage of total lines covered is correct
even when not  all  source  code  files  were loaded during the test.

```
(cd ..;  lcov --capture  --initial --directory . --no-external --output-file build/coverage-base.info)
```

Any nrniv runs will accumulate information about coverage in the .gdca files associated with the .o files
E.g. run tests with
```
ctest -VV
```

and create a report with all the coverage so far.
```
(cd ..; lcov --capture  --directory . --no-external --output-file build/coverage-run.info)
```

Combine baseline and test coverage data.
```
lcov --add-tracefile coverage-base.info --add-tracefile coverage-run.info --output-file coverage-combined.info
```

You can get a summary of the coverage so far.
```
lcov --summary coverage-combined.info
```

You can get a full html report.
(the output-directory avoids creating several dozen files in the top of the
build folder and hence it is easier to remove).
```
genhtml coverage-combined.info --output-directory html
```

And view the report by loading ```./html/index.html``` into your browser.

## Simplified Workflow

```
cmake .. -DCMAKE_INSTALL_PREFIX=install \
  -DPYTHON_EXECUTABLE=`which python3` \
  -DNRN_ENABLE_TESTS=ON \
  -DNRN_ENABLE_COVERAGE=ON \
  -DNRN_COVERAGE_FILES="src/nrniv/partrans.cpp;src/nmodl/parsact.cpp;src/nrnpython/nrnpy_hoc.cpp"

make -j 6 install

make cover_begin

make test

make cover_html
```
