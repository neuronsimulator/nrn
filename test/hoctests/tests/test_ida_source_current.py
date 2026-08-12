"""Plan (b): source-current discontinuities — PWL MOD + Section↔LM parity.

WP0b / WP1 / WP1b: PWLClamp electrode stimulus; mode-3 IClamp/PWL jump;
capacitive Section vs LM under jump and kink waveforms.

Each model case runs in a subprocess (LinearMechanism teardown hygiene).
"""

from __future__ import annotations

import os
import subprocess
import sys
import tempfile

# Canonical waveform tables (same as test_ida_init_mode A1/A3 play)
W_ISTEP = ([0.0, 1.0, 1.0, 5.0], [0.0, 0.0, 0.5, 0.5])
W_KINK = ([0.0, 1.0, 2.0], [0.0, 0.0, 1.0])
W_JUMP_RAMP = ([0.0, 1.0, 1.0, 2.0, 2.0, 5.0], [0.0, 0.0, 0.5, 1.0, 0.0, 0.0])


def _repo_hoctests():
    return os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))


def _find_nrnivmodl() -> str:
    env = os.environ.get("NRNIVMODL")
    if env and os.path.isfile(env):
        return env
    # Prefer build tree next to a PYTHONPATH neuron install
    for d in os.environ.get("PATH", "").split(os.pathsep):
        cand = os.path.join(d, "nrnivmodl")
        if os.path.isfile(cand) and os.access(cand, os.X_OK):
            # Skip broken pyenv shims that point at missing .data/bin
            try:
                r = subprocess.run(
                    [cand, "-help"],
                    capture_output=True,
                    text=True,
                    timeout=10,
                )
                # nrnivmodl -help may exit nonzero; existence of binary is enough if
                # it is the cmake-installed script under build/bin
                if "build/bin" in cand or r.returncode in (0, 1, 2):
                    if os.path.isfile(cand):
                        return cand
            except Exception:
                continue
    return "nrnivmodl"


_PWL_MECH_DIR = None


def _build_pwlclamp_mech_dir() -> str:
    """Once-per-process compile of pwlclamp.mod; return dir with libnrnmech.so."""
    global _PWL_MECH_DIR
    if _PWL_MECH_DIR and os.path.isfile(
        os.path.join(_PWL_MECH_DIR, "libnrnmech.so")
    ):
        return _PWL_MECH_DIR
    # Reuse prior local build if present
    cached = "/tmp/pwlclamp_build/x86_64"
    if os.path.isfile(os.path.join(cached, "libnrnmech.so")):
        _PWL_MECH_DIR = cached
        return cached
    mod = os.path.join(_repo_hoctests(), "pwlclamp.mod")
    build = tempfile.mkdtemp(prefix="pwlclamp_")
    nrnivmodl = _find_nrnivmodl()
    r = subprocess.run(
        [nrnivmodl, mod],
        cwd=build,
        capture_output=True,
        text=True,
        env=os.environ.copy(),
    )
    if r.returncode != 0:
        raise RuntimeError(
            f"nrnivmodl failed ({nrnivmodl}):\n{r.stdout}\n{r.stderr}"
        )
    arch = None
    for name in os.listdir(build):
        p = os.path.join(build, name)
        if os.path.isdir(p) and os.path.isfile(os.path.join(p, "libnrnmech.so")):
            arch = p
            break
    if not arch:
        raise RuntimeError(f"libnrnmech.so not found under {build}: {os.listdir(build)}")
    _PWL_MECH_DIR = arch
    return arch


def _run_isolated(code: str, timeout: float = 120.0) -> str:
    """Run code in a subprocess with the same interpreter/env; return stdout."""
    env = os.environ.copy()
    # Point children at a ready PWLClamp mechanism directory
    try:
        env["NRN_PWLCLAMP_MECH"] = _build_pwlclamp_mech_dir()
    except Exception as e:
        # IClamp-only tests do not need the mech
        env.pop("NRN_PWLCLAMP_MECH", None)
        env["NRN_PWLCLAMP_BUILD_ERR"] = str(e)
    proc = subprocess.run(
        [sys.executable, "-c", code],
        capture_output=True,
        text=True,
        timeout=timeout,
        env=env,
    )
    out = (proc.stdout or "") + (proc.stderr or "")
    if proc.returncode != 0:
        raise AssertionError(
            f"subprocess failed rc={proc.returncode}\n--- stdout+stderr ---\n{out}"
        )
    return out


