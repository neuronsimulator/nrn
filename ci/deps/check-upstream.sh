#!/usr/bin/env bash
# Compare managed / manifest pins against authoritative upstream_url.
#
# Usage:
#   ci/deps/check-upstream.sh           # all assets with upstream_url + sha256
#   ci/deps/check-upstream.sh <id> ...  # selected ids
#
# Exit codes:
#   0 — all checked assets match (or were skipped: no sha / no upstream)
#   1 — at least one mismatch or download failure
#   2 — usage / environment error
#
# Does not auto-upgrade. Prints a short report for humans / CI issue openers.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MANIFEST_PY="${SCRIPT_DIR}/_manifest.py"
WORKDIR="${NRN_CI_DEPS_CHECK_DIR:-$(mktemp -d -t nrn-ci-deps-check.XXXXXX)}"
KEEP_WORKDIR="${NRN_CI_DEPS_KEEP_CHECK_DIR:-0}"

cleanup() {
  if [[ "${KEEP_WORKDIR}" != "1" && -d "${WORKDIR}" && "${WORKDIR}" == *nrn-ci-deps-check* ]]; then
    rm -rf "${WORKDIR}"
  fi
}
trap cleanup EXIT

mkdir -p "${WORKDIR}"

IDS=()
if [[ $# -gt 0 ]]; then
  IDS=("$@")
else
  while IFS= read -r id; do
    IDS+=("$id")
  done < <(python3 "${MANIFEST_PY}" ids)
fi

if [[ ${#IDS[@]} -eq 0 ]]; then
  echo "nothing to check"
  exit 0
fi

mismatches=0
skipped=0
ok=0

meta_get() {
  # usage: meta_get <key> <json>
  local key="$1"
  local json="$2"
  python3 -c 'import json,sys; d=json.load(sys.stdin); print(d.get(sys.argv[1]) or "")' "${key}" <<<"${json}"
}

for id in "${IDS[@]}"; do
  meta="$(python3 "${MANIFEST_PY}" get "${id}")"

  file="$(meta_get file "${meta}")"
  sha="$(meta_get sha256 "${meta}")"
  upstream="$(meta_get upstream_url "${meta}")"
  managed="$(meta_get managed "${meta}")"

  if [[ -z "${upstream}" ]]; then
    echo "SKIP ${id}: no upstream_url"
    skipped=$((skipped + 1))
    continue
  fi
  if [[ -z "${sha}" ]]; then
    echo "SKIP ${id}: no sha256 in manifest"
    skipped=$((skipped + 1))
    continue
  fi

  dest="${WORKDIR}/${file}"
  echo "CHECK ${id} (managed=${managed})"
  echo "  upstream: ${upstream}"
  if ! curl -fL --connect-timeout 30 --max-time 600 -o "${dest}" "${upstream}"; then
    echo "  FAIL download"
    mismatches=$((mismatches + 1))
    continue
  fi
  if command -v sha256sum >/dev/null 2>&1; then
    got="$(sha256sum "${dest}" | awk '{print $1}')"
  else
    got="$(shasum -a 256 "${dest}" | awk '{print $1}')"
  fi
  if [[ "${got}" == "${sha}" ]]; then
    echo "  OK matches manifest sha256"
    ok=$((ok + 1))
  else
    echo "  STALE or changed"
    echo "    manifest: ${sha}"
    echo "    upstream: ${got}"
    mismatches=$((mismatches + 1))
  fi
done

echo
echo "summary: ok=${ok} mismatch_or_fail=${mismatches} skipped=${skipped}"
if [[ "${mismatches}" -gt 0 ]]; then
  exit 1
fi
exit 0
