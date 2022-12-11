#!/usr/bin/env bash
# A simple set of tests checking if a wheel is working correctly
set -xe

# See CMake's CMAKE_HOST_SYSTEM_PROCESSOR documentation
# On the systems where we are building wheel we can rely
# on uname -m. Note that this is just wheel testing script.
ARCH_DIR=`uname -m`

if [ ! -f setup.py ]; then
    echo "Error: Please launch $0 from the root dir"
    exit 1
fi

if [ "$#" -lt 2 ]; then
    echo "Usage: $(basename $0) python_exe python_wheel [use_virtual_env]"
    exit 1
fi

# cli parameters
python_exe=$1   # python to be used for testing
python_wheel=$2 # python wheel to be tested
use_venv=$3     # if $3 is not "false" then use virtual environment

# There are some considerations to test coreneuron with gpu support:
# - if coreneuron support exist then we can always run all tests on cpu
# - if coreneuron gpu support exist then we can run always binaries like
#   nrniv and nrniv-core without existence of NVC/NVC++ compilers
# - if coreneuron gpu support exist and nvc compiler available then we
#   can compile mod files and run tests via special binary
# - Note that the tests that use coreneuron can not be launched using
#   python or nrniv -python because in gpu build coreneuron is created
#   as a static library and linked to special. Hence only special can
#   be used to launch GPU tests
has_coreneuron=false   # true if coreneuron support is available
has_gpu_support=false  # true if coreneuron gpu support is available
has_dev_env=true       # true if c/c++ dev environment exist to compile mod files
run_gpu_test=false     # true if test should be run on the gpu

# python version being used
python_ver=$("$python_exe" -c "import sys; print('%d%d' % tuple(sys.version_info)[:2])")


