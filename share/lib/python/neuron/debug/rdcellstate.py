#!/usr/bin/env python3
"""
Compare ParallelContext.prcellstate dumps (.nrndat) by semantic keys, not raw lines.

Replaces the HOC rdcellstate() line-by-line diff in prcellstate.hoc, which misaligns
when dump sizes or section order differ (e.g. NEURON vs CoreNEURON, native GPU _ion).

NetCon payload lines are parsed for counting only; structured NetCon field compare
is not implemented yet. If both dumps report netcons N headers and N differs, a
warning is printed to stderr.

Master dumps use topology header ``inode parent area a b`` (matrix a,b only).
Some feature-line dumps add ``d rhs``. Comparing mixed formats reports missing
matrix d/rhs keys unless ``--ignore-matrix`` is used.

Usage:
  python -m neuron.debug.rdcellstate ref.nrndat other.nrndat
  python -m neuron.debug.rdcellstate ref.nrndat other.nrndat --top 25 --ignore-ion
  python -m neuron.debug.rdcellstate ref.nrndat other.nrndat --ignore-unused
  python -m neuron.debug.rdcellstate ref.nrndat other.nrndat --ignore-matrix
  rdcellstate ref.nrndat other.nrndat   # if bin/rdcellstate is on PATH

CLI file arguments are resolved with realpath and must stay under the process
current working directory (agent path-injection mitigation). Run from a parent
of the dumps, or pass paths relative to cwd.
"""

from __future__ import annotations

import argparse
import math
import os
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Set, Tuple, Union

Key = Tuple[str, ...]  # category-specific tuple keys


def cli_safe_path(path: Union[str, Path]) -> str:
    """Canonicalize a CLI path and require it stay under process cwd (Sonar S8707).

    LLMs/agents may pass ``../``-style arguments into this CLI. Resolve with
    ``realpath`` and reject anything outside the directory from which the tool
    was invoked. Returns the canonical path string for use with ``open()``.
    """
    resolved = os.path.realpath(os.fspath(path))
    base_dir = os.path.realpath(os.getcwd())
    if resolved != base_dir and not resolved.startswith(base_dir + os.sep):
        raise SystemExit(
            f"error: path {os.fspath(path)!r} resolves outside the current "
            f"working directory ({base_dir}); run from a parent of the files "
            "or pass a path under cwd"
        )
    return resolved


def cli_safe_file(path: Union[str, Path]) -> str:
    """Like :func:`cli_safe_path` but require an existing regular file."""
    resolved = cli_safe_path(path)
    if not os.path.isfile(resolved):
        raise SystemExit(f"error: not a file: {os.fspath(path)!r}")
    return resolved


def cli_safe_dir(path: Union[str, Path]) -> str:
    """Like :func:`cli_safe_path` but require an existing directory."""
    resolved = cli_safe_path(path)
    if not os.path.isdir(resolved):
        raise SystemExit(f"error: not a directory: {os.fspath(path)!r}")
    return resolved


@dataclass
class PrcellMeta:
    path: Path
    gid: Optional[int] = None
    t: Optional[float] = None
    celsius: Optional[float] = None
    n_nodes: Optional[int] = None
    threshold_header: Optional[int] = None  # value printed in "X is the threshold node"
    threshold_mV: Optional[float] = None
    netcons_count: Optional[int] = None  # from "netcons N" header if present


@dataclass
class PrcellState:
    meta: PrcellMeta
    voltages: Dict[int, float] = field(default_factory=dict)
    # inode -> (parent, area, a, b)  — a,b also stored in matrix
    topology: Dict[int, Tuple[int, float, float, float]] = field(default_factory=dict)
    # (inode, field_name) -> value for Hines matrix coeffs a,b,d,rhs
    matrix: Dict[Tuple[int, str], float] = field(default_factory=dict)
    mechanisms: Dict[Key, float] = field(default_factory=dict)
    # keys: ("mech", type_id, mech_name, inode, field_index)
    netcons: List[str] = field(default_factory=list)