_LOADER = r"""
import os
from neuron import h

def ensure_pwlclamp():
    if hasattr(h, 'PWLClamp'):
        return
    candidates = []
    mech = os.environ.get('NRN_PWLCLAMP_MECH')
    if mech:
        candidates.append(mech)
    for d in os.environ.get('PATH', '').split(os.pathsep):
        if d and os.path.isdir(d):
            candidates.append(d)
    for d in candidates:
        so = os.path.join(d, 'libnrnmech.so')
        if os.path.isfile(so):
            h.nrn_load_dll(so)
            if hasattr(h, 'PWLClamp'):
                return
    err = os.environ.get('NRN_PWLCLAMP_BUILD_ERR', '')
    raise RuntimeError(
        'PWLClamp not available; set NRN_PWLCLAMP_MECH to dir with libnrnmech.so. '
        + err
    )

ensure_pwlclamp()
"""


_SET_PWL = r"""
def set_pwl(stim, tvec, ivec):
    assert len(tvec) == len(ivec) and len(tvec) <= 16
    stim.nkt = len(tvec)
    for j, (tt, ii) in enumerate(zip(tvec, ivec)):
        stim.set_knot(j, tt, ii)
"""


def test_pwlclamp_ival_right_continuous():
    """WP0b: PWLClamp.ival matches jump/kink tables at knot t+."""
    code = (
        _LOADER
        + _SET_PWL
        + r"""
from neuron import h
s = h.Section(name='s')
stim = h.PWLClamp(s(0.5))
# istep at t=1+: I=0.5
set_pwl(stim, [0, 1, 1, 5], [0, 0, 0.5, 0.5])
assert abs(stim.ival(1.0) - 0.5) < 1e-12, stim.ival(1.0)
assert abs(stim.ival(0.5) - 0.0) < 1e-12, stim.ival(0.5)
assert abs(stim.ival(3.0) - 0.5) < 1e-12, stim.ival(3.0)
# kink at t=1: I=0, then ramp
set_pwl(stim, [0, 1, 2], [0, 0, 1])
assert abs(stim.ival(1.0) - 0.0) < 1e-12, stim.ival(1.0)
assert abs(stim.ival(1.5) - 0.5) < 1e-12, stim.ival(1.5)
# jump-ramp A1 at t=1+: I=0.5
set_pwl(stim, [0, 1, 1, 2, 2, 5], [0, 0, 0.5, 1.0, 0, 0])
assert abs(stim.ival(1.0) - 0.5) < 1e-12, stim.ival(1.0)
print('ok')
"""
    )
    assert "ok" in _run_isolated(code)


def test_iclamp_step_mode3_section():
    """WP1: pure capacitive Section + IClamp step; mode 3 path, Vm hold."""
    code = r"""
from neuron import h
import math
h.load_file('stdrun.hoc')
cvode = h.CVode()
s = h.Section(name='s')
s.L = s.diam = 10
s.insert('pas')
s.g_pas = 1e-4
s.e_pas = -65
ic = h.IClamp(s(0.5))
ic.delay = 1.0
ic.dur = 1e9
ic.amp = 0.1
h.cvode_active(True)
cvode.use_daspk(1)
cvode.dae_init_mode(3)
cvode.dae_init_stats(1)
h.finitialize(-65)
h.continuerun(0.999)
v_before = s(0.5).v
h.continuerun(1.0)
# at/after step: path_mode 3, residual ok via stats
v = h.Vector()
n = cvode.dae_init_stats(v)
assert n == 8 and v.size() == 8, list(v)
# After continuerun past del, at least one reinit should have used mode 3
# v[6] last path mode, v[1] mode3 ok count (see A5 layout)
assert int(v[6]) == 3 or v[1] >= 1, list(v)
# Vm continuous across the step (charge hold); allow tiny float drift
v_after = s(0.5).v
assert abs(v_after - v_before) < 1e-3, (v_before, v_after)
assert math.isfinite(v_after)
# integrate a bit further
h.continuerun(1.5)
assert math.isfinite(s(0.5).v)
print('ok')
"""
    assert "ok" in _run_isolated(code)


