#!/bin/bash
set -e
# rpm and deb distribution built with
#sh bldnrnlinuxdeb.sh
# works for user installation of python2.7, python3.5, python3.6
# and python3.7

INST=/usr/local/nrn
objdir=$HOME/neuron/nrnrpm

cd $objdir
sudo rm -r -f $INST
sudo rm -r -f $objdir/*

bld () {
  ../nrn/configure --prefix=$INST --with-paranrn=dynamic \
    --with-nrnpython=dynamic --with-pyexe=$1 --enable-rpm-rules
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
    $1 -c 'from neuron.tests import test_rxd; test_rxd.test(); quit()'
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
    $INST/x86_64/bin/nrniv -python -pyexe $1 -c 'from neuron.tests import test_rxd; test_rxd.test(); quit()'
  )
}

PYVS=py-37-36-35-27

bld python3.7 ""
chk python3.7
bld python3.6 ""
chk python3.6
bld python3.5 "--with-nrnpython-only"
chk python3.5
bld python2 "--with-nrnpython-only"
chk python2
sudo $INST/x86_64/bin/neurondemo -c 'quit()'

if false ; then
cd $INST/lib/python/neuron
sudo cp hoc.cpython-35m-x86_64-linux-gnu.so hoc.cpython-36m-x86_64-linux-gnu.so
sudo cp hoc.cpython-35m-x86_64-linux-gnu.so hoc.cpython-37m-x86_64-linux-gnu.so
cd rxd/geometry3d
for i in ctng surfaces graphicsPrimitives ; do
  sudo cp $i.cpython-35m-x86_64-linux-gnu.so $i.cpython-36m-x86_64-linux-gnu.so
  sudo cp $i.cpython-35m-x86_64-linux-gnu.so $i.cpython-37m-x86_64-linux-gnu.so
done
cd $objdir
fi

if true ; then
  sudo make rpm

  #make rpmdist
  #following requires cwd to be the build directory because of NSRC
  RPM_TARGET=/home/hines/rpmbuild/RPMS/x86_64
  ALPHADIR='hines@neuron.yale.edu:/home/htdocs/ftp/neuron/versions/alpha'
  export NSRC=../nrn
  describe="`sh $NSRC/nrnversion.sh describe`"
  NVER="`sh $NSRC/nrnversion.sh 3`"
  for i in rpm deb ; do
    a=${ALPHADIR}/nrn-${describe}.x86_64-linux-${PYVS}.$i
    b=${RPM_TARGET}/nrn*.$i
    echo "scp $b $a"
    scp $b $a
  done
fi
