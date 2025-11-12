Programmatically Changing Temperature During a NEURON Simulation
=================================================================

Can I change the simulation temperature (`celsius`) as a function of time during a NEURON simulation?

Yes. Although `celsius` is a special variable that cannot be directly assigned within NMODL code, you can programmatically modify `celsius` during a simulation from Hoc or Python. This is typically done by using NEURON's `Vector.play()` method with time and temperature vectors, or by scheduling events to change `celsius` abruptly.

Example in Hoc: Changing `celsius` with a linear ramp over time

.. code-block:: hoc

    objref tempvec, tvec
    tempvec = new Vector(3)
    tvec = new Vector(3)
    tempvec.x[0] = 6.3    // degC
    tvec.x[0] = 0
    tempvec.x[1] = 20     // degC
    tvec.x[1] = 100      // ms
    tempvec.x[2] = 20     // degC
    tvec.x[2] = 1e9

    // The third argument=1 enables interpolation
    tempvec.play(&celsius, tvec, 1)

Notes:
- Mechanisms with temperature-dependent rates must be written to depend on `celsius` correctly (e.g., using `DEPEND celsius` in TABLE statements) to respond to changes.
- Continuous changes in `celsius` cause recomputation of rate tables, which may slow the simulation.
  
Example in Python: Using `play` to change `celsius` over time

.. code-block:: python

    from neuron import h

    tempvec = h.Vector([6.3, 20, 20])
    tvec = h.Vector([0, 100, 1e9])
    tempvec.play(h._ref_celsius, tvec, 1)  # Interpolated change of temperature over time

Event-driven abrupt temperature changes or parameter changes can be done by scheduling events with `cvode.event()` combined with an `FInitializeHandler` to start the process. Below is an example controlling an IClamp current with events (the method is analogous for changing `celsius` or other parameters):

.. code-block:: hoc

    objref stim
    stim = new IClamp(0.5)

    const DUR = 0.1    // ms, pulse duration
    const AMP = 0.1    // nA
    const START = 5    // ms, time of first pulse
    const INTERVAL = 25 // ms between pulse starts

    objref fih
    fih = new FInitializeHandler("initi()")

    volatile int STIMON = 0

    proc initi() {
      STIMON = 0
      stim.del = 0
      stim.dur = 1e9
      cvode.event(START, "seti()")
    }

    proc seti() {
      if (STIMON == 0) {
        STIMON = 1
        stim.amp = AMP
        cvode.event(t + DUR, "seti()")
        cvode.event(t + INTERVAL, "seti()")
      } else {
        STIMON = 0
        stim.amp = 0
      }
      if (cvode.active()) {
        cvode.re_init()
      } else {
        fcurrent()
      }
    }

This pattern can be adapted to change `celsius` or any other parameter abruptly at desired times.

Summary:
- Do *not* assign `celsius` inside NMODL `BREAKPOINT` blocks; instead drive `celsius` externally from Hoc or Python.
- Use `Vector.play()` for continuous or interpolated temperature changes.
- Use `cvode.event()` and `FInitializeHandler` to schedule abrupt temperature or parameter changes.
- Re-initialize CVode after parameter changes when necessary with `cvode.re_init()` to ensure proper solver state.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=162
