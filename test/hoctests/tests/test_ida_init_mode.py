"""Phase 0: CVode.dae_init_mode and IDA_Y_INIT with LinearMechanism.

Each model case runs in a subprocess so LinearMechanism teardown does not
interact across tests (NrnDAE / sparse13 matrix reallocation).
"""

from neuron import h
import math
import os
import subprocess
import sys

h.load_file("stdrun.hoc")
cvode = h.CVode()


def test_dae_init_mode_api():
    # Setter/getter only. Process default is NRN_DAE_INIT_MODE_DEFAULT at
    # import (unset → 0); that path is tested in a subprocess below.
    assert cvode.dae_init_mode(1) == 1
    assert cvode.dae_init_mode(2) == 2
    assert cvode.dae_init_mode(3) == 3
    assert cvode.dae_init_mode(0) == 0


def test_dae_init_audit_api():
    assert cvode.dae_init_audit() == 0
    assert cvode.dae_init_audit(1) == 1
    assert cvode.dae_init_audit(2, 5.0) == 2
    assert cvode.dae_init_audit(0) == 0
    assert cvode.dae_init_audit_file() == 0
    # empty path → stdout; non-empty sets file mode flag
    assert cvode.dae_init_audit_file("") == 0


def test_dae_init_stats_api():
    """A5: dae_init_stats reset / vector fill."""
    code = r"""
from neuron import h
h.load_file('stdrun.hoc')
cv = h.CVode()
cv.dae_init_stats(1)  # reset
v = h.Vector()
n = cv.dae_init_stats(v)
assert n == 8 and v.size() == 8, (n, v.size())
assert v[0] == 0  # no reinits yet
# series CR + play ramp once
c = h.Matrix(2, 2)
g = h.Matrix(2, 2)
y = h.Vector(2)
b = h.Vector(2)
c.setval(0, 0, 1); c.setval(0, 1, -1)
c.setval(1, 0, -1); c.setval(1, 1, 1)
g.setval(1, 1, 1)
lm = h.LinearMechanism(c, g, y, b)
tvec = h.Vector([0, 1, 1, 2])
ivec = h.Vector([0, 0, 0.5, 1.0])
ivec.play(b._ref_x[0], tvec, True)
h.cvode_active(True)
cv.use_daspk(1)
cv.dae_init_mode(3)
h.finitialize(0)
h.continuerun(1.0)
cv.re_init()
cv.dae_init_stats(v)
assert v[0] >= 2, list(v)  # finitialize + re_init at least
assert v[1] >= 1, list(v)  # mode3 ok
assert v[3] >= 1, list(v)  # play free y'
assert int(v[6]) == 3, list(v)  # last path mode 3
assert int(v[7]) & 8, list(v)  # APPLIED bit
print('ok')
"""
    assert "ok" in _run_isolated(code)


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


def _run_isolated(code: str, env_updates=None, *, capture_err=False):
    env = _sanitizer_child_env()
    if env_updates:
        for key, val in env_updates.items():
            if val is None:
                env.pop(key, None)
            else:
                env[key] = str(val)
    exe = os.environ.get("NRN_PYTHON_EXECUTABLE", sys.executable)
    r = subprocess.run(
        [exe, "-c", code],
        env=env,
        capture_output=True,
        text=True,
    )
    if r.returncode != 0:
        raise AssertionError(f"stdout:\n{r.stdout}\nstderr:\n{r.stderr}")
    if capture_err:
        return r.stdout, r.stderr
    return r.stdout


