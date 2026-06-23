#!/usr/bin/env bash
# Emit a semicolon-separated source file list for -DNRN_COVERAGE_FILES=...
# Usage: coverage_files_from_diff.sh [base-ref]
# Default base-ref is master (three-dot diff: base...HEAD).
set -euo pipefail

base="${1:-master}"
git diff --name-only "${base}...HEAD" \
  | grep -E '\.(cpp|hpp|c|h)$' \
  | paste -sd';' -