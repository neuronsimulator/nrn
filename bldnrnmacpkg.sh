#!/bin/bash
set -ex
# distribution built with
#sh bldnrnmacpkg.sh python3.7 python3.6 python2.7

#10.7 possible if one builds with pythons that are consisent with that.
export MACOSX_DEPLOYMENT_TARGET=10.9

INST=/Applications/NEURON-7.7
export PATH=$INST/nrn/x86_64/bin:$PATH

if false ; then
  cd $HOME/neuron/iv
  make clean
  rm -r -f $INST
  ./configure --prefix=$INST/iv
  make
  make install
fi

rm -r -f $INST/nrn
cd $HOME/neuron/nrnobj
rm -r -f $HOME/neuron/nrnobj/*

PYVS="py"

bld () {
PYVER=`$1 -c 'from sys import version_info as v ; print (str(v.major) + str(v.minor)); quit()'`
PYVS=${PYVS}-${PYVER}
rm -r -f src/nrnpython/build
../nrn/configure --prefix=$INST/nrn --with-paranrn=dynamic \
  --with-nrnpython=dynamic --with-pyexe=$1 --with-readline=no #$2
make -j 2 install
}

chk () {
  # Can launch python and import neuron
  # only needs PYTHONPATH
  (
    export PYTHONPATH=$INST/nrn/lib/python
    $1 -c 'from neuron import test; test()'
    $1 -c 'from neuron.tests import test_rxd; test_rxd.test(); quit()'
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
    $INST/nrn/x86_64/bin/nrniv -python -pyexe $1 -c 'from neuron.tests import test_rxd; test_rxd.test(); quit()'
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

#chk $HOME/anaconda3/bin/python3.6
#chk $HOME/anaconda2/bin/python2.7


make after_install
#/Applications/Packages.app from
# http://s.sudre.free.fr/Software/Packages/about.html
# For mac to do a productsign, need my developerID_installer.cer
# and Neurondev.p12 file. To add to the keychain, double click each
# of those files. By default, I added my cerificate to the login keychain.
make pkg

#make alphadist
#following requires cwd to be the build directory because of NSRC
ALPHADIR='hines@neuron.yale.edu:/home/htdocs/ftp/neuron/versions/alpha'
export NSRC=../nrn
describe="`sh $NSRC/nrnversion.sh describe`"
NVER="`sh $NSRC/nrnversion.sh 3`"
a=${ALPHADIR}/nrn-${describe}.x86_64-osx-${PYVS}.pkg
b=./src/mac/build/NEURON-${NVER}.pkg
echo "scp $b $a"
scp $b $a

