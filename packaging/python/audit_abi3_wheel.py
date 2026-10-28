#!/usr/bin/env python3
"""Fail closed on an abi3 wheel audit.

The report is accepted only when the expected extensions were inspected and
each one is abi3 for the wheel baseline. An empty or unexpected report fails.
A nonzero abi3audit status fails unless the only failing object is the ignored
libpywrapper CLI library.
"""
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

# Importable extensions on every abi3 wheel. RxD is on for wheels.
# "hoc." does not match a longer stem.
_REQUIRED_STEMS = ("hoc", "graphicsPrimitives", "ctng")


def _requires_libnrnpython(wheel_name: str) -> bool:
    """abi3audit scans .so and .pyd only.

    manylinux and musllinux wheels contain libnrnpython.abi3.so. macOS builds
    that library as a .dylib and Windows as a .dll, so those reports have no
    such object.
    """
    return "manylinux" in wheel_name or "musllinux" in wheel_name


def _matches_stem(name: str, stem: str) -> bool:
    return name.startswith(stem + ".")


def _is_libnrnpython(name: str) -> bool:
    return name.startswith("libnrnpython") or name.startswith("nrnpython")


def _result_problems(name: str, result: object) -> list[str]:
    if not isinstance(result, dict):
        return [f"abi3 audit: {name}: result is not an object"]
    problems = []
    if result.get("is_abi3") is not True:
        problems.append(f"abi3 audit: {name}: is_abi3 is not true")
    if result.get("is_abi3_baseline_compatible") is not True:
        problems.append(f"abi3 audit: {name}: is_abi3_baseline_compatible is not true")
    return problems


def _object_list(spec_name: object, spec: dict) -> tuple[list, str | None]:
    if "wheel" in spec:
        listed = spec["wheel"]
        label = "wheel"
    elif "objects" in spec:
        listed = spec["objects"]
        label = "objects"
    else:
        return [], f"abi3 audit: spec {spec_name!r} has no wheel object list"
    if not isinstance(listed, list):
        return [], f"abi3 audit: spec {spec_name!r} {label} list is not a list"
    if not listed:
        return [], f"abi3 audit: spec {spec_name!r} {label} list is empty"
    return listed, None


def judge(report: object, tool_status: int, wheel_name: str) -> tuple[int, list[str]]:
    """Return an exit code and the lines to print.

    ``tool_status`` is abi3audit's exit code. Nonzero is accepted only when an
    ignored libpywrapper object is the failure that explains it.
    """
    if not isinstance(report, dict):
        return 1, ["abi3 audit: report is not an object"]
    specs = report.get("specs")
    if not isinstance(specs, dict) or not specs:
        return 1, ["abi3 audit: specs is missing or empty"]

    messages: list[str] = []
    problems: list[str] = []
    inspected: list[str] = []
    ignored_failure = False
    for spec_name, spec in specs.items():
        if not isinstance(spec, dict):
            return 1, [f"abi3 audit: spec {spec_name!r} is not an object"]
        listed, shape_error = _object_list(spec_name, spec)
        if shape_error:
            return 1, [shape_error]
        for obj in listed:
            if not isinstance(obj, dict):
                return 1, ["abi3 audit: wheel object is not an object"]
            name = os.path.basename(str(obj.get("name", "")))
            if not name:
                problems.append("abi3 audit: wheel object has no name")
                continue
            result = obj.get("result")
            if name in _IGNORE_NAMES:
                if _result_problems(name, result):
                    ignored_failure = True
                    messages.append(f"note: ignoring non-abi3 CLI {name}")
                continue
            inspected.append(name)
            problems.extend(_result_problems(name, result))

    if inspected:
        shown = ", ".join(sorted(inspected))
        messages.append(f"checked {len(inspected)} abi3 objects: {shown}")
    else:
        messages.append("checked 0 abi3 objects")

    for stem in _REQUIRED_STEMS:
        if not any(_matches_stem(name, stem) for name in inspected):
            problems.append(f"abi3 audit: missing {stem}")
    if _requires_libnrnpython(wheel_name):
        if not any(_is_libnrnpython(name) for name in inspected):
            problems.append("abi3 audit: missing libnrnpython")
    if tool_status != 0 and not ignored_failure:
        problems.append(
            "abi3 audit: abi3audit exited "
            f"{tool_status} and no ignored libpywrapper failure explains it"
        )
    if problems:
        messages.extend(problems)
        return 1, messages
    return 0, messages


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
    code, messages = judge(report, proc.returncode, wheel.name)
    for line in messages:
        print(line, file=sys.stderr)
    return code


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
