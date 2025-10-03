Current Clamp Electrode Modeling with Electrode Resistance and Capacitance in NEURON
=========================================================================================

**Q: How can I model a realistic current clamp electrode in NEURON that includes electrode resistance and capacitance to reproduce bridge balance artifacts?**

NEURON allows realistic modeling of current clamp electrodes incorporating series resistance and capacitance using either the built-in `SEClamp` point process or the Linear Circuit Builder for more complex equivalent circuit models.

- The **simplest electrode model** is an equivalent *L circuit*: a single series resistor representing electrode resistance, and a capacitor from the headstage end of that resistor to ground representing electrode capacitance.
- A more realistic model is an equivalent *T circuit*: the series resistance is split into two resistors, with the lumped electrode capacitance connected to the node between them. This arrangement better mimics abrupt voltage transients seen at the start and end of current pulses.
- Electrode capacitance compensation cannot be perfect and is best approximated by placing the lumped capacitor midway in the equivalent T circuit.
- Additional components such as the limited gain-bandwidth of the headstage and compensation amplifiers can be added to accurately model real-world clamp artifacts.
- For simple series resistance modeling with no capacitance, the `SEClamp` mechanism can be used directly.

It is also possible to consider the effect of non-infinite seal resistance (modeled as a linear resistor leak to ground) and to model very complex electrodes by approximating them as multi-compartment cables with adjusted axial resistance and membrane capacitance, but the Linear Circuit Builder is the recommended and most convenient tool.

Example implementations:

.. code-block:: python

    from neuron import h, gui

    # Simple SEClamp model with series resistance (no capacitance)
    stim = h.SEClamp(0.5)
    stim.rs = 10  # series resistance in MΩ
    stim.amp1 = 0.1  # current injection amplitude in nA
    stim.dur1 = 100  # duration in ms
    stim.del1 = 10   # delay before stimulation starts in ms

    # More complex electrode model using Linear Circuit Builder (conceptual)
    # Here you would build an equivalent T circuit with resistance and capacitance
    # components connected appropriately. This typically involves GUI or hoc scripting.

.. code-block:: hoc

    // Simple SEClamp usage example
    objref stim
    stim = new SEClamp(0.5)
    stim.rs = 10   // series resistance in MΩ
    stim.amp1 = 0.1
    stim.dur1 = 100
    stim.del1 = 10

    // For more complex models, use the Linear Circuit Builder GUI or scripts to
    // construct equivalent L or T circuits representing electrode resistance and capacitance.

Summary points:
- Use `SEClamp` for simple series resistance modeling.
- Use Linear Circuit Builder for detailed electrode models including capacitance and clamp circuitry.
- The equivalent T circuit (split series resistance with midpoint capacitor to ground) better approximates real electrode behavior than a simple L circuit.
- Seal resistance can be modeled as an additional leak resistor if needed.
- It is essential to understand the biophysical basis of electrode artifacts to construct realistic models.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=203
