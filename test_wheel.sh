# A simple set of tests checking if a wheel is working correctly
set -e

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
if [[ "$OSTYPE" == "darwin"* ]] && [[ "$(python -V)" =~ "Python 3.5" ]]; then
  echo "Updating pip for OSX with Python 3.5"
  curl https://bootstrap.pypa.io/get-pip.py | python
fi

pip install numpy
pip install $python_wheel
#pip install -i https://test.pypi.org/simple/ NEURON==7.8.11.2

# Run tests
./test/test_neuron.sh $python_exe

# cleanup
deactivate
rm -rf $venv_name
echo "Removed $venv_name"
