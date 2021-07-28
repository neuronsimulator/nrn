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

run_mpi_test () {
  mpi_launcher=${1}
  mpi_name=${2}
  mpi_module=${3}

  echo "======= Testing $mpi_name ========"
  if [ -n "$mpi_module" ]; then
     echo "Loading module $mpi_module"
     module load $mpi_module
  fi

  # build new special
  rm -rf x86_64
  nrnivmodl tmp_mod

  # hoc and python based test
  $mpi_launcher -n 2 $python_exe src/parallel/test0.py -mpi --expected-hosts 2
  $mpi_launcher -n 2 nrniv src/parallel/test0.hoc -mpi --expected-hosts 2

  # run python test via nrniv and special (except on azure pipelines)
  if [[ "$SKIP_EMBEDED_PYTHON_TEST" != "true" ]]; then
    $mpi_launcher -n 2 ./x86_64/special -python src/parallel/test0.py -mpi --expected-hosts 2
    $mpi_launcher -n 2 nrniv -python src/parallel/test0.py -mpi --expected-hosts 2
  fi

  if [ -n "$mpi_module" ]; then
     echo "Unloading module $mpi_module"
     module unload $mpi_module
  fi
  echo -e "----------------------\n\n"
}


run_serial_test () {
    # Test 1: run base tests for within python
    $python_exe -c "import neuron; neuron.test(); neuron.test_rxd()"

    # Test 2: execute nrniv
    nrniv -c "print \"hello\""

    # Test 3: execute nrnivmodl
    rm -rf x86_64
    nrnivmodl tmp_mod

    # Test 4: execute special hoc interpreter
    ./x86_64/special -c "print \"hello\""

    # Test 5: run basic tests via python while loading shared library
    $python_exe -c "import neuron; neuron.test(); neuron.test_rxd(); quit()"

    # Test 6: run basic test to use compiled mod file
    $python_exe -c "import neuron; from neuron import h; s = h.Section(); s.insert('cacum'); quit()"

    # Test 7: run basic tests via special : azure pipelines get stuck with their
    # own python from hosted cache (most likely security settings).
    if [[ "$SKIP_EMBEDED_PYTHON_TEST" != "true" ]]; then
      ./x86_64/special -python -c "import neuron; neuron.test(); neuron.test_rxd(); quit()"
      nrniv -python -c "import neuron; neuron.test(); neuron.test_rxd(); quit()"
    else
      $python_exe -c "import neuron; neuron.test(); neuron.test_rxd(); quit()"
    fi

    # Test 8: run demo
    neurondemo -c 'demo(4)' -c 'run()' -c 'quit()'

    # Test 9: modlunit available (and can find nrnunits.lib)
    modlunit tmp_mod/cacum.mod
}

run_parallel_test() {
    # this is for MacOS system
    if [[ "$OSTYPE" == "darwin"* ]]; then
      # assume both MPIs are installed via brew.

      brew unlink openmpi
      brew link mpich

      # TODO : latest mpich has issuee on Azure OSX
      if [[ "$CI_OS_NAME" == "osx" ]]; then
          run_mpi_test "/usr/local/opt/mpich/bin/mpirun" "MPICH" ""
      fi

      brew unlink mpich
      brew link openmpi
      run_mpi_test "/usr/local/opt/open-mpi/bin/mpirun" "OpenMPI" ""

    # CI Linux or Azure Linux
    elif [[ "$CI_OS_NAME" == "linux" || "$AGENT_OS" == "Linux" ]]; then
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
    cp share/examples/nrniv/nmodl/cacum.mod tmp_mod/

    echo "Using `which $python_exe` : `$python_exe --version`"
    echo "=========== SERIAL TESTS ==========="
    run_serial_test

    echo "=========== MPI TESTS ============"
    run_parallel_test

    #clean-up
    rm -rf tmp_mod x86_64
}

echo "== Testing $python_wheel using $python_exe ($python_ver) =="

# creat python virtual environment and use `python` as binary name
# because it will be correct one from venv.
if [[ "$use_venv" != "false" ]]; then
  echo " == Creating virtual environment == "
  venv_name="nrn_test_venv_${python_ver}"
  $python_exe -m venv $venv_name
  . $venv_name/bin/activate
  python_exe=`which python`
else
  echo " == Using global install == "
fi

# python 3.6 needs updated pip
if [[ "$python_ver" == "36" ]]; then
  $python_exe -m pip install --upgrade pip
fi

# install numpy and neuron
$python_exe -m pip install numpy
$python_exe -m pip install $python_wheel
$python_exe -m pip show neuron || $python_exe -m pip show neuron-nightly

# run tests
test_wheel $(which python)

# cleanup
if [[ "$use_venv" != "false" ]]; then
  deactivate
fi

#rm -rf $venv_name
echo "Removed $venv_name"
