# A simple set of tests checking if a wheel is working correctly
set -e

if [ ! -f setup.py ]; then
    echo "Error: Please launch $0 from the root dir"
    exit 1
fi

if [ "$#" -ne 2 ]; then
    echo "Usage: $(basename $0) python_exe python_wheel"
    exit 1
fi

test_wheel () {
    python_exe=${1}

    echo "PYTHON VERSION `python --version`"

	# sample mod file for nrnivmodl check
    mkdir -p tmp_mod
    cp share/examples/nrniv/nmodl/gap.mod tmp_mod/

    # Test 1: run base tests for within python
    $python_exe -c "import neuron; neuron.test(); neuron.test_rxd()"

    # Test 2: execute nrniv
    nrniv -c "print \"hello\""

    # Test 3: execute nrnivmodl
    rm -rf x86_64
    nrnivmodl tmp_mod

    # Test 4: run base tests for within python via special
    ./x86_64/special -python -c "import neuron; neuron.test(); neuron.test_rxd(); quit()"

    # Test 5: execute nrniv
    ./x86_64/special -c "print \"hello\""

    # Test 6: run demo
    neurondemo -c 'demo(4)' -c 'run()' -c 'quit()'

    # Test 7: multi-mpi test
    echo "Testing MPI"

    # TODO : we are using custom paths for MPI. We will change
    # this when we use travis CI.
    if [[ "$OSTYPE" == "darwin"* ]]; then
      # assume both MPIs are installed via brew.
      brew unlink openmpi
      brew link mpich
      echo "Testing MPICH"
      /usr/local/opt/mpich/bin/mpirun -n 2 python src/parallel/test0.py -mpi
      /usr/local/opt/mpich/bin/mpirun -n 2 nrniv -python src/parallel/test0.py -mpi
      /usr/local/opt/mpich/bin/mpirun -n 2 nrniv src/parallel/test0.hoc -mpi

      brew unlink mpich
      brew link openmpi
      echo "Testing OpenMPI"
      /usr/local/opt/openmpi/bin/mpirun -n 2 python src/parallel/test0.py -mpi
      /usr/local/opt/openmpi/bin/mpirun -n 2 nrniv -python src/parallel/test0.py -mpi
      /usr/local/opt/openmpi/bin/mpirun -n 2 nrniv src/parallel/test0.hoc -mpi
    else
      echo "Testing MPICH"
      export PATH=/opt/mpich/bin:$PATH
      export LD_LIBRARY_PATH=/opt/mpich/lib:$LD_LIBRARY_PATH
      mpirun -n 2 python src/parallel/test0.py -mpi
      mpirun -n 2 nrniv -python src/parallel/test0.py -mpi
      mpirun -n 2 nrniv src/parallel/test0.hoc -mpi

      echo "Testing OpenMPI"
      export PATH=/opt/openmpi/bin:$PATH
      export LD_LIBRARY_PATH=/opt/openmpi/lib:$LD_LIBRARY_PATH
      mpirun -n 2 python src/parallel/test0.py -mpi
      mpirun -n 2 nrniv -python src/parallel/test0.py -mpi
      mpirun -n 2 nrniv src/parallel/test0.hoc -mpi
    fi

    #clean-up
    rm -rf tmp_mod x86_64
}

# proviided parameters
python_exe=$1
python_wheel="$2"
python_ver=$("$python_exe" -c "import sys; print('%d%d' % tuple(sys.version_info)[:2])")

# setup python virtual environment
venv_name="nrn_test_venv_${python_ver}"
$python_exe -m venv $venv_name
. $venv_name/bin/activate

# on osx we need to install pip from source
if [[ "$OSTYPE" == "darwin"* ]] && [[ "$(python -V)" =~ "Python 3.5" ]]; then
  echo "Updating pip for OSX with Python 3.5"
  curl https://bootstrap.pypa.io/get-pip.py | python
fi

# install neuron and neuron
pip install numpy
pip install $python_wheel

# run tests
test_wheel $python_exe

# cleanup
#deactivate
#rm -rf $venv_name
echo "Removed $venv_name"
