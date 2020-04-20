#!/usr/bin/env bash

set -ex

# Test 1: run neurondemo
neurondemo -c 'demo(4)' -c 'run()' -c 'quit()'

# Test 2: import neuron serially
python -c "import neuron; neuron.test(); neuron.test_rxd()"

# Test 3: execute nrniv serially
nrniv -c "print \"hello\""

# Test 4: execute nrnivmodl
rm -rf x86_64
nrnivmodl .

# Test 5: run base tests for within python via special serially
./x86_64/special -python -c "import neuron; neuron.test(); neuron.test_rxd(); quit()"

# Test 6: execute nrniv serially
./x86_64/special -c "print \"hello\""

# Test 7: MPI support (make sure OpenMPI is installed/used)
mpirun -n 2 python src/parallel/test0.py -mpi
mpirun -n 2 nrniv -python src/parallel/test0.py -mpi