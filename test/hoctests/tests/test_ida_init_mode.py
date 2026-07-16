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


if __name__ == "__main__":
    test_dae_init_mode_api()
    test_pure_resistive_all_modes_isolated()
    test_series_cr_mode0_mode1_isolated()
    print("ok")
