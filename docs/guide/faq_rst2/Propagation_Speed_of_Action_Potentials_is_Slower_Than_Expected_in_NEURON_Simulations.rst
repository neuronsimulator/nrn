Propagation Speed of Action Potentials (AP) is Slower Than Expected in NEURON Simulations
==============================================================================================

**Question:**  
Why might the action potential (AP) propagation speed in my NEURON simulation be much slower than expected, and how can I fix it?

**Answer:**  
Several factors can influence AP propagation speed in NEURON. Common causes of unexpectedly slow propagation include:

- Differences in morphology, especially the diameter of neurites.
- How conduction velocity is measured and ensuring measurements are accurate.
- Stimulus parameters: use a brief stimulus (~0.1 ms) with amplitude about twice the threshold.
- Correct specification of ion channel densities.
- Spatial discretization: using too few segments (`nseg`) can decouple compartments and slow propagation.
- Operating temperature: NEURON’s default temperature is 6.3 °C, which can slow channel kinetics and propagation speed. Setting the temperature to physiological values (e.g., 20-37 °C) is important.

In one reported case, the main cause was the temperature set too low. Changing the temperature to 20 °C corrected the AP conduction velocity.

**Setting the temperature in Python:**  
Make sure to set the temperature before running your simulation:

.. code-block:: python

    from neuron import h
    h.celsius = 20  # Set temperature to 20 °C

**Equivalent setting in HOC:**

.. code-block:: hoc

    // Set temperature to 20 degrees Celsius
    celsius = 20

**Example snippet for adjusting morphology and channel properties in Python:**

.. code-block:: python

    for i, sec in enumerate(cell.allseclist):
        sec.Ra = 75  # Ohm*cm
        if i == 0:
            sec.cm = 1.5  # uF/cm2
        else:
            sec.cm = 3
        sec.insert('hh')
        sec.gnabar_hh = 0.083  # S/cm2
        sec.gkbar_hh = 0.03
        sec.gl_hh = 0.0003
        sec.ena = 55
        sec.ek = -72
        sec.el_hh = -49.3

**Additional tips:**

- Use high enough `nseg` to ensure spatial accuracy (e.g., determined by the d_lambda rule or experiment with increasing `nseg`).
- Initiate APs with short, supra-threshold stimuli (~0.1 ms duration).
- Verify your measurement method for conduction velocity (e.g., measure time difference for spike reaching two points separated by a known distance, using the same voltage threshold crossing time).

By carefully checking and setting these parameters, you can achieve physiologically plausible AP propagation speeds in NEURON.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=3482
