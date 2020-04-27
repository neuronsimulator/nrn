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

# cli parameters
python_exe=$1
python_wheel=$2

python_ver=$("$python_exe" -c "import sys; print('%d%d' % tuple(sys.version_info)[:2])")


run_mpi_test () {
  mpi_launcher=${1}
  mpi_name=${2}
  mpi_module=${3}

  echo "======= Testing $mpi_name ========"
  if [ -n "$mpi_module" ]; then
     echo "Loading module $mpi_module"
     module load $mpi_module
  fi

  $mpi_launcher -n 2 python src/parallel/test0.py -mpi --expected-hosts 2
  $mpi_launcher -n 2 nrniv -python src/parallel/test0.py -mpi --expected-hosts 2
  $mpi_launcher -n 2 nrniv src/parallel/test0.hoc -mpi --expected-hosts 2

  if [ -n "$mpi_module" ]; then
     echo "Unloading module $mpi_module"
     module unload $mpi_module
  fi
  echo -e "----------------------\n\n"
}


run_serial_test () {
    # Test 1: run base tests for within python
    python -c "import neuron; neuron.test(); neuron.test_rxd()"

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
}

run_parallel_test() {
    # this is for MacOS system
    if [[ "$OSTYPE" == "darwin"* ]]; then
      # assume both MPIs are installed via brew.
      brew unlink openmpi
      brew link mpich
      run_mpi_test "/usr/local/opt/mpich/bin/mpirun" "MPICH" ""

      brew unlink mpich
      brew link openmpi
      run_mpi_test "/usr/local/opt/open-mpi/bin/mpirun" "OpenMPI" ""

    # Travis Linux
    elif [ "$TRAVIS_OS_NAME" == "linux" ]; then
      sudo update-alternatives --set mpi /usr/include/mpich
      run_mpi_test "mpirun.mpich" "MPICH" ""
      sudo update-alternatives --set mpi /usr/lib/x86_64-linux-gnu/openmpi/include
      run_mpi_test "mpirun.openmpi" "OpenMPI" ""

    # BB5 with multiple MPI libraries
    elif [[ $(hostname -f) = *r*bbp.epfl.ch* ]]; then
      run_mpi_test "srun" "HPE-MPT" "hpe-mpi"
      run_mpi_test "mpirun" "Intel MPI" "intel-mpi"
      run_mpi_test "srun" "MVAPICH2" "mvapich2/2.3"
      run_mpi_test "mpirun" "OpenMPI" "openmpi/4.0.0"

    # linux desktop or docker container used for wheel
    else
      export PATH=/opt/mpich/bin:$PATH
      export LD_LIBRARY_PATH=/opt/mpich/lib:$LD_LIBRARY_PATH
      run_mpi_test "mpirun" "MPICH" ""

      export PATH=/opt/openmpi/bin:$PATH
      export LD_LIBRARY_PATH=/opt/openmpi/lib:$LD_LIBRARY_PATH
      run_mpi_test "mpirun" "OpenMPI" ""
    fi
}

test_wheel () {
    # sample mod file for nrnivmodl check
    mkdir -p tmp_mod
    cp share/examples/nrniv/nmodl/gap.mod tmp_mod/

    echo "Using `which python` : `python --version`"
    echo "=========== SERIAL TESTS ==========="
    run_serial_test

    echo "=========== MPI TESTS ============"
    run_parallel_test

    #clean-up
    rm -rf tmp_mod x86_64
}

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
pip show neuron

# run tests
test_wheel $(which python)

# cleanup
deactivate
#rm -rf $venv_name
echo "Removed $venv_name"
