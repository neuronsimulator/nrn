#!/usr/bin/env bash
# Fetch a CI dependency listed in MANIFEST.yml into an output directory.
#
# Usage:
#   ci/deps/fetch.sh <asset-id> [output-dir]
#   ci/deps/fetch.sh --all [output-dir]
#   ci/deps/fetch.sh --list
#
# Resolution order (override with NRN_CI_DEPS_SOURCE=local|release|upstream):
#   local   → ci/deps/assets/<file>  (gitignored scratch; optional)
#   release → ${NRN_CI_DEPS_BASE_URL}/<file>  (or default_release_base_url in MANIFEST)
#   upstream→ upstream_url from MANIFEST
#
# Managed pins are published to a GitHub Release (see publish.sh); they are not
# committed to the nrn tree. Always verifies sha256 when the manifest provides one.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ASSETS_DIR="${NRN_CI_DEPS_ASSETS:-${SCRIPT_DIR}/assets}"
SOURCE_PREF="${NRN_CI_DEPS_SOURCE:-}"  # empty = try local, release, upstream
BASE_URL="${NRN_CI_DEPS_BASE_URL:-}"
MANIFEST_PY="${SCRIPT_DIR}/_manifest.py"

usage() {
  sed -n '2,16p' "$0" | sed 's/^# \{0,1\}//'
  exit 2
}

meta_get() {
  # usage: meta_get <key> <json>
  local key="$1"
  local json="$2"
  python3 -c 'import json,sys; d=json.load(sys.stdin); print(d.get(sys.argv[1]) or "")' "${key}" <<<"${json}"
}

load_asset_json() {
  python3 "${MANIFEST_PY}" get "$1"
}

sha256_file() {
  local f="$1"
  if command -v sha256sum >/dev/null 2>&1; then
    sha256sum "$f" | awk '{print $1}'
  else
    shasum -a 256 "$f" | awk '{print $1}'
  fi
}

verify_sha() {
  local f="$1" expected="$2"
  if [[ -z "${expected}" || "${expected}" == "null" ]]; then
    echo "warning: no sha256 in manifest for $(basename "$f"); skipping verify" >&2
    return 0
  fi
  local got
  got="$(sha256_file "$f")"
  if [[ "${got}" != "${expected}" ]]; then
    echo "error: sha256 mismatch for $(basename "$f")" >&2
    echo "  expected: ${expected}" >&2
    echo "  got:      ${got}" >&2
    return 1
  fi
}

download_url() {
  local url="$1" dest="$2"
  echo "downloading ${url}"
  if command -v curl >/dev/null 2>&1; then
    # No aggressive retry policy by design; self-host is the reliability fix.
    curl -fL --connect-timeout 30 --max-time 600 -o "${dest}" "${url}"
  elif command -v wget >/dev/null 2>&1; then
    wget -O "${dest}" "${url}"
  else
    echo "error: need curl or wget" >&2
    return 1
  fi
}

fetch_one() {
  local id="$1"
  local out_dir="$2"
  mkdir -p "${out_dir}"

  local meta file sha managed upstream default_base
  meta="$(load_asset_json "${id}")"
  file="$(meta_get file "${meta}")"
  sha="$(meta_get sha256 "${meta}")"
  managed="$(meta_get managed "${meta}")"
  upstream="$(meta_get upstream_url "${meta}")"
  default_base="$(meta_get _default_release_base_url "${meta}")"

  local dest="${out_dir}/${file}"
  local release_base="${BASE_URL:-${default_base}}"
  local local_path="${ASSETS_DIR}/${file}"

  try_local() {
    if [[ -f "${local_path}" ]]; then
      echo "using local ${local_path}"
      cp -f "${local_path}" "${dest}"
      verify_sha "${dest}" "${sha}"
      return 0
    fi
    return 1
  }

  try_release() {
    if [[ -z "${release_base}" ]]; then
      return 1
    fi
    local url="${release_base%/}/${file}"
    if download_url "${url}" "${dest}"; then
      verify_sha "${dest}" "${sha}"
      return 0
    fi
    rm -f "${dest}"
    return 1
  }

  try_upstream() {
    if [[ -z "${upstream}" ]]; then
      return 1
    fi
    if download_url "${upstream}" "${dest}"; then
      verify_sha "${dest}" "${sha}"
      return 0
    fi
    rm -f "${dest}"
    return 1
  }

  case "${SOURCE_PREF}" in
    local)
      try_local || { echo "error: local asset missing: ${local_path}" >&2; return 1; }
      ;;
    release)
      try_release || { echo "error: release fetch failed for ${id}" >&2; return 1; }
      ;;
    upstream)
      try_upstream || { echo "error: upstream fetch failed for ${id}" >&2; return 1; }
      ;;
    "")
      if try_local; then
        :
      elif try_release; then
        :
      elif try_upstream; then
        echo "warning: used upstream for ${id} (managed=${managed}); prefer managed local/release copy" >&2
      else
        echo "error: could not fetch ${id} from local, release, or upstream" >&2
        return 1
      fi
      ;;
    *)
      echo "error: unknown NRN_CI_DEPS_SOURCE=${SOURCE_PREF}" >&2
      return 1
      ;;
  esac

  echo "ok ${id} -> ${dest}"
}

# --- main ---
if [[ $# -lt 1 ]]; then
  usage
fi

case "$1" in
  --list|-l)
    python3 "${MANIFEST_PY}" list
    exit 0
    ;;
  --all)
    out_dir="${2:-${ASSETS_DIR}}"
    while IFS= read -r id; do
      fetch_one "${id}" "${out_dir}"
    done < <(python3 "${MANIFEST_PY}" ids)
    exit 0
    ;;
  -h|--help)
    usage
    ;;
  *)
    id="$1"
    out_dir="${2:-${ASSETS_DIR}}"
    fetch_one "${id}" "${out_dir}"
    ;;
esac
