#!/usr/bin/env bash
set -ex

default_pythons="python3.9 python3.10 python3.11 python3.12 python3.13"
# distribution built with
# bash bldnrnmacpkgcmake.sh
# without args, default are the pythons above.

# rx3doptlevel=2 is default but very slow compiling. For script testing
# launch with rx3doptlevel set to 0 or 1, e.g.
# rx3doptlevel=0 bash bldnrnmacpkgcmake.sh
if test "$rx3doptlevel" = "" ; then
  rx3doptlevel=2
fi

# Now obsolete...
# python3.8 needs to be universal and macosx-10.9. So we built our own
# from python.org python3.8.18 sources. Unfortunately, I cannot get pip
# to work (OpenSSH issues), so I installed python.org Python3.8.10 installer
# (which would have been fine by itself except it is macosx-11)
# and copied relevant site-packages from that into the python3.8.18
# installation site-packages.
# export PATH=$HOME/soft/python3.8/bin:$PATH

# As of Python-3.13.0, we configure to link universal2 static readline
# (and ncurses) into libnrniv.dylib.
# The packages were downloade and the universal libraries were built with
# nrn/packaging/python/build_static_readline_osx.bash
READLINE_LIST="/opt/nrnwheel/readline;/opt/nrnwheel/ncurses"

# In the top level source dir?# 
if test -f nrnversion.sh ; then
  NRN_SRC=`pwd`
fi

# If all the pythons are universal, then so is NEURON.
# Otherwise $CPU
# All pythons must have the same macos version and that will become
# the MACOSX_DEPLOYMENT_TARGET

CPU=`uname -m`

universal="yes" # changes to "no" if any python not universal

args="$*"
if test "$args" = "" ; then
  args="$default_pythons"
fi

# sysconfig.get_platform() looks like, e.g. "macosx-12.2-arm64" or
# "macosx-11-universal2". I.e. encodes MACOSX_DEPLOYMENT_TARGET and archs.
# Demand all pythons we are building against have same platform.
# Python 3.13 is 10.13. The substantive aspect of
# the following fragment (exit 1 if not all the same platform), has been
# commented out.

mac_platform=""
for i in $args ; do
  last_py=$i
  mplat=`$i -c 'import sysconfig; print(sysconfig.get_platform())'`
  echo "platform for $i is $mplat"
  if test "$mac_platform" = "" ; then
    mac_platform=$mplat
  fi
  if test "$mac_platform" != "$mplat" ; then
    echo "$i platform \"$mplat\" differs from previous python \"$mac_platform\"."
#    exit 1
  fi
done

# extract MACOSX_DEPLOYMENT_TARGET and archs from mac_platform
macosver=`$last_py -c 'import sysconfig; print(sysconfig.get_platform().split("-")[1])'`
archs=`$last_py -c 'import sysconfig; print(sysconfig.get_platform().split("-")[2])'`
if test "$archs" != "universal2" ; then
  universal=no
fi

# Arrgh. Recent changes to nrn source require at least 10.15!
# macosver=10.15
mac_platform=macosx-$macosver-$archs

export MACOSX_DEPLOYMENT_TARGET=$macosver
echo "MACOSX_DEPLOYMENT_TARGET=$MACOSX_DEPLOYMENT_TARGET"

if test "$NRN_SRC" == "" ; then
  NRN_SRC=`pwd`
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

PYVS="py" # will be part of package file name, eg. py-38-39-310-311
pythons="" # will be arg value of NRN_PYTHON_DYNAMIC
for i in $args ; do
  PYVER=`$i -c 'from sys import version_info as v ; print (str(v.major) + str(v.minor)); quit()'`
  PYVS=${PYVS}-${PYVER}
  pythons="${pythons}${i};"
done

archs_cmake="" # arg for CMAKE_OSX_ARCHITECTURES, eg. arm64;x86_64
if test "$universal" = "yes" ; then
  archs_cmake='-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64'
fi

# The reason for the "-DCMAKE_PREFIX_PATH=/usr/X11" below
# is to definitely link against the xquartz.org installation instead
# of the one under /opt/homebrew/ (which I think came
# from brew install gedit). User installations are expected to have the
# former and would only accidentally have the latter.

cmake .. -G Ninja -DCMAKE_INSTALL_PREFIX=$NRN_INSTALL \
  -DNRN_ENABLE_MPI_DYNAMIC=ON \
  -DPYTHON_EXECUTABLE=`which python3` -DNRN_ENABLE_PYTHON_DYNAMIC=ON \
  -DNRN_PYTHON_DYNAMIC="$pythons" \
  -DIV_ENABLE_X11_DYNAMIC=ON \
  -DNRN_ENABLE_CORENEURON=OFF \
  -DNRN_RX3D_OPT_LEVEL=$rx3doptlevel \
  -DLINK_AGAINST_PYTHON=ON \
  -DNRN_MAC_PKG=ON \
  $archs_cmake \
  -DCMAKE_PREFIX_PATH=/usr/X11 \
  -DCMAKE_C_COMPILER=cc -DCMAKE_CXX_COMPILER=c++

ninja install

if test "$universal" = "yes" ; then
  _temp="`lipo -archs $NRN_INSTALL/share/nrn/demo/release/$CPU/special`"
  if test "$_temp" != "x86_64 arm64" ; then
    echo "universal build failed. lipo -archs .../special is \"$_temp\" instead of \"x86_64 arm64\""
    exit 1
  fi
fi
$NRN_INSTALL/bin/neurondemo -c 'quit()'

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

# test basic functionality
for i in $args ; do
  chk $i
done

#/Applications/Packages.app from
# http://s.sudre.free.fr/Software/Packages/about.html
# For mac to do a productsign, need my developerID_installer.cer
# and Neurondev.p12 file. To add to the keychain, double click each
# of those files. By default, I added my certificates to the login keychain.
ninja macpkg # will sign the binaries, construct below
            # mentioned PACKAGE_FILE_NAME, request notarization from
            # Apple, and staple the package.

# Copy the package to $HOME/$PACKAGE_FULL_NAME
# You should then manually upload that to github.
describe="`sh $NRN_SRC/nrnversion.sh describe`"
macos=macos${MACOSX_DEPLOYMENT_TARGET}
PACKAGE_FULL_NAME=nrn-${describe}-${mac_platform}-${PYVS}.pkg
PACKAGE_FILE_NAME=$NRN_BLD/src/mac/build/NEURON.pkg

cp $PACKAGE_FILE_NAME $HOME/$PACKAGE_FULL_NAME

echo "
  Manually upload $HOME/$PACKAGE_FULL_NAME to github
"
