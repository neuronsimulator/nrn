#!/bin/bash
# A simple set of tests checking if a wheel is working correctly
set -xe

if [ ! -f setup.py ]; then
    echo "Error: Please launch $0 from the root dir"
    exit 1
fi

if [ "$#" -lt 2 ]; then
    echo "Usage: $(basename $0) python_exe python_wheel [use_virtual_env]"
    exit 1
fi

# cli parameters
python_exe=$1
python_wheel=$2
use_venv=$3 #if $3 is not "false" then use virtual environment

python_ver=$("$python_exe" -c "import sys; print('%d%d' % tuple(sys.version_info)[:2])")


test_wheel () {
    # sample mod file for nrnivmodl check
    local TEST_DIR="test_dir" 
    mkdir -p $TEST_DIR
    cp ../nmodl/ext/example/*.mod $TEST_DIR/
    cp ../test/integration/mod/cabpump.mod ../test/integration/mod/var_init.inc $TEST_DIR/
    cd $TEST_DIR
    for mod in *.mod
    do
        nmodl $mod sympy --analytic
    done
    $python_exe -c "import nmodl; driver = nmodl.NmodlDriver(); driver.parse_file('hh.mod')"
    cd ..
    #clean-up
    rm -rf $TEST_DIR
}

echo "== Testing $python_wheel using $python_exe ($python_ver) =="

mkdir testwheel && cd testwheel

# creat python virtual environment and use `python` as binary name
# because it will be correct one from venv.
if [[ "$use_venv" != "false" ]]; then
  echo " == Creating virtual environment == "
  venv_name="nmodl_test_venv_${python_ver}"
  $python_exe -m venv $venv_name
  . $venv_name/bin/activate
  python_exe=`which python`
else
  echo " == Using global install == "
fi

# install nmodl
$python_exe -m pip install -U pip
$python_exe -m pip install ../$python_wheel
$python_exe -m pip show nmodl || $python_exe -m pip show nmodl-nightly

# run tests
test_wheel $(which python)

# cleanup
if [[ "$use_venv" != "false" ]]; then
  deactivate
fi

rm -rf $venv_name
echo "Removed $venv_name"