def test_pwl_istep_mode3_section():
    """WP1: W_istep via PWLClamp on capacitive Section; mode 3."""
    code = (
        _LOADER
        + _SET_PWL
        + r"""
from neuron import h
import math
h.load_file('stdrun.hoc')
cvode = h.CVode()
s = h.Section(name='s')
s.L = s.diam = 10
s.insert('pas')
s.g_pas = 1e-4
s.e_pas = -65
stim = h.PWLClamp(s(0.5))
set_pwl(stim, [0, 1, 1, 5], [0, 0, 0.5, 0.5])
h.cvode_active(True)
cvode.use_daspk(1)
cvode.dae_init_mode(3)
cvode.dae_init_stats(1)
h.finitialize(-65)
h.continuerun(0.999)
v_before = s(0.5).v
h.continuerun(1.0)
v = h.Vector()
cvode.dae_init_stats(v)
assert int(v[6]) == 3 or v[1] >= 1, list(v)
assert abs(s(0.5).v - v_before) < 1e-3, (v_before, s(0.5).v)
assert abs(stim.ival(1.0) - 0.5) < 1e-12
h.continuerun(1.2)
assert math.isfinite(s(0.5).v)
print('ok')
"""
    )
    assert "ok" in _run_isolated(code)


def test_pwl_kink_mode3_section():
    """WP1b: W_kink via PWLClamp; continuous I, slope change; mode 3."""
    code = (
        _LOADER
        + _SET_PWL
        + r"""
from neuron import h
import math
h.load_file('stdrun.hoc')
cvode = h.CVode()
s = h.Section(name='s')
s.L = s.diam = 10
s.insert('pas')
s.g_pas = 1e-4
s.e_pas = -65
stim = h.PWLClamp(s(0.5))
set_pwl(stim, [0, 1, 2], [0, 0, 1])
h.cvode_active(True)
cvode.use_daspk(1)
cvode.dae_init_mode(3)
cvode.dae_init_stats(1)
h.finitialize(-65)
h.continuerun(0.999)
v_before = s(0.5).v
h.continuerun(1.0)
v = h.Vector()
cvode.dae_init_stats(v)
assert int(v[6]) == 3 or v[1] >= 1, list(v)
assert abs(s(0.5).v - v_before) < 1e-3, (v_before, s(0.5).v)
assert abs(stim.ival(1.0) - 0.0) < 1e-12
h.continuerun(1.5)
assert math.isfinite(s(0.5).v)
print('ok')
"""
    )
    assert "ok" in _run_isolated(code)


