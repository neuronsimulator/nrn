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

# Install Neuron from wheel
pip install $python_wheel

# Run tests
bash test/test_neuron.sh $python_exe

# cleanup
deactivate
rm -rf $venv_name
