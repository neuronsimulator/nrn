#!/usr/bin/env python3
"""Audit an abi3 wheel, ignoring CLI libpywrapper (pybind11 embed, not importable)."""
from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

# NMODL CLI sympy embed; not an importable extension. cibuildwheel's default
# `abi3audit --strict` scans every .so in the wheel.
_IGNORE_NAMES = {"libpywrapper.so", "libpywrapper.dylib", "libpywrapper.dll"}


def _wheel_objects(report: dict) -> list[dict]:
    objects = []
    for spec in report.get("specs", {}).values():
        objects.extend(spec.get("wheel") or spec.get("objects") or [])
    return objects


def _violations(report: dict) -> list[tuple[str, dict]]:
    failed = []
    for obj in _wheel_objects(report):
        name = os.path.basename(obj.get("name", ""))
        result = obj.get("result") or {}
        if name in _IGNORE_NAMES:
            if not result.get("is_abi3", True):
                print(f"note: ignoring non-abi3 CLI {name}", file=sys.stderr)
            continue
        if result.get("is_abi3") is False:
            failed.append((name, result))
    return failed


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print("usage: audit_abi3_wheel.py WHEEL", file=sys.stderr)
        return 2
    wheel = Path(argv[1]).resolve()
    if not wheel.is_file() or wheel.suffix != ".whl":
        print(f"not a wheel file: {wheel}", file=sys.stderr)
        return 2
    abi3audit = shutil.which("abi3audit")
    if not abi3audit:
        print("abi3audit not found on PATH", file=sys.stderr)
        return 1
    proc = subprocess.run(
        [abi3audit, "--report", os.fspath(wheel)],
        check=False,
        capture_output=True,
        text=True,
    )
    raw = proc.stdout.strip() or proc.stderr.strip()
    if not raw:
        print(
            proc.stderr or proc.stdout or "abi3audit produced no output",
            file=sys.stderr,
        )
        return proc.returncode or 1
    try:
        report = json.loads(raw)
    except json.JSONDecodeError:
        sys.stdout.write(proc.stdout)
        sys.stderr.write(proc.stderr)
        return proc.returncode or 1
    failed = _violations(report)
    if not failed:
        return 0
    for name, result in failed:
        print(f"abi3 violation: {name}: {json.dumps(result)}", file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
