"""Plan (b): source-current discontinuities — PWL MOD + Section↔LM parity.

WP0b–WP3: PWLClamp electrode stimulus; mode-3 IClamp/PWL jump/kink;
Section↔LM parity (E0/E1); end-ri LM twins (Z0/Z1).

Run via ctest (preferred for CI — loads PWLClamp from hoctests nrnivmodl hash)::

    ctest -R hoctests::test_ida_source_current --output-on-failure

Or standalone (auto nrnivmodl of test/hoctests/pwlclamp.mod if needed)::

    export PATH=build/bin:$PATH LD_LIBRARY_PATH=build/lib:$LD_LIBRARY_PATH
    export PYTHONPATH=build/lib/python:$PYTHONPATH
    python test/hoctests/tests/test_ida_source_current.py

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
    if _PWL_MECH_DIR and os.path.isfile(os.path.join(_PWL_MECH_DIR, "libnrnmech.so")):
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
        raise RuntimeError(f"nrnivmodl failed ({nrnivmodl}):\n{r.stdout}\n{r.stderr}")
    arch = None
    for name in os.listdir(build):
        p = os.path.join(build, name)
        if os.path.isdir(p) and os.path.isfile(os.path.join(p, "libnrnmech.so")):
            arch = p
            break
    if not arch:
        raise RuntimeError(
            f"libnrnmech.so not found under {build}: {os.listdir(build)}"
        )
    _PWL_MECH_DIR = arch
    return arch


def _sanitizer_child_env(env=None):
    """Re-apply sanitizer preload for macOS SIP (see NeuronTestHelper.cmake)."""
    env = os.environ.copy() if env is None else env
    try:
        env[os.environ["NRN_SANITIZER_PRELOAD_VAR"]] = os.environ[
            "NRN_SANITIZER_PRELOAD_VAL"
        ]
    except KeyError:
        pass
    return env


def _run_isolated(code: str, timeout: float = 120.0) -> str:
    """Run code in a subprocess with the same interpreter/env; return stdout."""
    env = _sanitizer_child_env()
    # Point children at a ready PWLClamp mechanism directory
    try:
        env["NRN_PWLCLAMP_MECH"] = _build_pwlclamp_mech_dir()
    except Exception as e:
        # IClamp-only tests do not need the mech
        env.pop("NRN_PWLCLAMP_MECH", None)
        env["NRN_PWLCLAMP_BUILD_ERR"] = str(e)
    exe = os.environ.get("NRN_PYTHON_EXECUTABLE", sys.executable)
    proc = subprocess.run(
        [exe, "-c", code],
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


def test_e1_extracellular_pwl_istep_mode3():
    """WP3 E1: 1-layer extracellular + PWL istep; mode 3 holds Vm, path_mode=3.

    Uses nlayer_extracellular(1).  Sets xg/xc on all segments (including ends).
    Requires coupled cm+xc seed (electrode current in F[vi] charges both).
    """
    code = (
        _LOADER
        + _SET_PWL
        + r"""
from neuron import h
import math
h.load_file('stdrun.hoc')
h.nlayer_extracellular(1)
cvode = h.CVode()
s = h.Section(name='s')
s.L = s.diam = 10
s.nseg = 1
s.insert('pas')
s.g_pas = 1e-5
s.e_pas = 0.0
s.insert('extracellular')
for seg in s:
    seg.xg[0] = 1e-3
    seg.xc[0] = 1.0