def test_dae_init_mode_env_default():
    """NRN_DAE_INIT_MODE_DEFAULT is read once at CVode registration."""
    probe = r"""
from neuron import h
print(int(h.CVode().dae_init_mode()))
"""
    out = _run_isolated(probe, {"NRN_DAE_INIT_MODE_DEFAULT": None})
    assert out.strip().splitlines()[-1] == "0", out

    out = _run_isolated(probe, {"NRN_DAE_INIT_MODE_DEFAULT": "3"})
    assert out.strip().splitlines()[-1] == "3", out

    override = r"""
from neuron import h
cv = h.CVode()
assert cv.dae_init_mode() == 3
assert cv.dae_init_mode(1) == 1
print('ok')
"""
    assert "ok" in _run_isolated(override, {"NRN_DAE_INIT_MODE_DEFAULT": "3"})

    invalid = r"""
from neuron import h
assert h.CVode().dae_init_mode() == 0
print('ok')
"""
    for bad in ("4", "foo", ""):
        out, err = _run_isolated(
            invalid, {"NRN_DAE_INIT_MODE_DEFAULT": bad}, capture_err=True
        )
        assert "ok" in out, (bad, out, err)
        assert "NRN_DAE_INIT_MODE_DEFAULT" in err, (bad, err)


def test_pure_resistive_all_modes_isolated():
    code = r"""
from neuron import h
import math
h.load_file('stdrun.hoc')
cvode = h.CVode()
c = h.Matrix(2, 2)
g = h.Matrix(2, 2)
y = h.Vector(2)
y0 = h.Vector(2)
b = h.Vector([1.0, 2.0])
g.setval(0, 0, 1.0)
g.setval(1, 1, 0.5)
lm = h.LinearMechanism(c, g, y, y0, b)
h.cvode_active(True)
for mode in (0, 1, 2, 3):
    cvode.dae_init_mode(mode)
    h.finitialize(0.0)
    assert math.isclose(y[0], 1.0, rel_tol=1e-6, abs_tol=1e-6), mode
    assert math.isclose(y[1], 4.0, rel_tol=1e-6, abs_tol=1e-6), mode
print('ok')
"""
    assert "ok" in _run_isolated(code)


def test_series_cr_mode0_mode1_isolated():
    code = r"""
from neuron import h
import math
h.load_file('stdrun.hoc')
cvode = h.CVode()
I, R, C = 1.0, 2.0, 1.0
c = h.Matrix(2, 2)
g = h.Matrix(2, 2)
y = h.Vector(2)
y0 = h.Vector(2)
b = h.Vector([I, 0.0])
c.setval(0, 0, C)
c.setval(0, 1, -C)
c.setval(1, 0, -C)
c.setval(1, 1, C)
g.setval(1, 1, 1.0 / R)
lm = h.LinearMechanism(c, g, y, y0, b)
h.cvode_active(True)
IR = I * R
for mode in (0, 1):
    cvode.dae_init_mode(mode)
    h.finitialize(0.0)
    assert math.isclose(y[0] - y[1], 0.0, abs_tol=1e-6), mode
    assert math.isclose(y[0], IR, rel_tol=1e-4, abs_tol=1e-4), (mode, y[0])
    assert math.isclose(y[1], IR, rel_tol=1e-4, abs_tol=1e-4), (mode, y[1])
cvode.dae_init_mode(1)
h.finitialize(0.0)
h.continuerun(0.5)
assert math.isclose(y[1], IR, rel_tol=1e-3, abs_tol=1e-3)
assert y[0] > y[1]
print('ok')
"""
    assert "ok" in _run_isolated(code)


def test_series_cr_battery_mode3_isolated():
    """Battery IC: C→V hold; absolute V jumps to I*R, Δv held at 0."""
    code = r"""
from neuron import h
import math
h.load_file('stdrun.hoc')
cvode = h.CVode()
I, R, Cval = 1.0, 2.0, 1.0
c = h.Matrix(2, 2)
g = h.Matrix(2, 2)
y = h.Vector(2)
y0 = h.Vector(2)
b = h.Vector([I, 0.0])
c.setval(0, 0, Cval)
c.setval(0, 1, -Cval)
c.setval(1, 0, -Cval)
c.setval(1, 1, Cval)
g.setval(1, 1, 1.0 / R)
lm = h.LinearMechanism(c, g, y, y0, b)
h.cvode_active(True)
cvode.dae_init_mode(3)
h.finitialize(0.0)
IR = I * R
assert math.isclose(y[0] - y[1], 0.0, abs_tol=1e-9), (y[0], y[1])
assert math.isclose(y[0], IR, rel_tol=1e-6, abs_tol=1e-6), y[0]
assert math.isclose(y[1], IR, rel_tol=1e-6, abs_tol=1e-6), y[1]
h.continuerun(0.5)
assert math.isclose(y[1], IR, rel_tol=1e-3, abs_tol=1e-3)
assert y[0] > y[1] - 1e-9
print('ok')
"""
    assert "ok" in _run_isolated(code)


