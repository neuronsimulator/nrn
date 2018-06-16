#!/bin/bash
set -e
# rpm and deb distribution built with
#sh bldnrnlinuxdeb.sh
# works for user installation of python2.7, python3.5, and python3.6

INST=/usr/local/nrn
objdir=$HOME/neuron/nrnrpm

cd $objdir
sudo rm -r -f $INST
sudo rm -r -f $objdir/*

bld () {
  ../nrn/configure --prefix=$INST --with-paranrn=dynamic \
    --with-nrnpython=dynamic --with-pyexe=$1 $2 --enable-rpm-rules
  sudo rm -r -f src/nrnpython/build
  make -j 6
  sudo make -j 6 install
}

chk () {
  # Can launch python and import neuron
  # only needs PYTHONPATH
  (
    export PYTHONPATH=$INST/lib/python
    $1 -c 'from neuron import test; test()'
  )
  # Can launch nrniv -python and import neuron
  # needs NRN_PYLIB and perhaps PYTHONHOME
  (
    export PATH=$INST/x86_64/bin:$PATH
    eval "`nrnpyenv.sh $1`"
    nrniv -python -c "from neuron import test; test() ; quit()"
  )
  # Launching nrniv no longer needs NRN_PYLIB and PYTHONHOME
  (
    $INST/x86_64/bin/nrniv -python -pyexe $1 -c "import neuron ; neuron.test() ; quit()"
  )
}

bld python3 ""
sudo $INST/x86_64/bin/neurondemo -c 'quit()'
chk python3
bld python "--with-nrnpython-only"
chk python

cd $INST/lib/python/neuron
sudo cp hoc.cpython-35m-x86_64-linux-gnu.so hoc.cpython-36m-x86_64-linux-gnu.so
cd rxd/geometry3d
sudo cp surfaces.cpython-35m-x86_64-linux-gnu.so surfaces.cpython-36m-x86_64-linux-gnu.so
cd $objdir

if true ; then
  sudo make rpm
  make rpmdist
fi
