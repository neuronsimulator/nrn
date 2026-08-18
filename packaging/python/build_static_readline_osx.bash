#!/usr/bin/env bash
set -eux
# A script to build a static readline library for osx
#
# PREREQUESITES:
#  - curl
#  - C/C++ compiler
#  - /opt/nrnwheel/[ARCH] folder created with access rights (this specific path is kept for consistency wrt `build_wheels.bash`)
#  - repo checkout including ci/deps (fetches ncurses/readline from nrn-ci-deps Release ci-deps-v1)

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

# packaging/python -> repo root
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
FETCH="${REPO_ROOT}/ci/deps/fetch.sh"
WORKDIR="$(mktemp -d -t nrn-readline-src.XXXXXX)"
cleanup() { rm -rf "${WORKDIR}"; }
trap cleanup EXIT

export NRN_CI_DEPS_SOURCE="${NRN_CI_DEPS_SOURCE:-release}"
bash "${FETCH}" ncurses-6.4-src "${WORKDIR}"
# GNU readline 8.3 is OK for Mac wheels now that hoc.cpp always uses
# getc_hook (not rl_event_hook) with InterViews. Linux Docker stays on 7.0.
bash "${FETCH}" readline-8.3-src "${WORKDIR}"

(
  tar -xzf "${WORKDIR}/ncurses-6.4.tar.gz" -C "${WORKDIR}"
  cd "${WORKDIR}/ncurses-6.4"
  ./configure --prefix="${NRNWHEEL_DIR}/ncurses" --without-shared CFLAGS="-fPIC"
  make -j install
)

(
  tar -xzf "${WORKDIR}/readline-8.3.tar.gz" -C "${WORKDIR}"
  cd "${WORKDIR}/readline-8.3"
  ./configure --prefix="${NRNWHEEL_DIR}/readline" --disable-shared CFLAGS="-fPIC"
  make -j install
)

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