run_mpi_test () {
  mpi_launcher=${1}
  mpi_name=${2}
  mpi_module=${3}

  echo "======= Testing $mpi_name ========"
  if [ -n "$mpi_module" ]; then
     echo "Loading module $mpi_module"
     if [[ $(hostname -f) = *r*bbp.epfl.ch* ]]; then
        echo "\tusing unstable on BB5"
        module load unstable
     fi
     module load $mpi_module
  fi

  # hoc and python based test
  $mpi_launcher -n 2 $python_exe src/parallel/test0.py -mpi --expected-hosts 2
  $mpi_launcher -n 2 nrniv src/parallel/test0.hoc -mpi --expected-hosts 2

  # TODO: run coreneuron binary shipped inside wheel. This can not be executed in platform
  #       independent manner because we need to pre-load libmpi library beforehand e.g. using
  #       LD_PRELOAD mechanism. For now, execute without --mpi argument
  if [[ "$has_coreneuron" == "true" ]] && [[ $mpi_name == *"MPICH"* || $mpi_name == *"Intel MPI"* ]]; then
      site_package_dir=`$python_exe -c 'import os, neuron; print(os.path.dirname(neuron.__file__))'`
      corenrn_mpi_lib=`ls $site_package_dir/.data/lib/libcorenrnmpi_mpich*`

      HOC_LIBRARY_PATH=${PWD}/test/ringtest nrniv test/ringtest/ring.hoc
      mv out.dat out.nrn.dat
      $mpi_launcher -n 1 nrniv-core --datpath . --mpi-lib=$corenrn_mpi_lib
      diff -w out.dat out.nrn.dat
      rm -rf *.dat
  fi

  # rest of the tests we need development environment. For GPU wheel
  # make sure we have necessary compiler.
  if [[ "$has_dev_env" == "false" ]]; then
      echo "WARNING: Development environment missing, skipping rest of the MPI tests!"
      return
  fi

  # build new special
  rm -rf $ARCH_DIR
  nrnivmodl tmp_mod

  # run python test via nrniv and special (except on azure pipelines)
  if [[ "$SKIP_EMBEDED_PYTHON_TEST" != "true" ]]; then
    $mpi_launcher -n 2 ./$ARCH_DIR/special -python src/parallel/test0.py -mpi --expected-hosts 2
    $mpi_launcher -n 2 nrniv -python src/parallel/test0.py -mpi --expected-hosts 2
  fi

  # coreneuron execution via neuron
  if [[ "$has_coreneuron" == "true" ]]; then
    rm -rf $ARCH_DIR
    nrnivmodl -coreneuron "test/coreneuron/mod files/"

    # python as a launcher can be used only with non-gpi build
    if [[ "$has_gpu_support" == "false" ]]; then
      $mpi_launcher -n 1 $python_exe test/coreneuron/test_direct.py
    fi

    # using -python doesn't work on Azure CI
    if [[ "$SKIP_EMBEDED_PYTHON_TEST" != "true" ]]; then
      if [[ "$has_gpu_support" == "false" ]]; then
        $mpi_launcher -n 2 nrniv -python -mpi test/coreneuron/test_direct.py
      fi
      run_on_gpu=$([ "$run_gpu_test" == "true" ] && echo "1" || echo "0")
      NVCOMPILER_ACC_TIME=1 CORENRN_ENABLE_GPU=$run_on_gpu $mpi_launcher -n 2 ./$ARCH_DIR/special -python -mpi test/coreneuron/test_direct.py
    fi
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

    # Test 3: run coreneuron binary shipped inside wheel
    if [[ "$has_coreneuron" == "true" ]]; then
        HOC_LIBRARY_PATH=${PWD}/test/ringtest nrniv test/ringtest/ring.hoc
        mv out.dat out.nrn.dat
        nrniv-core --datpath .
        diff -w out.dat out.nrn.dat
        rm -rf *.dat
    fi

    # rest of the tests we need development environment
    if [[ "$has_dev_env" == "false" ]]; then
        echo "WARNING: Development environment missing, skipping rest of the serial tests!"
        return
    fi

    # Test 4: execute nrnivmodl
    rm -rf $ARCH_DIR
    nrnivmodl tmp_mod

    # Test 5: execute special hoc interpreter
    ./$ARCH_DIR/special -c "print \"hello\""

    # Test 6: run basic tests via python while loading shared library
    $python_exe -c "import neuron; neuron.test(); neuron.test_rxd(); quit()"

    # Test 7: run basic test to use compiled mod file
    $python_exe -c "import neuron; from neuron import h; s = h.Section(); s.insert('cacum'); quit()"

    # Test 8: run basic tests via special : azure pipelines get stuck with their
    # own python from hosted cache (most likely security settings).
    if [[ "$SKIP_EMBEDED_PYTHON_TEST" != "true" ]]; then
      ./$ARCH_DIR/special -python -c "import neuron; neuron.test(); neuron.test_rxd(); quit()"
      nrniv -python -c "import neuron; neuron.test(); neuron.test_rxd(); quit()"
    else
      $python_exe -c "import neuron; neuron.test(); neuron.test_rxd(); quit()"
    fi

    # Test 9: coreneuron execution via neuron
    if [[ "$has_coreneuron" == "true" ]]; then
      rm -rf $ARCH_DIR

      # first test vanialla coreneuron support, without nrnivmodl
      if [[ "$has_gpu_support" == "false" ]]; then
        $python_exe test/coreneuron/test_psolve.py
      fi

      nrnivmodl -coreneuron "test/coreneuron/mod files/"

      # coreneuron+gpu can be used via python but special only
      if [[ "$has_gpu_support" == "false" ]]; then
        $python_exe test/coreneuron/test_direct.py
      fi

      # using -python doesn't work on Azure CI
      if [[ "$SKIP_EMBEDED_PYTHON_TEST" != "true" ]]; then
        # we can run special with or without gpu wheel
        ./$ARCH_DIR/special -python test/coreneuron/test_direct.py

        # python and nrniv can be used only for non-gpu wheel
        if [[ "$has_gpu_support" == "false" ]]; then
          nrniv -python test/coreneuron/test_direct.py
        fi

        if [[ "$run_gpu_test" == "true" ]]; then
          NVCOMPILER_ACC_TIME=1 CORENRN_ENABLE_GPU=1 ./$ARCH_DIR/special -python test/coreneuron/test_direct.py
        fi
      fi

      rm -rf $ARCH_DIR
    fi


    # Test 10: run demo
    neurondemo -c 'demo(4)' -c 'run()' -c 'quit()'

    # Test 11: modlunit available (and can find nrnunits.lib)
    modlunit tmp_mod/cacum.mod
}


