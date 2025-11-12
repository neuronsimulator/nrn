Fast and Persistent Sodium Currents in NEURON
==============================================

Q: How can I implement and simulate fast sodium (Na) and persistent sodium (NaP) channels separately in NEURON using NMODL? Also, how can I access and plot their individual currents? Is there support for temperature dependence using Q10?

A: It is recommended to model fast Na and persistent NaP currents as separate mechanisms in different `.mod` files, each specifying its own `SUFFIX` and writing to the same ionic current variable (e.g., `ina` for sodium current). This approach allows you to manage currents independently, plot individual currents and conductances, and maintain clean and modular code.

Example implementations:

Fast Na channel (`nat.mod`):

.. code-block::

    NEURON {
      SUFFIX nat
      USEION na READ ena WRITE ina
      RANGE gbar, i, g
    }
    UNITS {
      (mA) = (milliamp)
      (mV) = (millivolt)
    }
    PARAMETER {
      gbar = 0.12 (mho/cm2)
    }
    STATE {
      m h
    }
    ASSIGNED {
      v (mV)
      ena (mV)
      i (mA/cm2)
      g (mho/cm2)
      minf hinf
    }
    BREAKPOINT {
      SOLVE states METHOD cnexp
      g = gbar * m*m * h
      i = g * (v - ena)
      ina = i
    }
    INITIAL {
      rates(v)
      m = minf
      h = hinf
    }
    DERIVATIVE states {
      rates(v)
      m' = malpha * (1 - m) - mbeta * m
      h' = halpha * (1 - h) - hbeta * h
    }
    PROCEDURE rates(v(mV)) {
      TABLE minf, hinf, malpha, mbeta, halpha, hbeta FROM -100 TO 100 WITH 200
      : define rate functions and steady states here
    }

Persistent Na channel (`nap.mod`):

.. code-block:: 

    NEURON {
      SUFFIX nap
      USEION na READ ena WRITE ina
      RANGE gbar, i, g
    }
    UNITS {
      (mA) = (milliamp)
      (mV) = (millivolt)
    }
    PARAMETER {
      gbar = 0.01 (mho/cm2)
    }
    STATE {
      m
    }
    ASSIGNED {
      v (mV)
      ena (mV)
      i (mA/cm2)
      g (mho/cm2)
      minf
    }
    BREAKPOINT {
      SOLVE states METHOD cnexp
      g = gbar * m*m*m
      i = g * (v - ena)
      ina = i
    }
    INITIAL {
      rates(v)
      m = minf
    }
    DERIVATIVE states {
      rates(v)
      m' = malpha * (1 - m) - mbeta * m
    }
    PROCEDURE rates(v(mV)) {
      TABLE minf, malpha, mbeta FROM -100 TO 100 WITH 200
      : define rate functions and steady states here
    }

Accessing and plotting individual currents in hoc or Python:

- In hoc, the total sodium current (`ina`) at a location is accessible as `section.ina(x)`. However, because multiple mechanisms write to `ina`, the individual current from each mechanism is not directly accessible unless:

  - You add `RANGE i` in the `NEURON` block.
  - You introduce a separate assigned variable (e.g., `i`) in the `ASSIGNED` block.
  - Assign the computed current to this variable inside `BREAKPOINT`.

  Then you can access individual currents by their mechanism suffix in hoc or Python.

Example to enable individual current access (within each mod file):

.. code-block:: hoc

    NEURON {
      SUFFIX nat
      USEION na READ ena WRITE ina
      RANGE gbar, i, g
    }

    ASSIGNED {
      ...
      i (mA/cm2)
    }

    BREAKPOINT {
      ...
      i = gbar * m*m * h * (v - ena)
      ina = i
    }

Plotting in Python assuming insertion of mechanisms with suffixes `nat` and `nap`:

.. code-block:: python

    from neuron import h
    soma = h.Section(name='soma')
    soma.insert('nat')
    soma.insert('nap')
    h.finitialize(-65)

    # Access individual currents at location 0.5
    i_nat = soma.i_nat(0.5)
    i_nap = soma.i_nap(0.5)
    total_ina = soma.ina(0.5)

    print(f'Fast Na current: {i_nat}')
    print(f'Persistent Na current: {i_nap}')
    print(f'Total Na current: {total_ina}')

Temperature dependence (Q10):

NEURON mod files can be written to include Q10 temperature dependence by scaling rate constants. This is typically done by multiplying rate functions by `q10^((celsius - temp_ref)/10)`, where `celsius` is the simulation temperature and `temp_ref` is the reference temperature (usually 23 or 37°C).

Example scaling expression in NMODL:

.. code-block:: hoc

    PROCEDURE rates(v (mV)) {
      LOCAL qt
      qt = q10 ^ ((celsius - 23)/10)
      malpha = malpha0 * qt
      mbeta = mbeta0 * qt
      ...
      minf = malpha / (malpha + mbeta)
    }

Summary:

- Use separate mod files for different channel types sharing the same ion current to modularize code.
- To access individual ionic currents, include a `RANGE` variable for the current and assign it separately.
- Implement temperature dependence in rate definitions using Q10 scaling.

This approach facilitates simulation, analysis, and visualization of distinct ionic currents in NEURON.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=346
