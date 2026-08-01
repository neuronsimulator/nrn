#!/usr/bin/env python3
"""Probe an installed NEURON (wheel/venv) for foreign-ctest configure.

Prints a single JSON object on stdout. Invoked by CMake DiscoverNeuron.cmake
using NRN_FOREIGN_PYTHON (must be the interpreter that can import neuron).
"""
from __future__ import annotations

import json
import os
import shutil
import sys


def _which(name: str) -> str | None:
    return shutil.which(name)


def main() -> int:
    try:
        import neuron
    except Exception as exc:  # noqa: BLE001 - report any import failure to CMake
        print(
            json.dumps(
                {
                    "ok": False,
                    "error": f"import neuron failed: {exc}",
                    "python": sys.executable,
                }
            ),
            file=sys.stderr,
        )
        return 1

    version = getattr(neuron, "__version__", None)
    git_sha = None
    version_full = version
    nrnversions: dict[str, str] = {}
    try:
        from neuron import h

        for i in range(8):
            try:
                nrnversions[str(i)] = str(h.nrnversion(i))
            except Exception:  # noqa: BLE001
                pass
        # nrnversion(3) is typically the short git SHA from the build
        git_sha = nrnversions.get("3") or None
        version_full = nrnversions.get("5") or version
    except Exception as exc:  # noqa: BLE001
        nrnversions["error"] = str(exc)

    # Fall back: extract g<sha> from describe-style version strings (nightlies).
    if not git_sha:
        import re

        for candidate in (version_full, version):
            if not candidate:
                continue
            m = re.search(r"g([0-9a-fA-F]+)", str(candidate))
            if m:
                git_sha = m.group(1)
                break

    features: dict[str, object] = {}
    try:
        from neuron import config

        args = getattr(config, "arguments", None) or {}
        for key, val in args.items():
            # JSON-friendly
            if isinstance(val, (bool, int, float, str)) or val is None:
                features[key] = val
            elif isinstance(val, (list, tuple)):
                features[key] = list(val)
            else:
                features[key] = str(val)
    except Exception as exc:  # noqa: BLE001
        features["_error"] = str(exc)

    tools = {
        "nrniv": _which("nrniv"),
        "nrnivmodl": _which("nrnivmodl"),
        "modlunit": _which("modlunit"),
        "nocmodl": _which("nocmodl"),
        "mpiexec": _which("mpiexec"),
    }

    neuron_file = getattr(neuron, "__file__", None)
    payload = {
        "ok": True,
        "python": sys.executable,
        "neuron_file": neuron_file,
        "version": version,
        "version_full": version_full,
        "git_sha": git_sha,
        "nrnversion": nrnversions,
        "features": features,
        "tools": tools,
        "env": {
            "PATH": os.environ.get("PATH", ""),
            "VIRTUAL_ENV": os.environ.get("VIRTUAL_ENV"),
            "NEURONHOME": os.environ.get("NEURONHOME"),
            "NRNHOME": os.environ.get("NRNHOME"),
        },
    }
    print(json.dumps(payload, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    sys.exit(main())
