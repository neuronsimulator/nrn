#!/usr/bin/env bash
# Publish files from a local directory (default: ci/deps/assets/, gitignored) to
# a GitHub Release on the CI-deps archive repo (not the NEURON product repo).
#
# Usage:
#   mkdir -p ci/deps/assets && cp /path/to/*.deb ci/deps/assets/
#   ci/deps/publish.sh [--repo owner/name] [--tag ci-deps-v1] [--dry-run]
#   rm -f ci/deps/assets/*   # optional cleanup; directory is gitignored
#
# Requires: gh (authenticated).
# Default --repo is neuronsimulator/nrn-ci-deps so pins do not appear under
# neuronsimulator/nrn product Releases.
#
# After publishing, ensure MANIFEST default_release_base_url (or NRN_CI_DEPS_BASE_URL)
# points at:
#   https://github.com/<owner>/<name>/releases/download/<tag>
#
# CI fetch defaults to release (see install helpers), not in-repo files.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ASSETS_DIR="${NRN_CI_DEPS_ASSETS:-${SCRIPT_DIR}/assets}"
REPO="${NRN_CI_DEPS_PUBLISH_REPO:-neuronsimulator/nrn-ci-deps}"
TAG="${NRN_CI_DEPS_PUBLISH_TAG:-ci-deps-v1}"
DRY_RUN=0
TITLE="NEURON CI dependency archive (${TAG})"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --repo) REPO="$2"; shift 2 ;;
    --tag) TAG="$2"; shift 2 ;;
    --dry-run) DRY_RUN=1; shift ;;
    -h|--help)
      sed -n '2,14p' "$0" | sed 's/^# \{0,1\}//'
      exit 0
      ;;
    *) echo "unknown arg: $1" >&2; exit 2 ;;
  esac
done

# REPO defaults to neuronsimulator/nrn-ci-deps (do not infer nrn product repo).
if [[ -z "${REPO}" ]]; then
  echo "error: pass --repo owner/name or set NRN_CI_DEPS_PUBLISH_REPO" >&2
  exit 2
fi

if [[ ! -d "${ASSETS_DIR}" ]]; then
  echo "error: no assets dir at ${ASSETS_DIR}" >&2
  exit 2
fi

mapfile -t FILES < <(find "${ASSETS_DIR}" -maxdepth 1 -type f ! -name '.*' ! -name 'README*' | sort)
if [[ ${#FILES[@]} -eq 0 ]]; then
  echo "error: no files in ${ASSETS_DIR}" >&2
  exit 2
fi

echo "repo:  ${REPO}"
echo "tag:   ${TAG}"
echo "files:"
for f in "${FILES[@]}"; do
  ls -lh "$f" | awk '{print "  " $5, $9}'
done

if [[ "${DRY_RUN}" -eq 1 ]]; then
  echo "dry-run: not creating release"
  exit 0
fi

if ! command -v gh >/dev/null 2>&1; then
  echo "error: gh CLI required" >&2
  exit 2
fi

notes="Pinned CI binary/source dependencies for NEURON pipelines.
See ci/deps/MANIFEST.yml and ci/deps/README.md in the repository.

Fetch with:
  NRN_CI_DEPS_BASE_URL=https://github.com/${REPO}/releases/download/${TAG} \\
    ci/deps/fetch.sh <asset-id>
"

if gh release view "${TAG}" --repo "${REPO}" >/dev/null 2>&1; then
  echo "release ${TAG} exists; uploading/overwriting assets"
  gh release upload "${TAG}" "${FILES[@]}" --repo "${REPO}" --clobber
else
  echo "creating release ${TAG}"
  gh release create "${TAG}" "${FILES[@]}" \
    --repo "${REPO}" \
    --title "${TITLE}" \
    --notes "${notes}"
fi

echo "published. Suggested MANIFEST default_release_base_url:"
echo "  https://github.com/${REPO}/releases/download/${TAG}"