stim = h.PWLClamp(s(0.5))
set_pwl(stim, [0, 1, 1, 5], [0, 0, 0.1, 0.1])
h.cvode_active(True)
cvode.use_daspk(1)
cvode.dae_init_mode(3)
cvode.dae_init_stats(1)
h.finitialize(0.0)
h.continuerun(0.999)
vm0 = s(0.5).v  # transmembrane Vm with extracellular
h.continuerun(1.0)
vm1 = s(0.5).v  # transmembrane Vm with extracellular
st = h.Vector()
cvode.dae_init_stats(st)
assert int(st[6]) == 3, list(st)
assert st[2] == 0, ('fallback', list(st))
assert abs(vm1 - vm0) < 1e-6, (vm0, vm1)
assert math.isfinite(s(0.5).v)
print('ok')
"""
    )
    assert "ok" in _run_isolated(code)


def test_e1_section_lm_parity_istep_kink():
    """WP3 E1 parity: Section+xtral+PWL vs LM floating Cm + xg + play force.

    LM twin (2-node): y0=v_int, y1=vext; floating C between them; xg on vext;
    I into y0 via play.  Compare Vm continuity and mode-3 path on both.
    """
    code = (
        _LOADER
        + _SET_PWL
        + r"""
from neuron import h
import math
h.load_file('stdrun.hoc')

def run_section(tvec, ivec, t_event):
    h.nlayer_extracellular(1)
    cvode = h.CVode()
    s = h.Section(name='s')
    s.L = s.diam = 10
    s.nseg = 1
    s.insert('pas')
    s.g_pas = 1e-5
    s.e_pas = 0.0
    s.insert('extracellular')
    for seg in s:
        seg.xg[0] = 1e-3
        seg.xc[0] = 1.0
    stim = h.PWLClamp(s(0.5))
    set_pwl(stim, tvec, ivec)
    h.cvode_active(True)
    cvode.use_daspk(1)
    cvode.dae_init_mode(3)
    cvode.dae_init_stats(1)
    h.finitialize(0.0)
    h.continuerun(t_event - 1e-4)
    vm0 = s(0.5).v  # transmembrane Vm with extracellular
    h.continuerun(t_event)
    vm1 = s(0.5).v  # transmembrane Vm with extracellular
    st = h.Vector()
    cvode.dae_init_stats(st)
    return vm0, vm1, list(st), stim.ival(t_event)

def run_lm(tvec, ivec, t_event):
    cvode = h.CVode()
    Cm, xg = 1.0, 1e-3
    c = h.Matrix(2, 2)
    g = h.Matrix(2, 2)
    y = h.Vector(2)
    y0 = h.Vector(2)
    b = h.Vector([0.0, 0.0])
    c.setval(0, 0, Cm); c.setval(0, 1, -Cm)
    c.setval(1, 0, -Cm); c.setval(1, 1, Cm)
    g.setval(1, 1, xg)
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
    vm0 = y[0] - y[1]
    h.continuerun(t_event)
    vm1 = y[0] - y[1]
    st = h.Vector()
    cvode.dae_init_stats(st)
    return vm0, vm1, list(st), b[0]

def check(name, tvec, ivec, te):
    svm0, svm1, sst, si = run_section(tvec, ivec, te)
    lvm0, lvm1, lst, li = run_lm(tvec, ivec, te)
    assert int(sst[6]) == 3 and sst[2] == 0, (name, 'sec', sst)
    assert int(lst[6]) == 3 and lst[2] == 0, (name, 'lm', lst)
    assert abs(svm1 - svm0) < 1e-6, (name, 'sec dVm', svm0, svm1)
    assert abs(lvm1 - lvm0) < 1e-6, (name, 'lm dVm', lvm0, lvm1)
    assert abs(si - li) < 1e-9, (name, 'I', si, li)
    print(name, 'ok')

check('istep', [0, 1, 1, 5], [0, 0, 0.1, 0.1], 1.0)
check('kink', [0, 1, 2], [0, 0, 1], 1.0)
print('ok')
"""
    )
    assert "ok" in _run_isolated(code)


def test_end_electrode_mode3_path():
    """A: IClamp at loc 0 and 1; mode 3 path_mode==3 (cable free-y)."""
    code = r"""