_RE_HEADER = re.compile(r"^gid\s*=\s*(\d+)")
_RE_T = re.compile(r"^t\s*=\s*([+-]?\d+(?:\.\d*)?(?:[eE][+-]?\d+)?)")
_RE_CELSIUS = re.compile(r"^celsius\s*=\s*([+-]?\d+(?:\.\d*)?(?:[eE][+-]?\d+)?)")
_RE_NODES = re.compile(r"^(\d+)\s+nodes\s+(\d+)\s+is the threshold node")
_RE_THRESHOLD = re.compile(r"^threshold\s+([+-]?\d+(?:\.\d*)?(?:[eE][+-]?\d+)?)")
_RE_MECH = re.compile(r"^type=(\d+)\s+(\S+)\s+size=(\d+)")
_RE_NETCONS_HDR = re.compile(r"^netcons\s+(\d+)\s*$")
_RE_TOPO = re.compile(
    r"^(\d+)\s+(-?\d+)\s+([+-]?\d+(?:\.\d*)?(?:[eE][+-]?\d+)?)\s+"
    r"([+-]?\d+(?:\.\d*)?(?:[eE][+-]?\d+)?)\s+([+-]?\d+(?:\.\d*)?(?:[eE][+-]?\d+)?)"
)
_RE_TOPO_MATRIX = re.compile(
    r"^(\d+)\s+(-?\d+)\s+([+-]?\d+(?:\.\d*)?(?:[eE][+-]?\d+)?)\s+"
    r"([+-]?\d+(?:\.\d*)?(?:[eE][+-]?\d+)?)\s+([+-]?\d+(?:\.\d*)?(?:[eE][+-]?\d+)?)\s+"
    r"([+-]?\d+(?:\.\d*)?(?:[eE][+-]?\d+)?)\s+([+-]?\d+(?:\.\d*)?(?:[eE][+-]?\d+)?)"
)
_RE_VOLTAGE = re.compile(r"^(\d+)\s+([+-]?\d+(?:\.\d*)?(?:[eE][+-]?\d+)?)")
_RE_MECH_VAL = re.compile(
    r"^\s*(\d+)\s+(\d+)\s+([+-]?\d+(?:\.\d*)?(?:[eE][+-]?\d+)?)\s*$"
)
_RE_MECH_NRI = re.compile(r"^\s*(\d+)\s+nri\s+(\d+)\s*$")
_RE_MECH_FIELD = re.compile(
    r'_nrn_mechanism_field<double>\{"([^"]+)"\}\s*/\*\s*(\d+)\s*\*/'
)
_RE_INSTANCE_STRUCT = re.compile(r"struct\s+(\w+)_Instance\s*\{")
_RE_INSTANCE_MEMBER = re.compile(r"(?:const\s+)?double\s*\*\s*(\w+)\s*\{\}")

UnusedFieldMap = Dict[str, Set[int]]


def _parse_float(s: str) -> float:
    return float(s)


