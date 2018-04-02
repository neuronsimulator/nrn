#!/bin/bash
set -e
# rpm and deb distribution built with
#sh bldnrnlinuxdeb.sh
# works for user installation of python2.7, python3.5, and python3.6

INST=/usr/local/nrn
objdir=$HOME/neuron/nrnrpm

cd $objdir

bld () {
../nrn/configure --prefix=$INST --with-paranrn=dynamic \
  --with-nrnpython=dynamic --with-pyexe=$1 $2 --enable-rpm-rules
sudo rm -r -f src/nrnpython/build
make -j 6
sudo make -j 6 install
}

bld python3 ""
bld python "--with-nrnpython-only"

cd $INST/lib/python/neuron
sudo cp hoc.cpython-35m-x86_64-linux-gnu.so hoc.cpython-36m-x86_64-linux-gnu.so
cd $objdir

sudo make rpm
make rpmdist

