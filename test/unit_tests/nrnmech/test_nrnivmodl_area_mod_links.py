"""nrnivmodl of a mechanism must link against nrniv.dll data.

AREA mods (nocmodl areadec) reference nrn_alloc_node_. That is C++ data;
WINDOWS_EXPORT_ALL_SYMBOLS does not export it. ModelDB 64195 was the
finder. Unix is the control.
"""

from __future__ import print_function

import os
import shutil
import subprocess
import sys

import pytest

from neuron import config, h

WIN = sys.platform == "win32"


def _asan_or_tsan():
    val = str(config.arguments.get("NRN_SANITIZERS") or "")
    parts = [p.strip().lower() for p in val.replace(";", ",").split(",") if p.strip()]
    return "address" in parts or "thread" in parts


def _exe_names(name):
    names = [name]
    if WIN and not name.lower().endswith(".exe"):
        names.append(name + ".exe")
    return names


def _tool_dirs():
    dirs = []
    for nrn in _exe_names("nrniv"):
        found = shutil.which(nrn)
        if found:
            dirs.append(os.path.dirname(os.path.abspath(found)))
            break
    nh = str(h.neuronhome())
    dirs.append(os.path.join(nh, "bin"))
    dirs.append(os.path.abspath(os.path.join(nh, "..", "..", "bin")))
    out = []
    seen = set()
    for d in dirs:
        d = os.path.normpath(os.path.abspath(d))
        if d not in seen:
            seen.add(d)
            out.append(d)
    return out


def _tool(name):
    names = _exe_names(name)
    for n in names:
        exe = shutil.which(n)
        if exe:
            return exe
    for d in _tool_dirs():
        for n in names:
            cand = os.path.join(d, n)
            if os.path.isfile(cand):
                return cand
    assert False, "missing %s on PATH" % name


def _run(args, cwd=None, env=None, timeout=30):
    return subprocess.run(
        args,
        cwd=cwd,
        env=env,
        timeout=timeout,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        universal_newlines=True,
    )


def _write(path, text):
    parent = os.path.dirname(path)
    if parent and not os.path.isdir(parent):
        os.makedirs(parent)
    with open(path, "w") as f:
        f.write(text)


@pytest.mark.skipif(
    _asan_or_tsan(),
    reason="nrnivmodl subprocess import of neuron is late for sanitizer interceptors",
)
def test_nrnivmodl_area_mod_links(tmp_path):
    """nrnivmodl of an AREA mod must link nrn_alloc_node_.

    nocmodl areadec emits extern Node* nrn_alloc_node_ and uses it in
    nrn_alloc. WINDOWS_EXPORT_ALL_SYMBOLS does not cover data, so MSVC
    nrnmech.dll was LNK2019 (ModelDB 64195 ih_stochastic). Same NRN_DLLSYM
    as nrn_point_prop_.
    """
    _write(
        os.path.join(str(tmp_path), "areamod.mod"),
        "NEURON { SUFFIX areamod }\nASSIGNED { area }\nBREAKPOINT { }\n",
    )
    r = _run([_tool("nrnivmodl"), "areamod.mod"], cwd=str(tmp_path), timeout=180)
    out = r.stdout or ""
    assert r.returncode == 0, out
    assert "LNK2019" not in out
    assert "unresolved external" not in out.lower()


@pytest.mark.skipif(
    _asan_or_tsan(),
    reason="nrnivmodl subprocess import of neuron is late for sanitizer interceptors",
)
def test_nrnivmodl_state_discon_mod_links(tmp_path):
    """nrnivmodl of state_discontinuity() must link state_discon_flag_.

    nocmodl emits extern int state_discon_flag_ for BREAKPOINT use.
    WINDOWS_EXPORT_ALL_SYMBOLS does not cover data, so MSVC nrnmech.dll
    was LNK2001 (ModelDB 249705 glutamate and 12 others). Same NRN_DLLSYM
    as nrn_alloc_node_.
    """
    _write(
        os.path.join(str(tmp_path), "sdflag.mod"),
        "NEURON { SUFFIX sdflag }\n"
        "STATE { x }\n"
        "BREAKPOINT { SOLVE states METHOD cnexp\n"
        "  state_discontinuity(x, 0)\n"
        "}\n"
        "DERIVATIVE states { x' = -x }\n",
    )
    r = _run([_tool("nrnivmodl"), "sdflag.mod"], cwd=str(tmp_path), timeout=180)
    out = r.stdout or ""
    assert r.returncode == 0, out
    assert "LNK2019" not in out
    assert "LNK2001" not in out
    assert "unresolved external" not in out.lower()