run_parallel_test() {
    # this is for MacOS system
    if [[ "$OSTYPE" == "darwin"* ]]; then
      # assume both MPIs are installed via brew.

      brew unlink openmpi
      brew link mpich
      BREW_PREFIX=$(brew --prefix)

      # TODO : latest mpich has issuee on Azure OSX
      if [[ "$CI_OS_NAME" == "osx" ]]; then
          run_mpi_test "${BREW_PREFIX}/opt/mpich/bin/mpirun" "MPICH" ""
      fi

      brew unlink mpich
      brew link openmpi
      run_mpi_test "${BREW_PREFIX}/opt/open-mpi/bin/mpirun" "OpenMPI" ""

    # CI Linux or Azure Linux
    elif [[ "$CI_OS_NAME" == "linux" || "$AGENT_OS" == "Linux" ]]; then
      # make debugging easier
      sudo update-alternatives --get-selections | grep mpi
      sudo update-alternatives --list mpi-x86_64-linux-gnu
      # choose mpich
      sudo update-alternatives --set mpi-x86_64-linux-gnu /usr/include/x86_64-linux-gnu/mpich
      run_mpi_test "mpirun.mpich" "MPICH" ""
      # choose openmpi
      sudo update-alternatives --set mpi-x86_64-linux-gnu /usr/lib/x86_64-linux-gnu/openmpi/include
      run_mpi_test "mpirun.openmpi" "OpenMPI" ""

    # BB5 with multiple MPI libraries
    elif [[ $(hostname -f) = *r*bbp.epfl.ch* ]]; then
      run_mpi_test "srun" "HPE-MPT" "hpe-mpi"
      run_mpi_test "mpirun" "Intel MPI" "intel-oneapi-mpi"
      run_mpi_test "srun" "MVAPICH2" "mvapich2"

    # circle-ci build
    elif [[ "$CIRCLECI" == "true" ]]; then
      sudo update-alternatives --set mpi-aarch64-linux-gnu /usr/include/aarch64-linux-gnu/mpich
      run_mpi_test "mpirun.mpich" "MPICH" ""
      sudo update-alternatives --set mpi-aarch64-linux-gnu /usr/lib/aarch64-linux-gnu/openmpi/include
      run_mpi_test "mpirun.openmpi" "OpenMPI" ""

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

    # check gcc and python versions
    gcc --version && python --version

    echo "Using `which $python_exe` : `$python_exe --version`"
    echo "=========== SERIAL TESTS ==========="
    run_serial_test

    echo "=========== MPI TESTS ============"
    run_parallel_test

    #clean-up
    rm -rf tmp_mod $ARCH_DIR
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


# gpu wheel needs updated pip
$python_exe -m pip install --upgrade pip


# install numpy, pytest and neuron
$python_exe -m pip install numpy pytest
$python_exe -m pip install $python_wheel
$python_exe -m pip show neuron \
    || $python_exe -m pip show neuron-nightly \
    || $python_exe -m pip show neuron-gpu \
    || $python_exe -m pip show neuron-gpu-nightly


# check the existence of coreneuron support
compile_options=`nrniv -nobanner -nogui -c 'nrnversion(6)'`
if echo $compile_options | grep "NRN_ENABLE_CORENEURON=ON" > /dev/null ; then
  has_coreneuron=true
fi

# check if the gpu support is enabled
if echo $compile_options | grep "CORENRN_ENABLE_GPU=ON" > /dev/null ; then
  has_gpu_support=true
fi

# in case of gpu support, nvc/nvc++ compiler must exist to compile mod files
if [[ "$has_gpu_support" == "true" ]]; then
  if ! command -v nvc &> /dev/null; then
    has_dev_env=false
  fi

  # check if nvidia gpu exist (todo: not a robust check)
  if pgaccelinfo -nvidia | grep -q "Device Name"; then
    run_gpu_test=true
  fi
fi


# run tests
test_wheel $(which python)


# cleanup
if [[ "$use_venv" != "false" ]]; then
  deactivate
fi

#rm -rf $venv_name
echo "Removed $venv_name"