def test_opamp_tau_battery_holds_output_voltage():
    """OpAmp lag (tau>0): C[o][k]=tau. Hold v_out, not bogus v_out-I.

    Before the inductor/opamp extension, mode 3 treated (v,I) as a floating
    capacitor and drove v→~1 (algebraic follower). Continuous IC from rest
    must keep v_out≈0 when + is stepped via b.
    """
    code = r"""
from neuron import h
import math
h.load_file('stdrun.hoc')
cvode = h.CVode()
# LinearCircuit OpAmp lag stamp (tau>0): y0=v_out, y1=I_out
#   tau*v' + (gain+1)*v = gain*Vplus
#   I + v/R = 0
gain, tau, R, Vplus = 1e6, 1.0, 1.0, 1.0
c = h.Matrix(2, 2)
g = h.Matrix(2, 2)
y = h.Vector(2)
y0 = h.Vector(2)
b = h.Vector([0.0, gain * Vplus])
c.setval(1, 0, tau)
g.setval(0, 0, 1.0 / R)
g.setval(0, 1, 1.0)
g.setval(1, 0, gain + 1.0)
lm = h.LinearMechanism(c, g, y, y0, b)
h.cvode_active(True)
cvode.dae_init_dteps(1e-9, 1)  # warn on residual, still check y
cvode.dae_init_mode(3)
h.finitialize(0.0)
# Continuous from rest: v_out held at 0, I=0 (not algebraic steady v~1)
assert abs(y[0]) < 1e-6, f'v_out should be held near 0, got {y[0]}'
assert abs(y[1]) < 1e-6, f'I should be ~0, got {y[1]}'
print('ok')
"""
    assert "ok" in _run_isolated(code)


def test_extracellular_battery_mode3_holds_vm():
    """Cable + extracellular: mode 3 holds Vm continuous after finitialize."""
    code = r"""
from neuron import h
import math
h.load_file('stdrun.hoc')
cvode = h.CVode()
s = h.Section(name='s')
s.L = s.diam = 10
s.insert('pas')
s.insert('extracellular')
# modest xg so outer layer is not pure capacitive float
for seg in s:
    seg.xg[0] = 1e-3
    seg.xc[0] = 1.0
ic = h.IClamp(s(0.5))
ic.delay = 0
ic.dur = 1e9
ic.amp = 0.01
h.cvode_active(True)
cvode.use_daspk(1)
cvode.dae_init_dteps(1e-9, 1)
cvode.dae_init_mode(3)
h.finitialize(-65)
# At t=0 from finitialize, Vm should remain near v_init
assert abs(s(0.5).v - (-65)) < 1e-3, s(0.5).v
h.continuerun(0.5)
# Should integrate without falling back hard-fail
assert math.isfinite(s(0.5).v)
print('ok')
"""
    assert "ok" in _run_isolated(code)


