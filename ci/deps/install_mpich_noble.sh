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

arch="$(dpkg --print-architecture)"
case "${arch}" in
  amd64)
    mpich_id="mpich-noble-4.2.0-5.1"
    lib_id="libmpich12-noble-4.2.0-5.1"
    mpich_deb="mpich_4.2.0-5.1_amd64.deb"
    lib_deb="libmpich12_4.2.0-5.1_amd64.deb"
    ;;
  arm64)
    mpich_id="mpich-noble-4.2.0-5.1-arm64"
    lib_id="libmpich12-noble-4.2.0-5.1-arm64"
    mpich_deb="mpich_4.2.0-5.1_arm64.deb"
    lib_deb="libmpich12_4.2.0-5.1_arm64.deb"
    ;;
  *)
    echo "error: unsupported architecture '${arch}' for pinned mpich (need amd64 or arm64)" >&2
    exit 1
    ;;
esac

WORKDIR="$(mktemp -d -t nrn-mpich-noble.XXXXXX)"
cleanup() { rm -rf "${WORKDIR}"; }
trap cleanup EXIT

# Prefer GitHub Release (ci-deps-vN). Do not fall through to Launchpad unless the
# caller sets NRN_CI_DEPS_SOURCE=upstream (or empty for local→release→upstream).
# Override with NRN_CI_DEPS_SOURCE=local if debugging with a gitignored assets/ copy.
export NRN_CI_DEPS_SOURCE="${NRN_CI_DEPS_SOURCE:-release}"

"${SCRIPT_DIR}/fetch.sh" "${mpich_id}" "${WORKDIR}"
"${SCRIPT_DIR}/fetch.sh" "${lib_id}" "${WORKDIR}"

# Ensure base packages exist (headers etc.) then overwrite with pinned debs.
apt-get install -y -qq mpich libmpich-dev || true
dpkg --install \
  "${WORKDIR}/${lib_deb}" \
  "${WORKDIR}/${mpich_deb}"

echo "installed pinned mpich 4.2.0-5.1 for Ubuntu 24.04 (${arch})"
mpichversion 2>/dev/null || true
mpirun.mpich --version 2>/dev/null || mpirun --version 2>/dev/null || true
