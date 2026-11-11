#!/usr/bin/env bash
set -eux
# Build a static ncurses for macOS wheels. CMake fetches BSD libedit and links
# this archive; GNU readline is not built.
#
# PREREQUESITES:
#  - curl
#  - C/C++ compiler
#  - /opt/nrnwheel/[ARCH] folder created with access rights (this specific path is kept for consistency wrt `build_wheels.bash`)
#  - repo checkout including ci/deps (fetches ncurses from nrn-ci-deps Release ci-deps-v1)

if [[ "$(uname -s)" != 'Darwin' ]]; then
    echo "Error: this script is for macOS only. ncurses is already built statically in the linux Docker images"
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
WORKDIR="$(mktemp -d -t nrn-ncurses-src.XXXXXX)"
cleanup() { rm -rf "${WORKDIR}"; }
trap cleanup EXIT

export NRN_CI_DEPS_SOURCE="${NRN_CI_DEPS_SOURCE:-release}"
bash "${FETCH}" ncurses-6.4-src "${WORKDIR}"

(
  tar -xzf "${WORKDIR}/ncurses-6.4.tar.gz" -C "${WORKDIR}"
  cd "${WORKDIR}/ncurses-6.4"
  ./configure --prefix="${NRNWHEEL_DIR}/ncurses" --without-shared CFLAGS="-fPIC"
  make -j install
)

NCURSES_MINOS="$(otool -l "${NRNWHEEL_DIR}/ncurses/lib/libncurses.a" | grep -e "minos \|version " | uniq | awk '{print $2}')"

if [ "$NCURSES_MINOS" != "$MACOSX_DEPLOYMENT_TARGET" ]; then
	echo "Error: ${NRNWHEEL_DIR}/ncurses/lib/libncurses.a doesn't match MACOSX_DEPLOYMENT_TARGET ($MACOSX_DEPLOYMENT_TARGET)"
	exit 1
fi
echo "Done."