from neuron import h
h.load_file('stdrun.hoc')
for loc in (0.0, 1.0):
    cv = h.CVode()
    s = h.Section()
    s.L = 100
    s.diam = 10
    s.nseg = 5
    s.Ra = 100
    s.insert('pas')
    s.g_pas = 1e-5
    s.e_pas = 0.0
    ic = h.IClamp(s(loc))
    ic.delay = 1.0
    ic.dur = 1e9
    ic.amp = 0.05
    h.cvode_active(True)
    cv.use_daspk(1)
    cv.dae_init_mode(3)
    cv.dae_init_stats(1)
    h.finitialize(0.0)
    h.continuerun(0.999)
    vmid0 = s(0.5).v
    h.continuerun(1.0)
    st = h.Vector()
    cv.dae_init_stats(st)
    assert int(st[6]) == 3 and st[2] == 0, (loc, list(st))
    assert abs(s(0.5).v - vmid0) < 1e-6, (loc, vmid0, s(0.5).v)
print('ok')
"""
    assert "ok" in _run_isolated(code)


def test_z0_end_ri_lm_parity_istep():
    """WP3 Z0: end loc 0; LM twin uses sec(0.0001).ri(); Section path_mode=3."""
    code = (
        _LOADER
        + _SET_PWL
        + r"""
from neuron import h
import math
h.load_file('stdrun.hoc')

s = h.Section(name='s')
s.L = 100
s.diam = 10
s.nseg = 5
s.Ra = 100
s.insert('pas')
s.g_pas = 1e-5
s.e_pas = 0.0
h.finitialize(0.0)
R_end = s(0.0001).ri()
assert R_end > 0, R_end
Iamp = 0.05
# Drop cable before LM so sparse13 is pure linear mechanism
for sec in list(h.allsec()):
    h.delete_section(sec=sec)

# --- LM twin ---
cv2 = h.CVode()
c = h.Matrix(2, 2)
g = h.Matrix(2, 2)
y = h.Vector(2)
y0 = h.Vector(2)
b = h.Vector([0.0, 0.0])
c.setval(1, 1, 1.0)
g.setval(0, 0, 1.0 / R_end)
g.setval(0, 1, -1.0 / R_end)
g.setval(1, 0, -1.0 / R_end)
g.setval(1, 1, 1.0 / R_end)
lm = h.LinearMechanism(c, g, y, y0, b)
tv = h.Vector([0, 1, 1, 5])
iv = h.Vector([0, 0, Iamp, Iamp])
iv.play(b._ref_x[0], tv, True)
h.cvode_active(True)
cv2.use_daspk(1)
cv2.dae_init_mode(3)
cv2.dae_init_stats(1)
h.finitialize(0.0)
h.continuerun(1.0)
cv2.re_init()
st2 = h.Vector()
cv2.dae_init_stats(st2)
assert int(st2[6]) == 3 and st2[2] == 0, list(st2)
assert abs(y[1]) < 1e-6, y[1]
assert math.isclose(y[0], Iamp * R_end, rel_tol=1e-4, abs_tol=1e-4), (y[0], Iamp * R_end)
print('ok', 'R_end', R_end, 'I*R', Iamp * R_end)
"""
    )
    assert "ok" in _run_isolated(code)

    # Section end path in its own process (no LM)
    code_sec = (
        _LOADER
        + _SET_PWL
        + r"""
from neuron import h
h.load_file('stdrun.hoc')
s = h.Section(name='s')
s.L = 100
s.diam = 10
s.nseg = 5
s.Ra = 100
s.insert('pas')
s.g_pas = 1e-5
s.e_pas = 0.0
stim = h.PWLClamp(s(0.0))
set_pwl(stim, [0, 1, 1, 5], [0, 0, 0.05, 0.05])
cvode = h.CVode()
h.cvode_active(True)
cvode.use_daspk(1)
cvode.dae_init_mode(3)
cvode.dae_init_stats(1)
h.finitialize(0.0)
seg = s(0.1)
h.continuerun(0.999)
vm0 = seg.v
h.continuerun(1.0)
vm1 = seg.v
st = h.Vector()
cvode.dae_init_stats(st)
assert int(st[6]) == 3 and st[2] == 0, list(st)
assert abs(vm1 - vm0) < 1e-3, (vm0, vm1)
print('ok')
"""
    )
    assert "ok" in _run_isolated(code_sec)


def test_z1_end_ri_lm_parity_istep():
    """WP3 Z1: loc 1; LM series sec(1.0).ri(); Section path_mode=3."""
    code = (
        _LOADER
        + _SET_PWL
        + r"""
