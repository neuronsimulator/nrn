#!/usr/bin/env bash
# Emit a semicolon-separated source file list for -DNRN_COVERAGE_FILES=...
# Usage: coverage_files_from_diff.sh [base-ref]
# Default base-ref is master (three-dot diff: base...HEAD).
# Exits 1 with a stderr message when no compiled sources (.cpp/.c) changed.
set -euo pipefail

base="${1:-master}"
mapfile -t files < <(git diff --name-only "${base}...HEAD" | grep -E '\.(cpp|c)$' || true)
if ((${#files[@]} == 0)); then
  echo "No compiled sources (.cpp/.c) changed vs ${base}; omit -DNRN_COVERAGE_FILES" >&2
  exit 1
fi
(IFS=';'; echo "${files[*]}")