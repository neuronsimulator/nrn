#!/bin/bash
set -ex
# distribution built with
# bash bldnrnmacpkgcmake.sh python3.6 python3.7 python3.8 python3.9
# without args, default is the 5 pythons above.

CPU=`uname -m`

args="$*"
if test "$args" = "" ; then
  if test "$CPU" = "x86_64" ; then
    args="python3.6 python3.7 python3.8 python3.9"
  else # arm64
    args="python3 python3.9"
  fi
fi

#10.7 possible if one builds with pythons that are consistent with that.
if test "$CPU" = "x86_64" ; then
  export MACOSX_DEPLOYMENT_TARGET=10.9
fi

if test "$NRN_SRC" == "" ; then
  NRN_SRC=$HOME/neuron/nrn
fi
NRN_BLD=$NRN_SRC/build
NSRC=$NRN_SRC
export NSRC

NRN_INSTALL=/Applications/NEURON
export PATH=$NRN_INSTALL/bin:$PATH

rm -r -f $NRN_INSTALL
cd $NRN_SRC
mkdir -p $NRN_BLD
rm -r -f $NRN_BLD/*
cd $NRN_BLD

PYVS="py" # will be part of package file name, eg. py-27-35-36-37-38
pythons="" # will be arg value of NRN_PYTHON_DYNAMIC
for i in $args ; do
  PYVER=`$i -c 'from sys import version_info as v ; print (str(v.major) + str(v.minor)); quit()'`
  PYVS=${PYVS}-${PYVER}
  pythons="${pythons}${i};"
done

cmake .. -DCMAKE_INSTALL_PREFIX=$NRN_INSTALL \
  -DNRN_ENABLE_MPI_DYNAMIC=ON \
  -DPYTHON_EXECUTABLE=`which python3` -DNRN_ENABLE_PYTHON_DYNAMIC=ON \
  -DNRN_PYTHON_DYNAMIC="$pythons" \
  -DIV_ENABLE_X11_DYNAMIC=ON \
  -DNRN_ENABLE_CORENEURON=OFF \
  -DNRN_RX3D_OPT_LEVEL=2 \
  -DCMAKE_OSX_ARCHITECTURES="$CPU" \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++

make -j install

chk () {
  # Can launch python and import neuron
  # only needs PYTHONPATH
  (
    export PYTHONPATH=$NRN_INSTALL/lib/python
    $1 -c 'from neuron import test; test()'
    $1 -c 'from neuron.tests import test_rxd; test_rxd.test(); quit()'
  )
  return # need to work out the multiprocessing problem
  # Launching nrniv does not need NRN_PYLIB and PYTHONHOME
  (
    $NRN_INSTALL/bin/nrniv -python -pyexe $1 -c 'import neuron ; neuron.test() ; quit()'
    $NRN_INSTALL/bin/nrniv -python -pyexe $1 -c 'from neuron.tests import test_rxd; test_rxd.test(); quit()'
  )
}

#/Applications/Packages.app from
# http://s.sudre.free.fr/Software/Packages/about.html
# For mac to do a productsign, need my developerID_installer.cer
# and Neurondev.p12 file. To add to the keychain, double click each
# of those files. By default, I added my certificates to the login keychain.
make macpkg # will sign the binaries, construct below
            # mentioned PACKAGE_FILE_NAME, and request notarization from
            # Apple. At the end it will print a stapling request that you
            # should run manually after receiving a "success" email from
            # Apple.

# test basic functionality
for i in $args ; do
  chk $i
done

# upload package to neuron.yale.edu
ALPHADIR='hines@neuron.yale.edu:/home/htdocs/ftp/neuron/versions/alpha'
describe="`sh $NRN_SRC/nrnversion.sh describe`"
PACKAGE_FULL_NAME=nrn-${describe}-osx-${CPU}-${PYVS}.pkg
PACKATE_DOWNLOAD_NAME=$ALPHADIR/$PACKAGE_FULL_NAME
PACKAGE_FILE_NAME=$NRN_BLD/src/mac/build/NEURON.pkg
echo "
  Until we figure out how to automatically staple the notarization
  the following two commands must be executed manually.
  xcrun stapler staple $PACKAGE_FILE_NAME
  cp $PACKAGE_FILE_NAME $HOME/$PACKAGE_FULL_NAME
"
