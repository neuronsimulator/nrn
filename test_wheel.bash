# A simple set of tests checking if a wheel is working correctly
set -xe

# Test 1: run base tests for within python
python -c "import neuron; neuron.test(); neuron.test_rxd()"

# Test 2: execute nrniv
nrniv -c "print \"hello\""

# Test 3: execute nrnivmodl
[ -d testcorenrn ] ||  git clone https://github.com/nrnhines/testcorenrn.git
nrnivmodl testcorenrn

