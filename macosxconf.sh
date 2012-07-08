#!/bin/sh

# assume InterViews sources in $HOME/neuron/iv
# assume NEURON sources in $HOME/neuron/nrn
# assume that you are using a Terminal window

SOFT=$HOME/neuron

# if configure does not exist in $SOFT/iv or $SOFT/nrn, then in those directories, execute
# sh build.sh

nver=`./nrnversion.sh`
echo $nver
exit 0

IDIR="/Applications/NEURON-${nver}"


#for Lion osx 10.7
# 64 bit, icons work, python available. If desire MPI then should install openmpi first.

# xcode command line tools must be installed. Two ways to do this.
#First way: From https://developer.apple.com/xcode one can get a free version
# of Xcode 4.3.3 for Lion from the Mac App Store.
# After installing, launch Xcode 4 and menu navigate to  Xcode Preferences and in the popup window
# install the "Command Line Tools".
#Second way: To avoid downloading the entire Xcode4 installer, which is about 4GB in size, one can
# install the much smaller (170 MB dmg) "Command Line Tools" separately from
# https://developer.apple.com (search there for "Command Line Tools for Xcode"). You will have to login
# with your apple id.


cd $SOFT/iv
./configure --prefix=$IDIR/iv
make
make install

cd $SOFT/nrn
../nrn/configure --prefix=$IDIR/nrn --with-iv=$IDIR/iv --with-nrnpython=dynamic
make
make install
#note:
make after_install
make dmg
#is used to create a distributed version with neurondemo and (for small dmg) stripped executables.

# if mpi desired, install openmpi, make sure mpicc and mpicxx is in your PATH and add --with-paranrn
# to the ../nrn/configure line. e.g.
# from http://www.open-mpi.org go to Download page and get the current stable release tar.gz
# after extracting, do the ./configure ; make; make install

#in order to get single click button action for x11 when entering a window,
# determine if you are using org.macosforge.xquartz.X11
# or org.x.X11 with
defaults read | grep X11
#depending on the result, then one or both of the following:
defaults write org.x.X11 wm_ffm -bool true
defaults write org.macosforge.xquartz.X11 wm_ffm -bool true

#for snow leopard osx 10.6
# Same as osx 10.7 except the latest Xcode is Xcode which automatically installs the command line tools.
# I installed xcode_3.2.6_and_ios_sdk_4.3.dmg
# Use the following configure lines to get a 64 bit version with working icons and python.
# I believe mpi is already present under osx 10.6 so it may be worth trying --with-paranrn without
# having to install openmpi (but I'm not sure if the existing is a 32 or 64 bit version of mpi).
# notice that by default, 32 bit versions are built so a few configure extra configure options are needed
# to build a 64 bit version.
#in $SOFT/iv
./configure --prefix=$IDIR/iv CFLAGS='-arch x86_64' CXXFLAGS='-arch x86_64' --host=x86_64-apple-darwin10.8.0

#in $SOFT/nrn
../nrn/configure --prefix=$IDIR/nrn --with-iv=$IDIR/iv --with-nrnpython=dynamic \
  CFLAGS='-arch x86_64' CXXFLAGS='-arch x86_64' --host=x86_64-apple-darwin10.8.0
