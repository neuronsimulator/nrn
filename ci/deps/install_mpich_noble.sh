#!/usr/bin/env bash
# Install pinned MPICH packages for Ubuntu 24.04 (noble) wheel testing.
#
# Background: distro mpich on noble was broken (LP#2072338). CI needs a working
# mpich alongside openmpi to exercise NRN_ENABLE_MPI_DYNAMIC in packaging/python/test_wheels.sh.
#
# Uses ci/deps/fetch.sh so Launchpad is not on the critical path when assets are managed.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [[ -f /etc/os-release ]]; then
  # shellcheck source=/dev/null
  source /etc/os-release
else
  echo "error: /etc/os-release not found" >&2
  exit 1
fi

if [[ "${ID:-}" != "ubuntu" || "${VERSION_ID:-}" != "24.04" ]]; then
  echo "skip install_mpich_noble: not Ubuntu 24.04 (${ID:-unknown} ${VERSION_ID:-unknown})"
  exit 0
fi

if [[ "$(id -u)" -ne 0 ]]; then
  if command -v sudo >/dev/null 2>&1; then
    exec sudo -E bash "$0" "$@"
  fi
  echo "error: must run as root or with sudo" >&2
  exit 1
fi

WORKDIR="$(mktemp -d -t nrn-mpich-noble.XXXXXX)"
cleanup() { rm -rf "${WORKDIR}"; }
trap cleanup EXIT

# Prefer local managed assets; do not fall through to Launchpad on CI unless forced.
export NRN_CI_DEPS_SOURCE="${NRN_CI_DEPS_SOURCE:-local}"

"${SCRIPT_DIR}/fetch.sh" mpich-noble-4.2.0-5.1 "${WORKDIR}"
"${SCRIPT_DIR}/fetch.sh" libmpich12-noble-4.2.0-5.1 "${WORKDIR}"

# Ensure base packages exist (headers etc.) then overwrite with pinned debs.
apt-get install -y -qq mpich libmpich-dev || true
dpkg --install \
  "${WORKDIR}/libmpich12_4.2.0-5.1_amd64.deb" \
  "${WORKDIR}/mpich_4.2.0-5.1_amd64.deb"

echo "installed pinned mpich 4.2.0-5.1 for Ubuntu 24.04"
mpichversion 2>/dev/null || true
mpirun.mpich --version 2>/dev/null || mpirun --version 2>/dev/null || true
