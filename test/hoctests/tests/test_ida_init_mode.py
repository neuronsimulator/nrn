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
    assert cvode.dae_init_mode() == 0
    assert cvode.dae_init_mode(1) == 1
    assert cvode.dae_init_mode(2) == 2
    assert cvode.dae_init_mode(3) == 3
    assert cvode.dae_init_mode(0) == 0


def _run_isolated(code: str):
    env = os.environ.copy()
    r = subprocess.run(
        [sys.executable, "-c", code],
        env=env,
        capture_output=True,
        text=True,
    )
    if r.returncode != 0:
        raise AssertionError(f"stdout:\n{r.stdout}\nstderr:\n{r.stderr}")
    return r.stdout


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
for mode in (0, 1, 2):
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


if __name__ == "__main__":
    test_dae_init_mode_api()
    test_pure_resistive_all_modes_isolated()
    test_series_cr_mode0_mode1_isolated()
    test_series_cr_battery_mode3_isolated()
    test_opamp_tau_battery_holds_output_voltage()
    test_inductor_battery_holds_current()
    print("ok")