def parse_nrndat_text(path: Path, text: str) -> PrcellState:
    """Parse an already-loaded ``.nrndat`` body (no filesystem access)."""
    meta = PrcellMeta(path=path)
    state = PrcellState(meta=meta)

    section: Optional[str] = None
    mech_type: Optional[int] = None
    mech_name: Optional[str] = None

    for raw in text.splitlines():
        line = raw.rstrip("\n")

        m = _RE_HEADER.match(line)
        if m:
            meta.gid = int(m.group(1))
            continue
        m = _RE_T.match(line)
        if m:
            meta.t = _parse_float(m.group(1))
            continue
        m = _RE_CELSIUS.match(line)
        if m:
            meta.celsius = _parse_float(m.group(1))
            continue
        m = _RE_NODES.match(line)
        if m:
            meta.n_nodes = int(m.group(1))
            meta.threshold_header = int(m.group(2))
            continue
        m = _RE_THRESHOLD.match(line)
        if m:
            meta.threshold_mV = _parse_float(m.group(1))
            continue

        if line in ("inode parent area a b", "inode parent area a b d rhs"):
            section = "topo"
            continue
        if line == "inode v":
            section = "v"
            continue
        m = _RE_NETCONS_HDR.match(line)
        if m:
            section = "netcon"
            meta.netcons_count = int(m.group(1))
            state.netcons.append(line)
            continue
        if line.startswith("netcons "):
            section = "netcon"
            state.netcons.append(line)
            continue
        if section == "netcon":
            state.netcons.append(line)
            continue

        m = _RE_MECH.match(line)
        if m:
            mech_type = int(m.group(1))
            mech_name = m.group(2)
            section = "mech"
            continue

        if section == "topo":
            m = _RE_TOPO_MATRIX.match(line)
            if m:
                inode = int(m.group(1))
                parent = int(m.group(2))
                area = _parse_float(m.group(3))
                a = _parse_float(m.group(4))
                b = _parse_float(m.group(5))
                d = _parse_float(m.group(6))
                rhs = _parse_float(m.group(7))
                state.topology[inode] = (parent, area, a, b)
                state.matrix[(inode, "a")] = a
                state.matrix[(inode, "b")] = b
                state.matrix[(inode, "d")] = d
                state.matrix[(inode, "rhs")] = rhs
                continue
            m = _RE_TOPO.match(line)
            if m:
                inode = int(m.group(1))
                parent = int(m.group(2))
                area = _parse_float(m.group(3))
                a = _parse_float(m.group(4))
                b = _parse_float(m.group(5))
                state.topology[inode] = (parent, area, a, b)
                state.matrix[(inode, "a")] = a
                state.matrix[(inode, "b")] = b
            continue

        if section == "v":
            m = _RE_VOLTAGE.match(line)
            if m:
                state.voltages[int(m.group(1))] = _parse_float(m.group(2))
            continue

        if section == "mech" and mech_type is not None and mech_name is not None:
            m = _RE_MECH_NRI.match(line)
            if m:
                inode = int(m.group(1))
                nri = int(m.group(2))
                key = ("mech_nri", mech_type, mech_name, inode)
                state.mechanisms[key] = float(nri)
                continue
            m = _RE_MECH_VAL.match(line)
            if m:
                inode = int(m.group(1))
                fld = int(m.group(2))
                val = _parse_float(m.group(3))
                key = ("mech", mech_type, mech_name, inode, fld)
                state.mechanisms[key] = val
            continue

    return state


def parse_nrndat(path: Path) -> PrcellState:
    """Parse a ``.nrndat`` file from the filesystem (library / tests API).

    The CLI entry point does not call this with raw argv paths; it validates
    with :func:`cli_safe_file` and opens the canonical path before parsing so
    agent-controlled path injection cannot escape the process cwd (S8707).
    """
    return parse_nrndat_text(path, path.read_text(encoding="utf-8", errors="replace"))


def threshold_voltage_inode(
    meta: PrcellMeta, *, legacy_header: bool = False
) -> Optional[int]:
    """Inode label in 'inode v' for the presyn threshold.

    Header is the same cell-local inode as voltage lines. Legacy dumps printed
    local_inode-1; pass legacy_header=True for those.
    """
    if meta.threshold_header is None:
        return None
    if legacy_header:
        return meta.threshold_header + 1
    return meta.threshold_header


def rel_diff(a: float, b: float) -> float:
    denom = abs(a) + abs(b)
    if denom == 0.0:
        return 0.0 if a == b else math.inf
    return abs(a - b) / denom


def parse_unused_fields_from_cpp(path: Path) -> Optional[Tuple[str, Set[int]]]:
    """Return (mech_name, unused_field_indices) from NEURON or CoreNEURON mod C++."""
    # Callers pass paths under a CLI-validated mod dir (or library-controlled dirs).
    text = path.read_text(encoding="utf-8", errors="replace")

    mech_name: Optional[str] = None
    m = _RE_INSTANCE_STRUCT.search(text)
    if m:
        mech_name = m.group(1)

    fields: Dict[int, str] = {}
    for name, idx_s in _RE_MECH_FIELD.findall(text):
        fields[int(idx_s)] = name

    if fields:
        unused = {idx for idx, name in fields.items() if name.endswith("_unused")}
        if unused and mech_name:
            return mech_name, unused
        if unused:
            return path.stem, unused

    if not mech_name:
        return None

    unused_struct: Set[int] = set()
    struct_m = _RE_INSTANCE_STRUCT.search(text)
    if not struct_m:
        return None
    brace = text.find("{", struct_m.end() - 1)
    if brace < 0:
        return None
    depth = 0
    idx = 0
    for line in text[brace + 1 :].splitlines():
        if "{" in line:
            depth += line.count("{")
        if "}" in line:
            depth -= line.count("}")
            if depth < 0:
                break
        member = _RE_INSTANCE_MEMBER.search(line)
        if member:
            if member.group(1).endswith("_unused"):
                unused_struct.add(idx)
            idx += 1

    if unused_struct:
        return mech_name, unused_struct
    return None


