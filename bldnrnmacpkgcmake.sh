#!/bin/bash
set -ex
# distribution built with
# bash bldnrnmacpkgcmake.sh python2.7 python3.6 python3.7 python3.8
# without args, default is the 4 pythons above.

args="$*"
if test "$args" = "" ; then
  args="python2.7 python3.6 python3.7 python3.8"
fi

#10.7 possible if one builds with pythons that are consistent with that.
export MACOSX_DEPLOYMENT_TARGET=10.9


SRC=$HOME/neuron/nrncmake
BLD=$SRC/build
NSRC="$SRC"
export NSRC
NVER="`sh $NSRC/nrnversion.sh 3`"
NEURON_NVER=NEURON-${NVER}

INST=/Applications/${NEURON_NVER}
export PATH=$INST/bin:$PATH

rm -r -f $INST
cd $SRC
mkdir -p $BLD
rm -r -f $BLD/*
cd $BLD

PYVS="py"
pythons=""
for i in $* ; do
  PYVER=`$i -c 'from sys import version_info as v ; print (str(v.major) + str(v.minor)); quit()'`
  PYVS=${PYVS}-${PYVER}
  pythons="${pythons}${i};"
done

cmake .. -DCMAKE_INSTALL_PREFIX=$INST \
  -DNRN_ENABLE_MPI_DYNAMIC=ON \
  -DPYTHON_EXECUTABLE=`which python3.7` -DNRN_ENABLE_PYTHON_DYNAMIC=ON \
  -DNRN_PYTHON_DYNAMIC="$pythons" \
  -DIV_ENABLE_X11_DYNAMIC=ON \
  -DNRN_ENABLE_CORENEURON=OFF \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++

make -j install

chk () {
  # Can launch python and import neuron
  # only needs PYTHONPATH
  (
    export PYTHONPATH=$INST/lib/python
    $1 -c 'from neuron import test; test()'
    $1 -c 'from neuron.tests import test_rxd; test_rxd.test(); quit()'
  )
  return # need to work out the multiprocessing problem
  # Launching nrniv does not need NRN_PYLIB and PYTHONHOME
  (
    $INST/bin/nrniv -python -pyexe $1 -c 'import neuron ; neuron.test() ; quit()'
    $INST/bin/nrniv -python -pyexe $1 -c 'from neuron.tests import test_rxd; test_rxd.test(); quit()'
  )
}

#/Applications/Packages.app from
# http://s.sudre.free.fr/Software/Packages/about.html
# For mac to do a productsign, need my developerID_installer.cer
# and Neurondev.p12 file. To add to the keychain, double click each
# of those files. By default, I added my cerificate to the login keychain.
make macpkg

# test basic functionality
for i in $* ; do
  chk $i
done

#make alphadist

ALPHADIR='hines@neuron.yale.edu:/home/htdocs/ftp/neuron/versions/alpha'
describe="`sh $NSRC/nrnversion.sh describe`"
a=${ALPHADIR}/nrn-${describe}-osx-${PYVS}.pkg
b=./src/mac/build/${NEURON_NVER}.pkg
echo "scp $b $a"
scp $b $a