def test_inductor_battery_holds_current():
    """Inductor: diagonal C[k][k]=L. Hold current; voltage may jump."""
    code = r"""
from neuron import h
import math
h.load_file('stdrun.hoc')
cvode = h.CVode()
# Node voltage y0, inductor current y1; R to ground; inject Is into node.
# LinearCircuit Inductor stamp (j=ground eliminated):
#   KCL: -I + v/R = Is  =>  G[0][0]=1/R, G[0][1]=-1, b[0]=Is
#   L*I' - v = 0       =>  C[1][1]=L, G[1][0]=-1
L, R, Is = 1.0, 2.0, 1.0
c = h.Matrix(2, 2)
g = h.Matrix(2, 2)
y = h.Vector(2)
y0 = h.Vector(2)
b = h.Vector([Is, 0.0])
c.setval(1, 1, L)
g.setval(0, 0, 1.0 / R)
g.setval(0, 1, -1.0)
g.setval(1, 0, -1.0)
lm = h.LinearMechanism(c, g, y, y0, b)
h.cvode_active(True)
cvode.dae_init_dteps(1e-9, 1)
cvode.dae_init_mode(3)
h.finitialize(0.0)
# Hold I=0 (rest); KCL => v = Is*R
assert abs(y[1]) < 1e-6, f'I_L should be held ~0, got {y[1]}'
assert math.isclose(y[0], Is * R, rel_tol=1e-5, abs_tol=1e-5), y[0]
print('ok')
"""
    assert "ok" in _run_isolated(code)


def test_ida_ic_three_panel_audit_isolated():
    """Smoke: three-panel audit on finitialize and after a later reinit."""
    code = r"""
from neuron import h
import tempfile, os
h.load_file('stdrun.hoc')
cvode = h.CVode()
c = h.Matrix(2, 2)
g = h.Matrix(2, 2)
y = h.Vector(2)
y0 = h.Vector(2)
b = h.Vector([1.0, 0.0])
c.setval(0, 0, 1.0)
c.setval(0, 1, -1.0)
c.setval(1, 0, -1.0)
c.setval(1, 1, 1.0)
g.setval(1, 1, 0.5)
lm = h.LinearMechanism(c, g, y, y0, b)
h.cvode_active(True)
cvode.dae_init_mode(3)
path = tempfile.mktemp(prefix='ida_ic_audit_', suffix='.txt')
cvode.dae_init_audit_file(path)
# finitialize audit
cvode.dae_init_audit(2, 0.0)
h.finitialize(0.0)
# second audit: after short run, force reinit at t via second finitialize-like path
# Arm for t>=0.1 then advance with at_time-style stop via continuerun + re-init
cvode.dae_init_audit(2, 0.0)  # next reinit (finitialize again)
h.finitialize(0.0)
cvode.dae_init_audit(0)
cvode.dae_init_audit_file('')
text = open(path).read()
os.remove(path)
assert 'IDA IC three-panel audit' in text, text
assert 'B post-event pre-IC' in text, text
assert 'C post-IC' in text, text
assert 'summary' in text, text
assert 'C*y' in text or "C*y'=f" in text or 'diagonal' in text or 'content' in text or 'mode 3' in text, text
print('ok')
"""
    assert "ok" in _run_isolated(code)


