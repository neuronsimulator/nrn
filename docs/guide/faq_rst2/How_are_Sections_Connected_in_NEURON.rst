How are Sections Connected in NEURON?
====================================

Sections in NEURON are connected end-to-end via axial resistances, similar to the way segments within a section are connected. Each section can be considered as a cable consisting of segments, and connections between sections involve the axial resistance between their ends.

Key Points:
- The total membrane capacitance of a cell is the integral of capacitance over its entire surface and is computed by summing the capacitance contributions of all segments and sections. This is *not* the same as assuming all capacitances are in parallel.
- Sections and segments are connected through axial resistances, so electrical elements (membrane resistances and capacitances) are not connected simply in parallel or series.
- Input resistance (“Rin”) cannot be computed by summing or combining input resistances of separate sections using parallel or series resistor formulas unless the cell is electrotonically compact.
- The membrane time constant \(\tau_m\) is given by the product of the specific membrane resistance (\(R_m\), in \(\Omega \cdot cm^2\)) and the specific membrane capacitance (\(C_m\), in \(\mu F/cm^{2}\)): 
  \[
  \tau_m = R_m \times C_m
  \]
  This time constant is independent of surface area because increasing surface area increases capacitance and decreases resistance proportionally.
- For complex morphologies, input resistance and time constant must be measured or estimated via simulations rather than simple analytic formulas.

Example of connecting sections in Python:

.. code-block:: python

    from neuron import h
    
    # Create two sections
    soma = h.Section(name='soma')
    dend = h.Section(name='dend')
    
    # Connect dend(0) to soma(1)
    dend.connect(soma(1))
    
    # Set biophysical properties if needed
    for sec in (soma, dend):
        sec.L = 100    # length in microns
        sec.diam = 10  # diameter in microns
        sec.nseg = 1   # number of segments per section

    # Insert passive mechanism
    for sec in (soma, dend):
        sec.insert('pas')
        sec.g_pas = 0.001  # S/cm2
        sec.e_pas = -65    # mV

Example of connecting sections in Hoc:

.. code-block:: hoc

    create soma, dend
    dend connect soma(1)
    forall {
        L = 100
        diam = 10
        nseg = 1
        insert pas
        g_pas = 0.001
        e_pas = -65
    }

Calculating input resistance via simulation:

- Rather than attempting to calculate total Rin by formula, inject a small current at the location of interest, record the steady-state voltage change, and apply Ohm’s law:  
  \[
  R_{in} = \frac{V_{steady\ state}}{I_{injected}}
  \]

Calculating membrane time constant in a compartment:

- For an electrotonically compact compartment:
  
  .. code-block:: python
  
      tau_m = sec.cm * sec.g_pas ** -1  # assuming g_pas = 1/Rm
  
- Remember that the time constant is specific to membrane properties, not surface area.

Summary:
- Sections are connected through axial resistances at their endpoints, forming cables.
- Total membrane capacitance sums over all segments, but input resistance and time constants require simulation-based measurement or careful interpretation.
- Time constant depends on specific membrane resistance and capacitance and is independent of cell area.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=4411
