"""Callback form of LinearMechanism on the pendulum ODE.

theta' = omega and omega' = -sin(theta), with the nonlinear term supplied
by the callback. A moderate swing is compared with an independent
integrator. omega = 2 is the separatrix: just below it the pendulum
librates, and just above it the pendulum rotates.
"""

import math

from neuron import h


def _energy(theta, omega):
    return 0.5 * omega * omega - math.cos(theta)


def _rk4_pendulum(theta0, omega0, t_end, dt=1e-4):
    """Independent reference for the same ODE. dt is the reference step."""
    theta, omega = theta0, omega0
    nstep = max(1, int(round(t_end / dt)))
    dt = t_end / nstep

    def deriv(theta, omega):
        return omega, -math.sin(theta)

    for _ in range(nstep):
        k1 = deriv(theta, omega)
        k2 = deriv(theta + 0.5 * dt * k1[0], omega + 0.5 * dt * k1[1])
        k3 = deriv(theta + 0.5 * dt * k2[0], omega + 0.5 * dt * k2[1])
        k4 = deriv(theta + dt * k3[0], omega + dt * k3[1])
        theta += dt * (k1[0] + 2 * k2[0] + 2 * k3[0] + k4[0]) / 6
        omega += dt * (k1[1] + 2 * k2[1] + 2 * k3[1] + k4[1]) / 6
    return theta, omega


def test_linmech_pendulum_callback():
    h.load_file("stdrun.hoc")
    saved_atol = h.cvode.atol()
    saved_active = h.cvode_active()

    cmat = h.Matrix(2, 2, 2).ident()
    gmat = h.Matrix(2, 2, 2)
    gmat.setval(0, 1, -1)
    y = h.Vector(2)
    y0 = h.Vector(2)
    b = h.Vector(2)

    def callback():
        b.x[1] = -math.sin(y.x[0])

    lm = h.LinearMechanism(callback, cmat, gmat, y, y0, b)
    sec = h.Section(name="linmech_pendulum")
    theta = h.Vector()
    omega = h.Vector()
    theta.record(y._ref_x[0])
    omega.record(y._ref_x[1])

    def run(omega0):
        y0.x[0] = 0.0
        y0.x[1] = omega0
        h.cvode.atol(1e-5)
        h.cvode_active(1)
        h.finitialize()
        assert abs(y.x[0]) < 1e-5
        assert abs(y.x[1] - omega0) < 1e-5
        samples = {}
        for t in (1.0, 5.0, 10.0):
            h.continuerun(t)
            samples[t] = (y.x[0], y.x[1])
        # The accepted state must be the one the callback just used.
        assert abs(b.x[1] + math.sin(y.x[0])) < 1e-6
        recorded_theta = theta.to_python()
        recorded_omega = omega.to_python()
        assert len(recorded_theta) > 2
        e0 = _energy(0.0, omega0)
        worst = 0.0
        for th, w in zip(recorded_theta, recorded_omega):
            worst = max(worst, abs(_energy(th, w) - e0))
        assert worst < 1e-3
        return recorded_theta, samples

    try:
        # Away from the separatrix the state at fixed times is stable.
        _, samples = run(1.0)
        for t in (1.0, 5.0, 10.0):
            ref_theta, ref_omega = _rk4_pendulum(0.0, 1.0, t)
            got_theta, got_omega = samples[t]
            assert abs(got_theta - ref_theta) < 1e-3
            assert abs(got_omega - ref_omega) < 1e-3

        # Near omega = 2 a small phase error is amplified, so check the
        # topology rather than the state at late times.
        librating, _ = run(1.9999)
        peak = max(abs(value) for value in librating)
        assert 3.0 < peak < math.pi

        rotating, _ = run(2.0001)
        assert max(abs(value) for value in rotating) > math.pi
    finally:
        del lm
        h.cvode.atol(saved_atol)
        h.cvode_active(saved_active)
        h.delete_section(sec=sec)
        # The DAE turns the sparse solver on, and ~NrnDAE leaves it on.
        # use_mxb(0) has to follow both, or recalc_diam turns it back on.
        h.cvode.use_mxb(0)