def test_forcing_tplus_play_ramp_at_reinit():
    """A1/A2: forcing t+ in audit; free y' from u' on series C–R ramp.

    After jump at t=1 onto slope I'=0.5 (I=0.5, R=C=1):
      V2' = R*I' = 0.5,  V1' = V2' + I/C = 1.0
    """
    code = r"""
from neuron import h
import tempfile, os
h.load_file('stdrun.hoc')
cvode = h.CVode()
# Series floating C + R to ground; I into node 0 via b[0]
c = h.Matrix(2, 2)
g = h.Matrix(2, 2)
y = h.Vector(2)
y0 = h.Vector(2)
b = h.Vector([0.0, 0.0])
c.setval(0, 0, 1.0)
c.setval(0, 1, -1.0)
c.setval(1, 0, -1.0)
c.setval(1, 1, 1.0)
g.setval(1, 1, 1.0)
lm = h.LinearMechanism(c, g, y, y0, b)
tvec = h.Vector([0, 1, 1, 2, 2, 5])
ivec = h.Vector([0, 0, 0.5, 1.0, 0, 0])
ivec.play(b._ref_x[0], tvec, True)
h.cvode_active(True)
cvode.use_daspk(1)
cvode.dae_init_mode(3)
h.finitialize(0.0)
h.continuerun(1.0)
path = tempfile.mktemp(prefix='ida_forcing_tplus_', suffix='.txt')
cvode.dae_init_audit_file(path)
cvode.dae_init_audit(2, 0.0)
cvode.re_init()
text = open(path).read()
os.remove(path)
cvode.dae_init_audit(0)
cvode.dae_init_audit_file('')
assert 'forcing t+ info' in text, text
found_u = False
for line in text.splitlines():
    parts = line.split()
    if len(parts) < 4:
        continue
    try:
        u = float(parts[2])
        up = float(parts[3])
    except ValueError:
        continue
    if abs(u - 0.5) < 1e-6 and abs(up - 0.5) < 1e-6:
        found_u = True
        break
assert found_u, 'expected u≈0.5 and u′≈0.5 in forcing t+ dump:\n' + text
# A2: panel C y' should be V1'≈1, V2'≈0.5 (eq order = LM y order)
# Parse "C post-IC" table rows: eq y y' residual
in_c = False
yp = {}
for line in text.splitlines():
    if 'C post-IC' in line:
        in_c = True
        continue
    if in_c and line.startswith('---'):
        break
    if not in_c:
        continue
    parts = line.split()
    if len(parts) >= 4 and parts[0].isdigit():
        eq = int(parts[0])
        yp[eq] = float(parts[2])
assert 0 in yp and 1 in yp, text
assert abs(yp[0] - 1.0) < 1e-4, (yp, text)
assert abs(yp[1] - 0.5) < 1e-4, (yp, text)
print('ok')
"""
    assert "ok" in _run_isolated(code)


def test_seclamp_tiny_cm_mode3_no_init_failure():
    """SEClamp + tiny cm: mode 3 C*y'=f clears residual (scm2eem-style).

    Legacy nano-step leaves O(dteps*|y'|) residual and fails WRMS under
    default style 0. Mode 3 should report err=0 and WRMS C ~ 0.
    """
    code = r"""
from neuron import h
import math
import tempfile, os
h.load_file('stdrun.hoc')
cvode = h.CVode()
s = h.Section(name='eem')
s.L = 10
s.diam = 100 / s.L / h.PI
s.insert('hh')
s.cm = 0.001
vc = h.SEClamp(s(0.5))
vc.rs = 0.1
vc.dur1 = 1e9
vc.amp1 = -65.0
h.cvode_active(True)
cvode.use_daspk(1)
cvode.atol(1e-6)
cvode.dae_init_mode(3)
path = tempfile.mktemp(prefix='ida_seclamp_', suffix='.txt')
cvode.dae_init_audit_file(path)
cvode.dae_init_audit(2, 0.0)
h.finitialize(-65)
cvode.dae_init_audit(0)
cvode.dae_init_audit_file('')
text = open(path).read()
os.remove(path)
assert 'requested_mode=3' in text, text
assert 'path_mode=3' in text, text
assert 'fallback=0' in text, text
assert 'err=0' in text, text
# summary WRMS C is the third number — expect exact 0 after C*y'=f
assert 'WRMS     A/B/C' in text, text
# last token on that line should be 0 (or ~0)
for line in text.splitlines():
    if 'WRMS     A/B/C' in line:
        parts = line.split('=')[-1].split('/')
        wrms_c = float(parts[-1].strip())
        assert wrms_c < 1e-6, (wrms_c, line)
        break
assert math.isclose(s(0.5).v, -65.0, abs_tol=1e-6)
vc.amp1 = -60.0
cvode.re_init()
h.continuerun(0.1)
assert math.isfinite(s(0.5).v)
print('ok')
"""
    assert "ok" in _run_isolated(code)