def test_e0_section_lm_parity_istep_kink():
    """WP1b / E0: Section+PWL vs LM+play for W_istep and W_kink (capacitive).

    LM: single C to ground + leak R + I into the node (play).
    Section: pas + PWLClamp mid-compartment.
    Compare relative Vm change across the event and mode-3 success.
    Absolute IR drop scaling differs (area units); compare continuity and path.
    """
    code = (
        _LOADER
        + _SET_PWL
        + r"""
from neuron import h
import math
h.load_file('stdrun.hoc')

def run_section(tvec, ivec, t_event):
    cvode = h.CVode()
    s = h.Section(name='s')
    s.L = s.diam = 10
    s.insert('pas')
    s.g_pas = 0.001
    s.e_pas = 0.0
    s.cm = 1.0
    stim = h.PWLClamp(s(0.5))
    set_pwl(stim, tvec, ivec)
    h.cvode_active(True)
    cvode.use_daspk(1)
    cvode.dae_init_mode(3)
    cvode.dae_init_stats(1)
    h.finitialize(0.0)
    h.continuerun(t_event - 1e-4)
    v0 = s(0.5).v
    h.continuerun(t_event)
    v1 = s(0.5).v
    stats = h.Vector()
    cvode.dae_init_stats(stats)
    return v0, v1, list(stats), stim.ival(t_event)

def run_lm(tvec, ivec, t_event):
    cvode = h.CVode()
    # C=1, R=1e3 (weak leak), I into node 0; single voltage unknown
    # Use 1-state: C v' + v/R = I
    c = h.Matrix(1, 1)
    g = h.Matrix(1, 1)
    y = h.Vector(1)
    y0 = h.Vector(1)
    b = h.Vector([0.0])
    c.setval(0, 0, 1.0)
    g.setval(0, 0, 1.0 / 1e3)
    lm = h.LinearMechanism(c, g, y, y0, b)
    tv = h.Vector(tvec)
    iv = h.Vector(ivec)
    iv.play(b._ref_x[0], tv, True)
    h.cvode_active(True)
    cvode.use_daspk(1)
    cvode.dae_init_mode(3)
    cvode.dae_init_stats(1)
    h.finitialize(0.0)
    h.continuerun(t_event - 1e-4)
    v0 = y[0]
    h.continuerun(t_event)
    v1 = y[0]
    stats = h.Vector()
    cvode.dae_init_stats(stats)
    return v0, v1, list(stats), b[0]

def check_case(name, tvec, ivec, t_event):
    sv0, sv1, sstats, si = run_section(tvec, ivec, t_event)
    lv0, lv1, lstats, li = run_lm(tvec, ivec, t_event)
    # mode 3 used on both
    assert int(sstats[6]) == 3 or sstats[1] >= 1, (name, 'sec stats', sstats)
    assert int(lstats[6]) == 3 or lstats[1] >= 1, (name, 'lm stats', lstats)
    # continuous content: voltage continuous across event
    assert abs(sv1 - sv0) < 1e-3, (name, 'sec dV', sv0, sv1)
    assert abs(lv1 - lv0) < 1e-3, (name, 'lm dV', lv0, lv1)
    # drive value at t+ agrees
    assert abs(si - li) < 1e-9, (name, 'I', si, li)
    print(name, 'ok', 'I=', si, 'sec dV=', sv1-sv0, 'lm dV=', lv1-lv0)

check_case('istep', [0, 1, 1, 5], [0, 0, 0.5, 0.5], 1.0)
check_case('kink', [0, 1, 2], [0, 0, 1], 1.0)
print('ok')
"""
    )
    assert "ok" in _run_isolated(code)


def test_pwl_jump_ramp_section_mode3():
    """WP1b: W_jump_ramp on Section via PWLClamp (A1-style table)."""
    code = (
        _LOADER
        + _SET_PWL
        + r"""
from neuron import h
import math
h.load_file('stdrun.hoc')
cvode = h.CVode()
s = h.Section(name='s')
s.L = s.diam = 10
s.insert('pas')
s.g_pas = 1e-4
s.e_pas = -65
stim = h.PWLClamp(s(0.5))
set_pwl(stim, [0, 1, 1, 2, 2, 5], [0, 0, 0.5, 1.0, 0, 0])
h.cvode_active(True)
cvode.use_daspk(1)
cvode.dae_init_mode(3)
cvode.dae_init_stats(1)
h.finitialize(-65)
h.continuerun(0.999)
v_before = s(0.5).v
h.continuerun(1.0)
assert abs(stim.ival(1.0) - 0.5) < 1e-12
v = h.Vector()
cvode.dae_init_stats(v)
assert int(v[6]) == 3 or v[1] >= 1, list(v)
assert abs(s(0.5).v - v_before) < 1e-3, (v_before, s(0.5).v)
h.continuerun(1.5)
assert math.isfinite(s(0.5).v)
print('ok')
"""
    )
    assert "ok" in _run_isolated(code)


if __name__ == "__main__":
    test_pwlclamp_ival_right_continuous()
    test_iclamp_step_mode3_section()
    test_pwl_istep_mode3_section()
    test_pwl_kink_mode3_section()
    test_pwl_jump_ramp_section_mode3()
    test_e0_section_lm_parity_istep_kink()
    print("all ok")
