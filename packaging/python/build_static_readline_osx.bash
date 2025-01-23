#!/usr/bin/env bash
set -xe
# A script to build a static readline library for osx
#
# PREREQUESITES:
#  - curl
#  - wget
#  - C/C++ compiler
#  - /opt/nrnwheel folder created with access rights (this specific path is kept for consistency wrt `build_wheels.bash`)

set -e

if [[ `uname -s` != 'Darwin' ]]; then
    echo "Error: this script is for macOS only. readline is already built statically in the linux Docker images"
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
	export MACOSX_DEPLOYMENT_TARGET=10.9  # for x86_64
fi

(wget http://ftpmirror.gnu.org/ncurses/ncurses-6.4.tar.gz \
    && tar -xvzf ncurses-6.4.tar.gz \
    && cd ncurses-6.4  \
    && ./configure --prefix=/opt/nrnwheel/ncurses --without-shared CFLAGS="-fPIC" \
    && make -j install)

(curl -L -o readline-7.0.tar.gz https://ftp.gnu.org/gnu/readline/readline-7.0.tar.gz \
    && tar -xvzf readline-7.0.tar.gz \
    && cd readline-7.0  \
    && ./configure --prefix=/opt/nrnwheel/readline --disable-shared CFLAGS="-fPIC" \
    && make -j install)

(cd /opt/nrnwheel/readline/lib \
    && ar -x libreadline.a \
    && ar -x ../../ncurses/lib/libncurses.a \
    && ar cq libreadline.a *.o \
    && rm *.o)

RDL_MINOS=`otool -l /opt/nrnwheel/readline/lib/libreadline.a | grep -e "minos \|version " | uniq | awk '{print $2}'`

if [ "$RDL_MINOS" != "$MACOSX_DEPLOYMENT_TARGET" ]; then 
	echo "Error: /opt/nrnwheel/readline/lib/libreadline.a doesn't match MACOSX_DEPLOYMENT_TARGET ($MACOSX_DEPLOYMENT_TARGET)"
	exit 1
fi

echo "Done." 
