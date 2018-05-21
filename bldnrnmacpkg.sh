#!/bin/bash
set -ex
# distribution built with
# sh bldnrnmacpkg.sh python3 /Volumes/HinesWD/mac/anaconda3/bin/python python
#sh bldnrnmac.sh python3 /Volumes/HinesWD/mac/anaconda3/bin/python /Volumes/HinesWD/mac/anaconda2/bin/python

INST=/Applications/NEURON-7.6/nrn
cd $HOME/neuron/nrnobj

export MACOSX_DEPLOYMENT_TARGET=10.7

bld () {
rm -r -f src/nrnpython/build
../nrn/configure --prefix=$INST --with-paranrn=dynamic \
  --with-nrnpython=dynamic --with-pyexe=$1 $2
make -j 2 install
}

chk () {
  (
    export PYTHONPATH=$INST/lib/python
    $1 -c 'from neuron import test; test()'
  )
  (
    export PATH=$INST/x86_64/bin:$PATH
    eval "`nrnpyenv.sh $1`"
    nrniv -python -c "from neuron import test; test() ; quit()"
  )
}

# no args, use standard python
if test "$1" = "" ; then
  first=python
else
  first=$1
  shift
fi

bld $first ""
chk $first
for i in $* ; do
  bld $i "--with-nrnpython-only"
  chk $i
done

chk /Volumes/HinesWD/mac/anaconda3/bin/python3.6
chk /Users/hines/anaconda2/bin/python2.7
chk /Volumes/HinesWD/mac/anaconda3/envs/py35/bin/python3.5


make after_install
#/Applications/Packages.app from
# http://s.sudre.free.fr/Software/Packages/about.html
# For mac to do a productsign, need my developerID_installer.cer
# and Neurondev.p12 file. To add to the keychain, double click each
# of those files. By default, I added my cerificate to the login keychain.
make pkg
make alphadist