def test_mode3_forcing_tplus_suite_a3():
    """A3: istep / kink / end extrapolate / finitialize / multi-event (series C–R).

    Algebraic oracles (R=C=1): V2' = I', V1' = I' + I  (from V2'=R*I', V1'=V2'+I/C).
    Distills external nrntest/nrniv/ida iramp/istep-style cases into CI tests.
    """
    code = r"""
from neuron import h
import tempfile, os

def parse_forcing_and_yp(text):
    u = up = None
    yp = {}
    in_c = False
    wrms_c = None
    in_forcing = False
    for line in text.splitlines():
        if 'forcing t+ info' in line:
            in_forcing = True
            continue
        if in_forcing and line.startswith('---'):
            in_forcing = False
        if in_forcing:
            parts = line.split()
            if len(parts) >= 4 and parts[0].isdigit():
                try:
                    int(parts[1])
                    u, up = float(parts[2]), float(parts[3])
                except ValueError:
                    pass
        if 'WRMS' in line and 'A/B/C' in line:
            wrms_c = float(line.split('=')[-1].split('/')[-1].strip())
        if 'C post-IC' in line:
            in_c = True
            continue
        if in_c and line.startswith('---'):
            in_c = False
            continue
        if in_c:
            parts = line.split()
            if len(parts) >= 4 and parts[0].isdigit():
                yp[int(parts[0])] = float(parts[2])
    return u, up, yp, wrms_c

def check_case(name, u, up, yp, wrms_c, eu, eup, eyp0, eyp1, tol=1e-4):
    assert wrms_c is not None and abs(wrms_c) < 1e-9, (name, 'wrms', wrms_c)
    assert u is not None and abs(u - eu) < tol, (name, 'u', u, eu)
    assert up is not None and abs(up - eup) < tol, (name, 'up', up, eup)
    assert abs(yp.get(0, 1e9) - eyp0) < tol, (name, 'yp0', yp, eyp0)
    assert abs(yp.get(1, 1e9) - eyp1) < tol, (name, 'yp1', yp, eyp1)

h.load_file('stdrun.hoc')
cvode = h.CVode()
# Single series C–R LM for the whole suite (avoid stacking mechanisms)
c = h.Matrix(2, 2)
g = h.Matrix(2, 2)
y = h.Vector(2)
y0 = h.Vector(2)
b = h.Vector([0.0, 0.0])
c.setval(0, 0, 1.0)
c.setval(0, 1, -1.0)
c.setval(1, 0, -1.0)
c.setval(1, 1, 1.0)
g.setval(1, 1, 1.0)
lm = h.LinearMechanism(c, g, y, y0, b)
h.cvode_active(True)
cvode.use_daspk(1)
cvode.dae_init_mode(3)

play_iv = None
play_tv = None

def set_play(tvec, ivec):
    global play_iv, play_tv
    if play_iv is not None:
        play_iv.play_remove()
    play_tv = h.Vector(tvec)
    play_iv = h.Vector(ivec)
    play_iv.play(b._ref_x[0], play_tv, True)

def audit_reinit():
    path = tempfile.mktemp(prefix='ida_a3_', suffix='.txt')
    cvode.dae_init_audit_file(path)
    cvode.dae_init_audit(2, 0.0)
    cvode.re_init()
    text = open(path).read()
    os.remove(path)
    cvode.dae_init_audit(0)
    cvode.dae_init_audit_file('')
    return text

# --- T_istep: jump to I=0.5 then flat (I'=0) ---
set_play([0, 1, 1, 5], [0, 0, 0.5, 0.5])
h.finitialize(0.0)
h.continuerun(1.0)
u, up, yp, wrms = parse_forcing_and_yp(audit_reinit())
check_case('istep', u, up, yp, wrms, 0.5, 0.0, 0.5, 0.0)

# --- T_kink: continuous I, slope becomes 1 at t=1 (I=0) ---
set_play([0, 1, 2], [0, 0, 1])
h.finitialize(0.0)
h.continuerun(1.0)
u, up, yp, wrms = parse_forcing_and_yp(audit_reinit())
check_case('kink', u, up, yp, wrms, 0.0, 1.0, 1.0, 1.0)

# --- T_end_extrap: past last knot, last segment slope 2 ---
set_play([0, 1, 2], [0, 1, 3])
h.finitialize(0.0)
h.continuerun(2.5)
u, up, yp, wrms = parse_forcing_and_yp(audit_reinit())
# I(2.5)=4, I'=2 → V2'=2, V1'=6
check_case('end_extrap', u, up, yp, wrms, 4.0, 2.0, 6.0, 2.0, tol=1e-3)

# --- T_flat_end: last two y equal → I'=0 after end ---
set_play([0, 1, 2], [0, 1, 1])
h.finitialize(0.0)
h.continuerun(3.0)
u, up, yp, wrms = parse_forcing_and_yp(audit_reinit())
check_case('flat_end', u, up, yp, wrms, 1.0, 0.0, 1.0, 0.0)

# --- T_finitialize: first segment slope 0.5 ---
set_play([0, 2], [0, 1])
path = tempfile.mktemp(prefix='ida_a3_fini_', suffix='.txt')
cvode.dae_init_audit_file(path)
cvode.dae_init_audit(2, 0.0)
h.finitialize(0.0)
text = open(path).read()
os.remove(path)
cvode.dae_init_audit(0)
cvode.dae_init_audit_file('')
u, up, yp, wrms = parse_forcing_and_yp(text)
check_case('finitialize_slope', u, up, yp, wrms, 0.0, 0.5, 0.5, 0.5)

# --- T_multi_event: iramp-like jump+ramp then off ---
set_play([0, 1, 1, 2, 2, 5], [0, 0, 0.5, 1.0, 0, 0])
h.finitialize(0.0)
h.continuerun(1.0)
u, up, yp, wrms = parse_forcing_and_yp(audit_reinit())
check_case('multi_t1_ramp', u, up, yp, wrms, 0.5, 0.5, 1.0, 0.5)
h.continuerun(2.0)
u, up, yp, wrms = parse_forcing_and_yp(audit_reinit())
check_case('multi_t2_off', u, up, yp, wrms, 0.0, 0.0, 0.0, 0.0)

if play_iv is not None:
    play_iv.play_remove()
print('ok')
"""
    assert "ok" in _run_isolated(code)