from neuron import h
import math
h.load_file('stdrun.hoc')
s = h.Section(name='s')
s.L = 100
s.diam = 10
s.nseg = 5
s.Ra = 100
s.insert('pas')
s.g_pas = 1e-5
s.e_pas = 0.0
h.finitialize(0.0)
R_end = s(1.0).ri()
Iamp = 0.05
for sec in list(h.allsec()):
    h.delete_section(sec=sec)

cv2 = h.CVode()
c = h.Matrix(2, 2)
g = h.Matrix(2, 2)
y = h.Vector(2)
y0 = h.Vector(2)
b = h.Vector([0.0, 0.0])
c.setval(1, 1, 1.0)
g.setval(0, 0, 1.0 / R_end)
g.setval(0, 1, -1.0 / R_end)
g.setval(1, 0, -1.0 / R_end)
g.setval(1, 1, 1.0 / R_end)
lm = h.LinearMechanism(c, g, y, y0, b)
tv = h.Vector([0, 1, 1, 5])
iv = h.Vector([0, 0, Iamp, Iamp])
iv.play(b._ref_x[0], tv, True)
h.cvode_active(True)
cv2.use_daspk(1)
cv2.dae_init_mode(3)
cv2.dae_init_stats(1)
h.finitialize(0.0)
h.continuerun(1.0)
cv2.re_init()
st2 = h.Vector()
cv2.dae_init_stats(st2)
assert int(st2[6]) == 3 and st2[2] == 0, list(st2)
assert abs(y[1]) < 1e-6, y[1]
assert math.isclose(y[0], Iamp * R_end, rel_tol=1e-4, abs_tol=1e-4), (y[0], Iamp * R_end)
print('ok')
"""
    )
    assert "ok" in _run_isolated(code)

    code_sec = (
        _LOADER
        + _SET_PWL
        + r"""
from neuron import h
h.load_file('stdrun.hoc')
s = h.Section(name='s')
s.L = 100
s.diam = 10
s.nseg = 5
s.Ra = 100
s.insert('pas')
s.g_pas = 1e-5
s.e_pas = 0.0
stim = h.PWLClamp(s(1.0))
set_pwl(stim, [0, 1, 1, 5], [0, 0, 0.05, 0.05])
cvode = h.CVode()
h.cvode_active(True)
cvode.use_daspk(1)
cvode.dae_init_mode(3)
cvode.dae_init_stats(1)
h.finitialize(0.0)
seg = s(0.9)
h.continuerun(0.999)
vm0 = seg.v
h.continuerun(1.0)
vm1 = seg.v
st = h.Vector()
cvode.dae_init_stats(st)
assert int(st[6]) == 3 and st[2] == 0, list(st)
assert abs(vm1 - vm0) < 1e-3, (vm0, vm1)
print('ok')
"""
    )
    assert "ok" in _run_isolated(code_sec)


def test_e2_xc0_extracellular_istep():
    """B E2: xc=0 (resistive xtral) + IClamp step; mode 3 holds Vm (=seg.v).

    With extracellular, seg.v is transmembrane Vm; vext may jump when xc=0.
    Do not use v-vext as Vm.
    """
    code = r"""
from neuron import h
h.load_file('stdrun.hoc')
h.nlayer_extracellular(1)
cv = h.CVode()
s = h.Section()
s.L = s.diam = 10
s.nseg = 1
s.insert('pas')
s.g_pas = 1e-5
s.e_pas = 0.0
s.insert('extracellular')
for seg in s:
    seg.xg[0] = 1e-3
    seg.xc[0] = 0.0
