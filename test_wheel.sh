# A simple set of tests checking if a wheel is working correctly
set -xe

if [ "$#" -ne 2 ]; then
    echo "Error : use ./test_wheel.bash python_exe python_wheel"
    exit 1
fi

python_exe=$1
python_wheel=$2

# setup python virtual environment
venv_name="my_venv_${python_exe}"
$python_exe -m venv $venv_name
. $venv_name/bin/activate

# insrall neuron
pip install $python_wheel

# download some mod files
[ -d testcorenrn ] ||  git clone https://github.com/nrnhines/testcorenrn.git

# Test 1: run base tests for within python
$python_exe -c "import neuron; neuron.test(); neuron.test_rxd()"

# Test 2: execute nrniv
nrniv -c "print \"hello\""

# Test 3: execute nrnivmodl
[ -d testcorenrn ] ||  git clone https://github.com/nrnhines/testcorenrn.git
nrnivmodl testcorenrn

# Test 4: run base tests for within python via special
./x86_64/special -python -c "import neuron; neuron.test(); neuron.test_rxd(); quit()"

# Test 5: execute nrniv
./x86_64/special -c "print \"hello\""

# cleanup
deactivate
rm -rf $venv_name