def discover_mod_cpp_dirs(*paths: Path) -> List[Path]:
    """Candidate directories of translated mod C++ (NEURON x86_64, CoreNEURON mod2c)."""
    seen: Set[Path] = set()
    out: List[Path] = []
    for base in paths:
        if base is None:
            continue
        for candidate in (
            base,
            base / "x86_64",
            base / "x86_64" / "corenrn" / "mod2c",
        ):
            resolved = candidate.resolve()
            if resolved in seen or not resolved.is_dir():
                continue
            if any(resolved.glob("*.cpp")):
                seen.add(resolved)
                out.append(resolved)
    return out


def load_unused_field_map(mod_dirs: Iterable[Path]) -> UnusedFieldMap:
    """Map mechanism name -> SOA field indices named *_unused in translated mod C++."""
    out: UnusedFieldMap = {}
    for mod_dir in mod_dirs:
        for cpp in sorted(mod_dir.glob("*.cpp")):
            parsed = parse_unused_fields_from_cpp(cpp)
            if not parsed:
                continue
            mech_name, unused = parsed
            out.setdefault(mech_name, set()).update(unused)
    return out


def should_ignore_key(
    key: Key,
    ignore_ion: bool,
    ignore_matrix: bool,
    ignore_names: Set[str],
    unused_fields: Optional[UnusedFieldMap] = None,
) -> bool:
    if not key:
        return True
    cat = key[0]
    if ignore_matrix and cat == "matrix":
        return True
    if cat == "mech" and len(key) >= 5:
        _, _type_id, name, _inode, fld = key[:5]
        if name in ignore_names:
            return True
        if unused_fields and fld in unused_fields.get(name, ()):
            return True
        if ignore_ion and name.endswith("_ion"):
            # Ion SOA: 0=erev, 1=conci, 2=conco, 3=cur, 4=dcurdv (eion.cpp).
            # Native GPU often leaves these unset; erev (0) can be a 1e6 sentinel.
            return True
        if ignore_ion and name == "capacitance" and fld == 1:  # i_cap
            return True
    return False


@dataclass
class Diff:
    key: Key
    a: float
    b: float
    abs_diff: float
    rel: float
    label: str


def format_key(key: Key) -> str:
    cat = key[0]
    if cat == "v":
        return f"v[inode={key[1]}]"
    if cat == "topo":
        return f"topo inode={key[1]} {key[2]}"
    if cat == "matrix":
        return f"matrix inode={key[1]} {key[2]}"
    if cat == "mech":
        _, type_id, name, inode, fld = key
        return f"mech type={type_id} {name} inode={inode} field={fld}"
    if cat == "mech_nri":
        _, type_id, name, inode = key
        return f"mech_nri type={type_id} {name} inode={inode}"
    return str(key)


