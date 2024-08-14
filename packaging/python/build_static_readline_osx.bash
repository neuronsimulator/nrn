#!/usr/bin/env bash
set -eux
# A script to build a static readline library for osx
#
# PREREQUESITES:
#  - curl
#  - C/C++ compiler
#  - /opt/nrnwheel/[ARCH] folder created with access rights (this specific path is kept for consistency wrt `build_wheels.bash`)

if [[ "$(uname -s)" != 'Darwin' ]]; then
    echo "Error: this script is for macOS only. readline is already built statically in the linux Docker images"
    exit 1
fi

ARCH="$(uname -m)"

NRNWHEEL_DIR="${1:-/opt/nrnwheel/${ARCH}}"
if [[ ! -d "$NRNWHEEL_DIR" || ! -x "$NRNWHEEL_DIR" ]]; then
    echo "Error: ${NRNWHEEL_DIR} must exist and be accessible, i.e: sudo mkdir -p ${NRNWHEEL_DIR} && sudo chown -R ${USER} ${NRNWHEEL_DIR}"
    exit 1
fi

# Set MACOSX_DEPLOYMENT_TARGET based on wheel arch.
# For upcoming `universal2` wheels we will consider leveling everything to 11.0.
if [[ "${ARCH}" == 'arm64' ]]; then
	export MACOSX_DEPLOYMENT_TARGET=11.0  # for arm64 we need 11.0
else
	export MACOSX_DEPLOYMENT_TARGET=10.9  # for x86_64
fi

(curl -L -o ncurses-6.4.tar.gz http://ftpmirror.gnu.org/ncurses/ncurses-6.4.tar.gz \
    && tar -xvzf ncurses-6.4.tar.gz \
    && cd ncurses-6.4  \
    && ./configure --prefix="${NRNWHEEL_DIR}/ncurses" --without-shared CFLAGS="-fPIC" \
    && make -j install)

(curl -L -o readline-7.0.tar.gz https://ftp.gnu.org/gnu/readline/readline-7.0.tar.gz \
    && tar -xvzf readline-7.0.tar.gz \
    && cd readline-7.0  \
    && ./configure --prefix="${NRNWHEEL_DIR}/readline" --disable-shared CFLAGS="-fPIC" \
    && make -j install)

(cd "${NRNWHEEL_DIR}/readline/lib" \
    && ar -x libreadline.a \
    && ar -x ../../ncurses/lib/libncurses.a \
    && ar cq libreadline.a *.o \
    && rm *.o)

RDL_MINOS="$(otool -l "${NRNWHEEL_DIR}/readline/lib/libreadline.a" | grep -e "minos \|version " | uniq | awk '{print $2}')"

if [ "$RDL_MINOS" != "$MACOSX_DEPLOYMENT_TARGET" ]; then 
	echo "Error: ${NRNWHEEL_DIR}/readline/lib/libreadline.a doesn't match MACOSX_DEPLOYMENT_TARGET ($MACOSX_DEPLOYMENT_TARGET)"
	exit 1
fi
echo "Done." 
