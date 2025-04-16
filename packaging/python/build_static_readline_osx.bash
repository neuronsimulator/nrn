#!/usr/bin/env bash
set -eux
# A script to build a static readline library for macOS
# universal2 or x86_64 or arm64
#
# PREREQUISITES:
#  - curl
#  - C/C++ compiler
#  - /opt/nrnwheel folder created with access rights

if [[ "$(uname -s)" != 'Darwin' ]]; then
    echo "Error: this script is for macOS only."
    exit 1
fi

if [ "$ARCHTYPE" == "" ]; then
    echo "Error ARCHTYPE must exist as universal2 or x86_64 or arm64."
    exit 1
fi

if [ "$ARCHFLAGS" == "" ]; then
    echo "Error: ARCHFLAGS must exist."
    exit 1
fi

LOC="/opt/nrnwheel"
ULOC="${LOC}/${ARCHTYPE}"
mkdir -p "${ULOC}"

# Download packages
curl -L -o ncurses-6.4.tar.gz http://ftpmirror.gnu.org/ncurses/ncurses-6.4.tar.gz \
    && tar -xvzf ncurses-6.4.tar.gz
curl -L -o readline-7.0.tar.gz https://ftp.gnu.org/gnu/readline/readline-7.0.tar.gz \
    && tar -xvzf readline-7.0.tar.gz

# Set  flags
export CFLAGS="$ARCHFLAGS -fPIC"
export CXXFLAGS="$ARCHFLAGS -fPIC"
if [ "ARCHFLAGS" == "universal2" ]; then
    export MACOSX_DEPLOYMENT_TARGET=11.0  # Required for universal2?
    hostarg=""
elif [ "ARCHFLAGS" == "x86_64" ]; then
    export MACOSX_DEPLOYMENT_TARGET=10.15
    hostarg="--nhost=x86_64-apple-darwin"
elif [ "ARCHFLAGS" == "arm64" ]; then
    export MACOSX_DEPLOYMENT_TARGET=10.15
    hostarg="--nhost=aarch64-apple-darwin"
fi

# Build ncurses (static only, no executables)
(cd ncurses-6.4 \
    && ./configure --prefix="${ULOC}/ncurses" --without-shared --without-tests \
    && make clean \
    && make -j \
    && make install)

# Build readline (static only)
(cd readline-7.0 \
    && ./configure --prefix="${ULOC}/readline" --disable-shared \
    && make clean \
    && make -j \
    && make install)

# Combine readline and ncurses static libraries with libtool
(cd "${ULOC}/readline/lib" \
    && mv libreadline.a libreadline_orig.a \
    && libtool -static -o libreadline.a libreadline_orig.a ../../ncurses/lib/libncurses.a \
    && rm libreadline_orig.a)

# Verify universal2
if [ "$ARCHTYPE" == "universal2" ]; then
    RDL_ARCHS="$(lipo -info "${ULOC}/readline/lib/libreadline.a")"
    if [[ ! "${RDL_ARCHS}" =~ "x86_64" || ! "${RDL_ARCHS}" =~ "arm64" ]]; then
        echo "Error: ${ULOC}/readline/lib/libreadline.a is not universal2"
        exit 1
    fi
fi

echo "Done."