def compare_numeric_maps(
    a_map: Dict[Key, float],
    b_map: Dict[Key, float],
    ignore_ion: bool,
    ignore_matrix: bool,
    ignore_names: Set[str],
    unused_fields: Optional[UnusedFieldMap] = None,
) -> List[Diff]:
    diffs: List[Diff] = []
    all_keys = set(a_map) | set(b_map)
    for key in sorted(all_keys):
        if should_ignore_key(
            key, ignore_ion, ignore_matrix, ignore_names, unused_fields
        ):
            continue
        in_a = key in a_map
        in_b = key in b_map
        if not in_a or not in_b:
            missing = "A" if not in_a else "B"
            diffs.append(
                Diff(
                    key=key,
                    a=a_map.get(key, float("nan")),
                    b=b_map.get(key, float("nan")),
                    abs_diff=float("inf"),
                    rel=float("inf"),
                    label=f"missing in {missing}",
                )
            )
            continue
        va, vb = a_map[key], b_map[key]
        if va == vb:
            continue
        diffs.append(
            Diff(
                key=key,
                a=va,
                b=vb,
                abs_diff=abs(va - vb),
                rel=rel_diff(va, vb),
                label=format_key(key),
            )
        )
    return diffs


def build_compare_maps(state: PrcellState) -> Dict[Key, float]:
    out: Dict[Key, float] = {}
    for inode, v in state.voltages.items():
        out[("v", inode)] = v
    for inode, (parent, area, _a, _b) in state.topology.items():
        out[("topo", inode, "parent")] = float(parent)
        out[("topo", inode, "area")] = area
    for (inode, name), val in state.matrix.items():
        out[("matrix", inode, name)] = val
    out.update(state.mechanisms)
    return out


def warn_netcons_if_count_differs(ref: PrcellState, other: PrcellState) -> None:
    """Stderr note when netcons counts differ. Payloads are not compared."""
    ra, rb = ref.meta.netcons_count, other.meta.netcons_count
    if ra is None and rb is None:
        return
    if ra != rb:
        print(
            f"warning: netcons count differs (ref={ra} other={rb}); "
            "NetCon payload fields are not compared by this tool",
            file=sys.stderr,
        )


def summarize(diffs: List[Diff]) -> Dict[str, Dict[str, float]]:
    summary: Dict[str, Dict[str, float]] = {}
    for d in diffs:
        cat = d.key[0]
        bucket = summary.setdefault(cat, {"count": 0, "max_abs": 0.0})
        bucket["count"] += 1
        if math.isfinite(d.abs_diff):
            bucket["max_abs"] = max(bucket["max_abs"], d.abs_diff)
    return summary


def print_report(
    ref: PrcellState,
    other: PrcellState,
    diffs: List[Diff],
    top: int,
    *,
    legacy_threshold_header: bool = False,
) -> None:
    rm, om = ref.meta, other.meta
    print(f"ref:   {rm.path.name}  gid={rm.gid} t={rm.t}")
    print(f"other: {om.path.name}  gid={om.gid} t={om.t}")

    th_inode = threshold_voltage_inode(rm, legacy_header=legacy_threshold_header)
    if th_inode is not None:
        va = ref.voltages.get(th_inode)
        vb = other.voltages.get(th_inode)
        hdr_note = (
            f" (legacy header {rm.threshold_header} → inode {th_inode})"
            if legacy_threshold_header
            else ""
        )
        print(
            f"threshold inode {th_inode}{hdr_note}: "
            f"ref={va} other={vb}  dV={None if va is None or vb is None else va - vb}"
        )

    print()
    summary = summarize(diffs)
    if not summary:
        print("No differences (after filters).")
        return
    print("Summary by category:")
    for cat, bucket in sorted(summary.items()):
        print(f"  {cat}: {int(bucket['count'])} diffs, max |d|={bucket['max_abs']:.6g}")

    finite = [d for d in diffs if math.isfinite(d.abs_diff)]
    finite.sort(key=lambda d: d.abs_diff, reverse=True)

    print(f"\nTop {min(top, len(finite))} by |absolute difference|:")
    for d in finite[:top]:
        print(
            f"  |d|={d.abs_diff:.6g}  rel={d.rel:.6g}  "
            f"A={d.a:.9g}  B={d.b:.9g}  {d.label}"
        )

    missing = [d for d in diffs if not math.isfinite(d.abs_diff)]
    if missing:
        print(f"\nKeys present in only one file: {len(missing)}")
        for d in missing[: min(10, len(missing))]:
            print(f"  {d.label}  {format_key(d.key)}")