ic = h.IClamp(s(0.5))
ic.delay = 1.0
ic.dur = 1e9
ic.amp = 0.1
h.cvode_active(True)
cv.use_daspk(1)
cv.dae_init_mode(3)
cv.dae_init_stats(1)
h.finitialize(0.0)
h.continuerun(0.999)
vm0 = s(0.5).v  # transmembrane
vext0 = s(0.5).vext[0]
h.continuerun(1.0)
vm1 = s(0.5).v
vext1 = s(0.5).vext[0]
st = h.Vector()
cv.dae_init_stats(st)
assert int(st[6]) == 3 and st[2] == 0, list(st)
assert abs(vm1 - vm0) < 1e-6, (vm0, vm1, vext0, vext1)
# algebraic outer layer: vext free to jump under electrode step
assert abs(vext1 - vext0) > 1e-6 or abs(ic.amp) < 1e-12
print('ok', 'dVm', vm1 - vm0, 'dvext', vext1 - vext0)
"""
    assert "ok" in _run_isolated(code)


def test_e3_multilayer_xc_istep():
    """B E3: default nlayer=2 with xc>0 on both layers; mode 3 istep."""
    code = r"""
from neuron import h
h.load_file('stdrun.hoc')
# default nlayer is 2
cv = h.CVode()
s = h.Section()
s.L = s.diam = 10
s.nseg = 1
s.insert('pas')
s.g_pas = 1e-5
s.e_pas = 0.0
s.insert('extracellular')
nl = int(h.nlayer_extracellular())
assert nl >= 2, nl
for seg in s:
    for j in range(nl):
        seg.xg[j] = 1e-3
        seg.xc[j] = 1.0
ic = h.IClamp(s(0.5))
ic.delay = 1.0
ic.dur = 1e9
ic.amp = 0.1
h.cvode_active(True)
cv.use_daspk(1)
cv.dae_init_mode(3)
cv.dae_init_stats(1)
h.finitialize(0.0)
h.continuerun(0.999)
vm0 = s(0.5).v  # transmembrane Vm with extracellular
h.continuerun(1.0)
vm1 = s(0.5).v  # transmembrane Vm with extracellular
st = h.Vector()
cv.dae_init_stats(st)
assert int(st[6]) == 3 and st[2] == 0, list(st)
assert abs(vm1 - vm0) < 1e-6, (vm0, vm1)
print('ok', 'nlayer', nl)
"""
    assert "ok" in _run_isolated(code)


def test_e4_xg_scale_istep():
    """B E4: weak and strong xg; mode 3 istep holds Vm."""
    code = r"""
from neuron import h
h.load_file('stdrun.hoc')
h.nlayer_extracellular(1)
for xg in (1e-6, 1e-3, 1.0):
    cv = h.CVode()
    s = h.Section()
    s.L = s.diam = 10
    s.nseg = 1
    s.insert('pas')
    s.g_pas = 1e-5
    s.e_pas = 0.0
    s.insert('extracellular')
    for seg in s:
        seg.xg[0] = xg
        seg.xc[0] = 1.0
    ic = h.IClamp(s(0.5))
    ic.delay = 1.0
    ic.dur = 1e9
    ic.amp = 0.1
    h.cvode_active(True)
    cv.use_daspk(1)
    cv.dae_init_mode(3)
    cv.dae_init_stats(1)
    h.finitialize(0.0)
    h.continuerun(0.999)
    vm0 = s(0.5).v  # transmembrane Vm with extracellular
    h.continuerun(1.0)
    vm1 = s(0.5).v  # transmembrane Vm with extracellular
    st = h.Vector()
    cv.dae_init_stats(st)
    assert int(st[6]) == 3 and st[2] == 0, (xg, list(st))
    assert abs(vm1 - vm0) < 1e-6, (xg, vm0, vm1)
    print('xg', xg, 'ok')
