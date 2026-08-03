#!/usr/bin/env bash
# Publish managed assets under ci/deps/assets/ to a GitHub Release.
#
# Usage:
#   ci/deps/publish.sh [--repo owner/name] [--tag ci-deps-v1] [--dry-run]
#
# Requires: gh (authenticated), curl.
# After publishing, set default_release_base_url in MANIFEST.yml or export:
#   NRN_CI_DEPS_BASE_URL=https://github.com/<owner>/<name>/releases/download/<tag>
#
# Local managed assets remain the default for fetch.sh (NRN_CI_DEPS_SOURCE=local).
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ASSETS_DIR="${SCRIPT_DIR}/assets"
REPO="${NRN_CI_DEPS_PUBLISH_REPO:-}"
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

if [[ -z "${REPO}" ]]; then
  # Infer from git remote if possible
  if command -v gh >/dev/null 2>&1; then
    REPO="$(gh repo view --json nameWithOwner -q .nameWithOwner 2>/dev/null || true)"
  fi
fi
if [[ -z "${REPO}" ]]; then
  echo "error: pass --repo owner/name (or run inside a gh-connected checkout)" >&2
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