def main(argv: Optional[Iterable[str]] = None) -> int:
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("ref", type=Path, help="reference .nrndat (e.g. NEURON CPU)")
    parser.add_argument("other", type=Path, help="comparison .nrndat")
    parser.add_argument(
        "--top",
        type=int,
        default=20,
        help="number of largest diffs to print (default: 20)",
    )
    parser.add_argument(
        "--ignore-ion",
        "--ignore_ion",
        action="store_true",
        help=(
            "skip all _ion mechanism fields (erev, conci, conco, cur, dcurdv) "
            "and capacitance i_cap(1)"
        ),
    )
    parser.add_argument(
        "--ignore-matrix",
        "--ignore_matrix",
        action="store_true",
        help=(
            "skip Hines matrix coefficients a, b, d, rhs per inode "
            "(useful when one dump has a b only and the other has a b d rhs)"
        ),
    )
    parser.add_argument(
        "--ignore-mech",
        action="append",
        default=[],
        metavar="NAME",
        help="skip mechanism by name (repeatable)",
    )
    parser.add_argument(
        "--ignore-unused",
        "--ignore_unused",
        action="store_true",
        help=(
            "skip mechanism SOA fields named *_unused (v_unused, g_unused, ...) "
            "discovered from translated mod C++"
        ),
    )
    parser.add_argument(
        "--mod-dir",
        action="append",
        default=[],
        type=Path,
        metavar="DIR",
        help=(
            "directory of translated mod C++ (e.g. x86_64). Repeatable. "
            "Default: x86_64 and x86_64/corenrn/mod2c next to the .nrndat files"
        ),
    )
    parser.add_argument(
        "--legacy-threshold-header",
        action="store_true",
        help=(
            "header printed local_inode-1 (pre NEURON/CN hygiene); "
            "map threshold voltage via header+1"
        ),
    )
    args = parser.parse_args(list(argv) if argv is not None else None)

    # Validate CLI paths under cwd, then open via canonical strings (Sonar S8707).
    ref_resolved = cli_safe_file(args.ref)
    other_resolved = cli_safe_file(args.other)
    with open(ref_resolved, encoding="utf-8", errors="replace") as f:
        ref_text = f.read()
    with open(other_resolved, encoding="utf-8", errors="replace") as f:
        other_text = f.read()
    ref = parse_nrndat_text(Path(ref_resolved), ref_text)
    other = parse_nrndat_text(Path(other_resolved), other_text)
    ignore_names = set(args.ignore_mech)

    warn_netcons_if_count_differs(ref, other)

    unused_fields: Optional[UnusedFieldMap] = None
    if args.ignore_unused:
        if args.mod_dir:
            mod_dirs = [Path(cli_safe_dir(d)) for d in args.mod_dir]
        else:
            mod_dirs = discover_mod_cpp_dirs(
                Path(ref_resolved).parent, Path(other_resolved).parent
            )
            # Default discovery stays under already-validated dump parents.
            mod_dirs = [Path(cli_safe_dir(d)) for d in mod_dirs]
        unused_fields = load_unused_field_map(mod_dirs)
        if unused_fields:
            parts = [
                f"{name} fields {sorted(idxs)}"
                for name, idxs in sorted(unused_fields.items())
            ]
            print(
                f"ignore-unused: {len(unused_fields)} mechs from "
                f"{len(mod_dirs)} mod dir(s); e.g. {', '.join(parts[:5])}"
                + (" ..." if len(parts) > 5 else ""),
                file=sys.stderr,
            )
        else:
            print(
                "warning: --ignore-unused but no *_unused fields found in mod C++ "
                f"(searched {mod_dirs})",
                file=sys.stderr,
            )

    diffs = compare_numeric_maps(
        build_compare_maps(ref),
        build_compare_maps(other),
        ignore_ion=args.ignore_ion,
        ignore_matrix=args.ignore_matrix,
        ignore_names=ignore_names,
        unused_fields=unused_fields,
    )
    print_report(
        ref,
        other,
        diffs,
        args.top,
        legacy_threshold_header=args.legacy_threshold_header,
    )
    return 0 if not diffs else 1


if __name__ == "__main__":
    sys.exit(main())
