# A simple set of tests checking if neuron is working correctly
set -xe

python_exe="${1:-python}"

# download some mod files for testing nrnivmodl
[ -d testcorenrn ] ||  git clone https://github.com/nrnhines/testcorenrn.git

# Test 1: run base tests for within python
$python_exe -c "import neuron; neuron.test(); neuron.test_rxd()"

# Test 2: execute nrniv
nrniv -c "print \"hello\""

# Test 3: execute nrnivmodl
rm -rf x86_64
nrnivmodl testcorenrn

# Test 4: run base tests for within python via special
./x86_64/special -python -c "import neuron; neuron.test(); neuron.test_rxd(); quit()"

# Test 5: execute nrniv
./x86_64/special -c "print \"hello\""
