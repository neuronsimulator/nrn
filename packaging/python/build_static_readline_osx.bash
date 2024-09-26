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

LOC="/opt/nrnwheel"

# Download packages
curl -L -o ncurses-6.4.tar.gz http://ftpmirror.gnu.org/ncurses/ncurses-6.4.tar.gz \
    && tar -xvzf ncurses-6.4.tar.gz
curl -L -o readline-7.0.tar.gz https://ftp.gnu.org/gnu/readline/readline-7.0.tar.gz \
    && tar -xvzf readline-7.0.tar.gz


for ARCH in arm64 x86_64 ; do # down to done # for ARCH

NRNWHEEL_DIR="/${LOC}/${ARCH}"
mkdir -p $NRNWHEEL_DIR

# Set MACOSX_DEPLOYMENT_TARGET based on wheel arch.
# For upcoming `universal2` wheels we will consider leveling everything to 11.0.
if [[ "${ARCH}" == 'arm64' ]]; then
	export MACOSX_DEPLOYMENT_TARGET=11.0  # for arm64 we need 11.0
else
	export MACOSX_DEPLOYMENT_TARGET=10.9  # for x86_64
fi

cfl="-fPIC -arch ${ARCH}"

(cd ncurses-6.4  \
    && ./configure --prefix="${NRNWHEEL_DIR}/ncurses" --without-shared CFLAGS="$cfl" CXXFLAGS="$cfl"\
    && make clean \
    && make -j install)

(cd readline-7.0  \
    && ./configure --prefix="${NRNWHEEL_DIR}/readline" --disable-shared CFLAGS="$cfl" CXXFLAGS="$cfl" \
    && make clean \
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

done # for ARCH

# combine libraries into universal2

LOC=/opt/nrnwheel
XLOC=$LOC/x86_64
ALOC=$LOC/arm64
ULOC=$LOC/universal2
mkdir -p $ULOC/ncurses/lib
mkdir -p $ULOC/readline/lib

libs="`(cd $ALOC ; find . -name \*.a)`"

for lib in $libs ; do
    lipo -create -output $ULOC/$lib $ALOC/$lib $XLOC/$lib
done

# The executables are not universal2. Copy the universal libraries over
# the native architecture libraries.
native="$(uname -m)"
NLOC=$LOC/$native
for lib in $libs ; do
    cp $ULOC/$lib $NLOC/$lib
done

echo "Done." 
