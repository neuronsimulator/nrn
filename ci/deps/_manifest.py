#!/usr/bin/env python3
"""Minimal MANIFEST.yml helpers (stdlib only — no PyYAML required on CI).

Supports the restricted subset used by ci/deps/MANIFEST.yml:
  - top-level key: value
  - assets: list of maps with simple scalar values and folded (>) notes

The manifest path is fixed next to this file (not taken from CLI) so there is
no user/agent-controlled path into the filesystem.
"""
from __future__ import annotations

import json
import sys
from pathlib import Path
from typing import Any, Dict, List, Optional

# Only file this module will open. Not derived from argv (path-injection safe).
MANIFEST_PATH = Path(__file__).resolve().parent / "MANIFEST.yml"


def _parse_scalar(raw: str) -> str:
    s = raw.strip()
    # strip YAML inline comments (not inside URLs)
    if not s.startswith("http") and " #" in s:
        s = s.split(" #", 1)[0].rstrip()
    if s in ('""', "''", ""):
        return ""
    if len(s) >= 2 and s[0] == s[-1] and s[0] in "\"'":
        return s[1:-1]
    return s


def load_manifest() -> Dict[str, Any]:
    if not MANIFEST_PATH.is_file():
        raise SystemExit(f"error: manifest not found: {MANIFEST_PATH}")
    text = MANIFEST_PATH.read_text(encoding="utf-8")
    doc: Dict[str, Any] = {"assets": []}
    current: Optional[Dict[str, Any]] = None
    in_consumers = False
    in_folded = False
    folded_key: Optional[str] = None
    folded_lines: List[str] = []

    def end_folded() -> None:
        nonlocal in_folded, folded_key, folded_lines, current
        if in_folded and current is not None and folded_key:
            current[folded_key] = " ".join(x.strip() for x in folded_lines if x.strip())
        in_folded = False
        folded_key = None
        folded_lines = []

    for line in text.splitlines():
        if in_folded:
            # folded block lines are indented more than key
            if line.startswith("    ") or line.startswith("\t"):
                folded_lines.append(line)
                continue
            if line.strip() == "":
                folded_lines.append("")
                continue
            end_folded()
            # fall through to parse this line

        if not line.strip() or line.lstrip().startswith("#"):
            continue

        # top-level version / default_release_base_url
        if not line.startswith(" ") and not line.startswith("-") and ":" in line:
            key, _, val = line.partition(":")
            key = key.strip()
            val = _parse_scalar(val)
            if key == "assets":
                continue
            doc[key] = val
            current = None
            in_consumers = False
            continue

        # new asset
        if line.startswith("  - id:") or line.startswith("  - id :"):
            end_folded()
            if current:
                doc["assets"].append(current)
            current = {"id": _parse_scalar(line.split(":", 1)[1])}
            in_consumers = False
            continue

        if current is None:
            continue

        stripped = line.strip()
        if stripped.startswith("- ") and in_consumers:
            current.setdefault("consumers", []).append(
                stripped[2:].strip().strip("'\"")
            )
            continue

        if ":" not in line:
            continue

        # asset field (4-space indent typical)
        key, _, val = stripped.partition(":")
        key = key.strip()
        val = val.strip()
        in_consumers = False

        if key == "consumers":
            current["consumers"] = []
            in_consumers = True
            continue

        if val in (">", "|"):
            in_folded = True
            folded_key = key
            folded_lines = []
            continue

        current[key] = _parse_scalar(val)

    end_folded()
    if current:
        doc["assets"].append(current)
    return doc


def get_asset(doc: Dict[str, Any], asset_id: str) -> Dict[str, Any]:
    for a in doc.get("assets") or []:
        if a.get("id") == asset_id:
            out = dict(a)
            out["_default_release_base_url"] = doc.get("default_release_base_url") or ""
            return out
    raise SystemExit(f"error: unknown asset id: {asset_id}")


def main(argv: List[str]) -> None:
    if len(argv) < 1:
        print(
            "usage: _manifest.py list|ids|get <id>|json",
            file=sys.stderr,
        )
        raise SystemExit(2)
    cmd = argv[0]
    doc = load_manifest()
    if cmd == "list":
        for a in doc.get("assets") or []:
            managed = a.get("managed", "?")
            print(f"{a.get('id', '?'):40} managed={managed!s:5} {a.get('file', '')}")
    elif cmd == "ids":
        for a in doc.get("assets") or []:
            if a.get("id"):
                print(a["id"])
    elif cmd == "get" and len(argv) >= 2:
        json.dump(get_asset(doc, argv[1]), sys.stdout)
    elif cmd == "json":
        json.dump(doc, sys.stdout, indent=2)
        print()
    else:
        raise SystemExit(f"unknown command: {cmd}")


if __name__ == "__main__":
    main(sys.argv[1:])
