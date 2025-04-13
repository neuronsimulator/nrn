#!/usr/bin/env bash
set -eux
# A script to build a static universal2 readline library for macOS
#
# PREREQUISITES:
#  - curl
#  - C/C++ compiler
#  - /opt/nrnwheel/universal2 folder created with access rights

if [[ "$(uname -s)" != 'Darwin' ]]; then
    echo "Error: this script is for macOS only."
    exit 1
fi

LOC="/opt/nrnwheel"
ULOC="${LOC}/universal2"
mkdir -p "${ULOC}"

# Download packages
curl -L -o ncurses-6.4.tar.gz http://ftpmirror.gnu.org/ncurses/ncurses-6.4.tar.gz \
    && tar -xvzf ncurses-6.4.tar.gz
curl -L -o readline-7.0.tar.gz https://ftp.gnu.org/gnu/readline/readline-7.0.tar.gz \
    && tar -xvzf readline-7.0.tar.gz

# Set universal2 flags
export MACOSX_DEPLOYMENT_TARGET=11.0  # Required for universal2
export CFLAGS="-arch x86_64 -arch arm64 -fPIC"
export CXXFLAGS="-arch x86_64 -arch arm64 -fPIC"

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

# Verify universal2
RDL_ARCHS="$(lipo -info "${ULOC}/readline/lib/libreadline.a")"
if [[ ! "${RDL_ARCHS}" =~ "x86_64" || ! "${RDL_ARCHS}" =~ "arm64" ]]; then
    echo "Error: ${ULOC}/readline/lib/libreadline.a is not universal2"
    exit 1
fi

echo "Done."