print('ok')
"""
    assert "ok" in _run_isolated(code)


def test_e5_seclamp_two_algebraic_layers():
    """SEClamp step + default nlayer=2, all xc=0: mode 3 keeps the xg ladder.

    Regression: the battery restore used to paint every algebraic layer with
    a common-mode from vext[0], so vext[1]==vext[0] and xg[1]*vext[1] blew
    the residual (test1.py / xg[1]=0.002 or default 1e9).
    SEClamp sees vi=v+vext; Vm=seg.v is held; layer voltages follow the
    resistive divider.
    """
    code = r"""
from neuron import h
h.load_file('stdrun.hoc')
assert int(h.nlayer_extracellular()) >= 2
for xg1 in (2e-3, 1e9):
    cv = h.CVode()
    s = h.Section()
    s.L = s.diam = 10
    s.nseg = 1
    s.insert('extracellular')
    nl = int(h.nlayer_extracellular())
    assert nl >= 2, nl
    for seg in s:
        for j in range(nl):
            seg.xc[j] = 0.0
        seg.xg[0] = 1e-3
        seg.xg[1] = xg1
    vc = h.SEClamp(s(0.5))
    vc.rs = 10
    vc.dur1 = 1
    vc.amp1 = -65
    vc.dur2 = 1
    vc.amp2 = 10
    h.cvode_active(True)
    cv.use_daspk(1)
    cv.dae_init_mode(3)
    cv.dae_init_stats(1)
    h.finitialize(-65)
    h.continuerun(0.999)
    vm0 = s(0.5).v
    h.continuerun(1.0)
    vm1 = s(0.5).v
    # Python RangeVar vext[i] always aliases layer 0; read via HOC.
    s.push()
    h('hoc_ac_ = vext[0](.5)')
    vx0 = float(h.hoc_ac_)
    h('hoc_ac_ = vext[1](.5)')
    vx1 = float(h.hoc_ac_)
    h.pop_section()
    st = h.Vector()
    cv.dae_init_stats(st)
    assert int(st[6]) == 3 and st[2] == 0, (xg1, list(st))
    assert abs(vm1 - vm0) < 1e-6, (xg1, vm0, vm1)
    # Resistive divider: i = (vc - (Vm+vext0))/rs = xg_eq * vext0 * area*0.01
    xg0 = 1e-3
    xg_eq = 1.0 / (1.0 / xg0 + 1.0 / xg1)
    area_fac = s(0.5).area() * 0.01
    vx0_exp = (10.0 - vm1) / (1.0 + vc.rs * area_fac * xg_eq)
    vx1_exp = vx0_exp * (1.0 / xg1) / (1.0 / xg0 + 1.0 / xg1)
    assert abs(vx0 - vx0_exp) < 1e-3, (xg1, vx0, vx0_exp, vx1)
    assert abs(vx1 - vx1_exp) < 1e-3, (xg1, vx1, vx1_exp, vx0)
    # The bug was vext[1] == vext[0] (not the split)
    assert abs(vx1 - vx0) > 1.0, (xg1, vx0, vx1)
    print('xg1', xg1, 'ok', 'vext', vx0, vx1)
print('ok')
"""
    assert "ok" in _run_isolated(code)


if __name__ == "__main__":
    test_pwlclamp_ival_right_continuous()
    test_iclamp_step_mode3_section()
    test_pwl_istep_mode3_section()
    test_pwl_kink_mode3_section()
    test_pwl_jump_ramp_section_mode3()
    test_e0_section_lm_parity_istep_kink()
    test_e1_extracellular_pwl_istep_mode3()
    test_e1_section_lm_parity_istep_kink()
    test_end_electrode_mode3_path()
    test_z0_end_ri_lm_parity_istep()
    test_z1_end_ri_lm_parity_istep()
    test_e2_xc0_extracellular_istep()
    test_e3_multilayer_xc_istep()
    test_e4_xg_scale_istep()
    test_e5_seclamp_two_algebraic_layers()
    print("all ok")
