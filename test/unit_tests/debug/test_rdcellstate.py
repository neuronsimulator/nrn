"""Unit tests for neuron.debug.rdcellstate (synthetic .nrndat fixtures).

Loads the module from the source tree so tests do not require a built NEURON
C extension — only pure Python parse/compare.
"""

from __future__ import annotations

import importlib.util
import io
import math
import sys
from contextlib import redirect_stderr, redirect_stdout
from pathlib import Path

import pytest

_ROOT = Path(__file__).resolve().parents[3]
_MODULE_PATH = _ROOT / "share/lib/python/neuron/debug/rdcellstate.py"
_FIX = Path(__file__).resolve().parent / "fixtures"


def _load_rd():
    # Register in sys.modules before exec so dataclasses (Py3.14+) resolve annotations.
    name = "rdcellstate_ut"
    spec = importlib.util.spec_from_file_location(name, _MODULE_PATH)
    assert spec is not None and spec.loader is not None
    mod = importlib.util.module_from_spec(spec)
    sys.modules[name] = mod
    spec.loader.exec_module(mod)
    return mod


rd = _load_rd()


def test_identical_exit_0():
    assert rd.main([str(_FIX / "ref_ab.nrndat"), str(_FIX / "identical.nrndat")]) == 0


def test_voltage_diff_exit_1():
    rc = rd.main([str(_FIX / "ref_ab.nrndat"), str(_FIX / "v_diff.nrndat")])
    assert rc == 1
    ref = rd.parse_nrndat(_FIX / "ref_ab.nrndat")
    other = rd.parse_nrndat(_FIX / "v_diff.nrndat")
    diffs = rd.compare_numeric_maps(
        rd.build_compare_maps(ref),
        rd.build_compare_maps(other),
        ignore_ion=False,
        ignore_matrix=False,
        ignore_names=set(),
    )
    v_diffs = [d for d in diffs if d.key[0] == "v"]
    assert any(d.key == ("v", 1) for d in v_diffs)
    assert abs(next(d for d in v_diffs if d.key == ("v", 1)).abs_diff - 4.5) < 1e-12


def test_topo_parent_area_in_compare_maps():
    ref = rd.parse_nrndat(_FIX / "ref_ab.nrndat")
    other = rd.parse_nrndat(_FIX / "topo_diff.nrndat")
    diffs = rd.compare_numeric_maps(
        rd.build_compare_maps(ref),
        rd.build_compare_maps(other),
        ignore_ion=False,
        ignore_matrix=False,
        ignore_names=set(),
    )
    assert any(d.key == ("topo", 1, "area") for d in diffs)
    assert rd.main([str(_FIX / "ref_ab.nrndat"), str(_FIX / "topo_diff.nrndat")]) == 1


def test_missing_mech_key():
    ref = rd.parse_nrndat(_FIX / "ref_ab.nrndat")
    other = rd.parse_nrndat(_FIX / "mech_missing.nrndat")
    diffs = rd.compare_numeric_maps(
        rd.build_compare_maps(ref),
        rd.build_compare_maps(other),
        ignore_ion=False,
        ignore_matrix=False,
        ignore_names=set(),
    )
    missing = [d for d in diffs if not math.isfinite(d.abs_diff)]
    assert any(d.key[0] == "mech" for d in missing)
    assert rd.main([str(_FIX / "ref_ab.nrndat"), str(_FIX / "mech_missing.nrndat")]) == 1


def test_ignore_matrix_hides_format_asymmetry():
    ref = rd.parse_nrndat(_FIX / "ref_ab.nrndat")
    other = rd.parse_nrndat(_FIX / "ref_abdrhs.nrndat")
    full = rd.compare_numeric_maps(
        rd.build_compare_maps(ref),
        rd.build_compare_maps(other),
        ignore_ion=False,
        ignore_matrix=False,
        ignore_names=set(),
    )
    matrix_missing = [
        d
        for d in full
        if d.key[0] == "matrix" and d.key[2] in ("d", "rhs") and not math.isfinite(d.abs_diff)
    ]
    assert matrix_missing
    ignored = rd.compare_numeric_maps(
        rd.build_compare_maps(ref),
        rd.build_compare_maps(other),
        ignore_ion=False,
        ignore_matrix=True,
        ignore_names=set(),
    )
    assert not any(d.key[0] == "matrix" for d in ignored)
    assert (
        rd.main(
            [
                str(_FIX / "ref_ab.nrndat"),
                str(_FIX / "ref_abdrhs.nrndat"),
                "--ignore-matrix",
            ]
        )
        == 0
    )


def test_parse_both_topo_headers():
    ab = rd.parse_nrndat(_FIX / "ref_ab.nrndat")
    abdrhs = rd.parse_nrndat(_FIX / "ref_abdrhs.nrndat")
    assert ab.meta.gid == 1 and ab.meta.t == 0.025
    assert (0, "a") in ab.matrix and (0, "d") not in ab.matrix
    assert (0, "d") in abdrhs.matrix and (0, "rhs") in abdrhs.matrix


def test_netcons_count_mismatch_stderr():
    err = io.StringIO()
    out = io.StringIO()
    with redirect_stdout(out), redirect_stderr(err):
        rc = rd.main(
            [str(_FIX / "ref_ab.nrndat"), str(_FIX / "netcons_count_diff.nrndat")]
        )
    # numerical maps match aside from nothing compared for netcons
    assert rc == 0
    assert "netcons count differs" in err.getvalue()
    assert "not compared" in err.getvalue()


def test_meta_parse():
    st = rd.parse_nrndat(_FIX / "ref_ab.nrndat")
    assert st.meta.gid == 1
    assert st.meta.t == pytest.approx(0.025)
    assert st.meta.netcons_count == 1
    assert st.meta.threshold_header == 0
