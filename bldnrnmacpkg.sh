#!/bin/bash
set -e
# distribution built with
# sh bldnrnmacpkg.sh python3 /Volumes/HinesWD/mac/anaconda3/bin/python python
#sh bldnrnmac.sh python3 /Volumes/HinesWD/mac/anaconda3/bin/python /Volumes/HinesWD/mac/anaconda2/bin/python

INST=/Applications/NEURON-7.6/nrn
cd $HOME/neuron/nrnobj

export MACOSX_DEPLOYMENT_TARGET=10.7

bld () {
rm -r -f src/nrnpython/build
../nrn/configure --prefix=$INST --with-paranrn=dynamic \
  --with-nrnpython=dynamic --with-pyexe=$1 $2 CYTHON=/Volumes/HinesWD/mac/anaconda2/bin/cython
make -j 2 install
}

bld $1 ""
shift
for i in $* ; do
  bld $i "--with-nrnpython-only"
done

make after_install
#/Applications/Packages.app from
# http://s.sudre.free.fr/Software/Packages/about.html
# For mac to do a productsign, need my developerID_installer.cer
# and Neurondev.p12 file. To add to the keychain, double click each
# of those files. By default, I added my cerificate to the login keychain.
make pkg
make alphadist


