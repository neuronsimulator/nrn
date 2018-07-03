#!/bin/bash
set -ex
# distribution built with
#sh bldnrnmacpkg.sh python3.6 python

#10.7 possible if one builds with pythons that are consisent with that.
export MACOSX_DEPLOYMENT_TARGET=10.9

INST=/Applications/NEURON-7.6

if true ; then
  cd $HOME/neuron/iv
  make clean
  rm -r -f $INST
  ./configure --prefix=$INST/iv
  make
  make install
fi

cd $HOME/neuron/nrnobj

bld () {
rm -r -f src/nrnpython/build
../nrn/configure --prefix=$INST/nrn --with-paranrn=dynamic \
  --with-nrnpython=dynamic --with-pyexe=$1 $2
make -j 2 install
}

chk () {
  # Can launch python and import neuron
  # only needs PYTHONPATH
  (
    export PYTHONPATH=$INST/nrn/lib/python
    $1 -c 'from neuron import test; test()'
  )
  # Can launch nrniv -python and import neuron
  # needs NRN_PYLIB and perhaps PYTHONHOME
  (
    export PATH=$INST/nrn/x86_64/bin:$PATH
    eval "`nrnpyenv.sh $1`"
    nrniv -python -c "from neuron import test; test() ; quit()"
  )
  # Launching nrniv no longer needs NRN_PYLIB and PYTHONHOME
  (
    $INST/nrn/x86_64/bin/nrniv -python -pyexe $1 -c "import neuron ; neuron.test() ; quit()"
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

chk $HOME/anaconda3/bin/python3.6
chk $HOME/anaconda2/bin/python2.7


make after_install
#/Applications/Packages.app from
# http://s.sudre.free.fr/Software/Packages/about.html
# For mac to do a productsign, need my developerID_installer.cer
# and Neurondev.p12 file. To add to the keychain, double click each
# of those files. By default, I added my cerificate to the login keychain.
make pkg
make alphadist