def test_mode3_dforce_sinusoid_a4():
    """A4: LinearMechanism.dforce supplies analytic b' for free y' (sinusoid I)."""
    code = r"""
from neuron import h
import math
import tempfile, os
h.load_file('stdrun.hoc')
cvode = h.CVode()
A, w = 1.0, 2.0 * math.pi  # I = A sin(w t), I' = A w cos(w t)
c = h.Matrix(2, 2)
g = h.Matrix(2, 2)
y = h.Vector(2)
y0 = h.Vector(2)
b = h.Vector([0.0, 0.0])
bdot = h.Vector([0.0, 0.0])
c.setval(0, 0, 1.0)
c.setval(0, 1, -1.0)
c.setval(1, 0, -1.0)
c.setval(1, 1, 1.0)
g.setval(1, 1, 1.0)

def force():
    b.x[0] = A * math.sin(w * h.t)

def dforce():
    bdot.x[0] = A * w * math.cos(w * h.t)

lm = h.LinearMechanism(force, c, g, y, y0, b)
lm.dforce(dforce, bdot)
h.cvode_active(True)
cvode.use_daspk(1)
cvode.dae_init_mode(3)
# IC at t=0: I=0, I'=A*w → V2'=A*w, V1'=A*w + I = A*w
path = tempfile.mktemp(prefix='ida_dforce_', suffix='.txt')
cvode.dae_init_audit_file(path)
cvode.dae_init_audit(2, 0.0)
h.finitialize(0.0)
text = open(path).read()
os.remove(path)
assert 'dforce' in text or 'bdot' in text, text
assert 'err=0' in text, text
in_c = False
yp = {}
for line in text.splitlines():
    if 'C post-IC' in line:
        in_c = True
        continue
    if in_c and line.startswith('---'):
        break
    if in_c:
        parts = line.split()
        if len(parts) >= 4 and parts[0].isdigit():
            yp[int(parts[0])] = float(parts[2])
expect = A * w
assert abs(yp.get(1, 1e9) - expect) < 1e-4, (yp, expect, text)
assert abs(yp.get(0, 1e9) - expect) < 1e-4, (yp, expect, text)  # I=0 so V1'=V2'
# mid-ramp time: re_init at t=0.25 without play
h.t = 0.25
force()
dforce()
cvode.dae_init_audit_file(path)
cvode.dae_init_audit(2, 0.0)
cvode.re_init()
text = open(path).read()
os.remove(path)
I = A * math.sin(w * 0.25)
Ip = A * w * math.cos(w * 0.25)
yp = {}
in_c = False
for line in text.splitlines():
    if 'C post-IC' in line:
        in_c = True
        continue
    if in_c and line.startswith('---'):
        break
    if in_c:
        parts = line.split()
        if len(parts) >= 4 and parts[0].isdigit():
            yp[int(parts[0])] = float(parts[2])
assert abs(yp.get(1, 1e9) - Ip) < 1e-3, (yp, Ip, text)
assert abs(yp.get(0, 1e9) - (Ip + I)) < 1e-3, (yp, Ip, I, text)
print('ok')
"""
    assert "ok" in _run_isolated(code)


