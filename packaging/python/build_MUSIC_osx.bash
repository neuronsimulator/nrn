#!/usr/bin/env bash
set -xe
# A script to build MUSIC for osx
#
# PREREQUESITES:
#  - curl
#  - wget
#  - C/C++ compiler
#  - /opt/nrnwheel folder created with access rights (this specific path is kept for consistency wrt `build_wheels.bash`)

set -e

if [[ `uname -s` != 'Darwin' ]]; then
    echo "Error: this script is for macOS only."
    exit 1
fi

NRNWHEEL_DIR=/opt/nrnwheel
if [[ ! -d "$NRNWHEEL_DIR" || ! -x "$NRNWHEEL_DIR" ]]; then
    echo "Error: /opt/nrnwheel must exist and be accessible, i.e: sudo mkdir -p /opt/nrnwheel && sudo chown -R $USER /opt/nrnwheel"
    exit 1
fi

# Set MACOSX_DEPLOYMENT_TARGET based on wheel arch.
# For upcoming `universal2` wheels we will consider leveling everything to 11.0.
if [[ `uname -m` == 'arm64' ]]; then
	export MACOSX_DEPLOYMENT_TARGET=11.0  # for arm64 we need 11.0
else
	export MACOSX_DEPLOYMENT_TARGET=10.15  # for x86_64
fi

MUSIC_INSTALL_DIR=/opt/nrnwheel/MUSIC
MUSIC_VERSION=1.2.0

python3 -m venv music-venv
source music-venv/bin/activate
python3 -m pip install mpi4py cython numpy
mkdir -p $MUSIC_INSTALL_DIR
curl -L -o MUSIC.zip https://github.com/INCF/MUSIC/archive/refs/tags/${MUSIC_VERSION}.zip
unzip MUSIC.zip && mv MUSIC-* MUSIC && cd MUSIC
./autogen.sh
./configure --with-python-sys-prefix --prefix=$MUSIC_INSTALL_DIR --disable-anysource
make -j install
deactivate


# MUSIC_MINOS=`otool -l /opt/nrnwheel/MUSIC/lib/libmusic.dylib | grep -e "minos \|version " | uniq | awk '{print $2}'`

# if [ "$MUSIC_MINOS" != "$MACOSX_DEPLOYMENT_TARGET" ]; then 
# 	echo "Error: /opt/nrnwheel/MUSIC/lib/libmusic.dylib doesn't match MACOSX_DEPLOYMENT_TARGET ($MACOSX_DEPLOYMENT_TARGET)"
# 	exit 1
# fi

echo "Done." 
