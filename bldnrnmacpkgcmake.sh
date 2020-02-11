#!/bin/bash
set -ex
# distribution built with
#sh bldnrnmacpkgcmake.sh python2.7 python3.6 python3.7 python3.8

#10.7 possible if one builds with pythons that are consistent with that.
export MACOSX_DEPLOYMENT_TARGET=10.9

INST=/Applications/NEURON-7.8
export PATH=$INST/nrn/bin:$PATH

SRC=$HOME/neuron/nrncmake
BLD=$SRC/build

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

cmake .. -DCMAKE_INSTALL_PREFIX=$INST/nrn \
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
    export PYTHONPATH=$INST/nrn/lib/python
    $1 -c 'from neuron import test; test()'
    $1 -c 'from neuron.tests import test_rxd; test_rxd.test(); quit()'
  )
  return # need to work out the multiprocessing problem
  # Launching nrniv does not need NRN_PYLIB and PYTHONHOME
  (
    $INST/nrn/bin/nrniv -python -pyexe $1 -c 'import neuron ; neuron.test() ; quit()'
    $INST/nrn/bin/nrniv -python -pyexe $1 -c 'from neuron.tests import test_rxd; test_rxd.test(); quit()'
  )
}

make after-install
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
export NSRC="$SRC"
describe="`sh $NSRC/nrnversion.sh describe`"
NVER="`sh $NSRC/nrnversion.sh 3`"
a=${ALPHADIR}/nrn-${describe}-osx-${PYVS}.pkg
b=./src/mac/build/NEURON-${NVER}.pkg
echo "scp $b $a"
scp $b $a