def test_mode3_dforce_fd_fallback_a4():
    """A4: without dforce, FD of f_callable estimates b' for free y'."""
    code = r"""
from neuron import h
import math
import tempfile, os
h.load_file('stdrun.hoc')
cvode = h.CVode()
A, w = 1.0, 2.0  # slower so FD with h=1e-8 is accurate
c = h.Matrix(2, 2)
g = h.Matrix(2, 2)
y = h.Vector(2)
y0 = h.Vector(2)
b = h.Vector([0.0, 0.0])
c.setval(0, 0, 1.0)
c.setval(0, 1, -1.0)
c.setval(1, 0, -1.0)
c.setval(1, 1, 1.0)
g.setval(1, 1, 1.0)

def force():
    b.x[0] = A * math.sin(w * h.t)

lm = h.LinearMechanism(force, c, g, y, y0, b)
# no lm.dforce — FD fallback
h.cvode_active(True)
cvode.use_daspk(1)
cvode.dae_init_mode(3)
path = tempfile.mktemp(prefix='ida_fd_', suffix='.txt')
cvode.dae_init_audit_file(path)
cvode.dae_init_audit(2, 0.0)
h.finitialize(0.0)
text = open(path).read()
os.remove(path)
assert 'bdot_fd' in text or 'err=0' in text, text
yp = {}
in_c = False
for line in text.splitlines():
    if 'C post-IC' in line:
        in_c = True
        continue
    if in_c and line.startswith('---'):
        break
    if in_c:
        parts = line.split()
        if len(parts) >= 4 and parts[0].isdigit():
            yp[int(parts[0])] = float(parts[2])
expect = A * w  # cos(0)=1
assert abs(yp.get(1, 1e9) - expect) < 1e-2, (yp, expect, text)
assert abs(yp.get(0, 1e9) - expect) < 1e-2, (yp, expect, text)
print('ok')
"""
    assert "ok" in _run_isolated(code)


if __name__ == "__main__":
    test_dae_init_mode_api()
    test_dae_init_mode_env_default()
    test_dae_init_audit_api()
    test_dae_init_stats_api()
    test_pure_resistive_all_modes_isolated()
    test_series_cr_mode0_mode1_isolated()
    test_series_cr_battery_mode3_isolated()
    test_opamp_tau_battery_holds_output_voltage()
    test_inductor_battery_holds_current()
    test_extracellular_battery_mode3_holds_vm()
    test_ida_ic_three_panel_audit_isolated()
    test_forcing_tplus_play_ramp_at_reinit()
    test_seclamp_tiny_cm_mode3_no_init_failure()
    test_mode3_forcing_tplus_suite_a3()
    test_mode3_dforce_sinusoid_a4()
    test_mode3_dforce_fd_fallback_a4()
    print("ok")
