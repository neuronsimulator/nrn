Continue Simulation Until a Preset Number of Spikes is Reached
=================================================================

**Q:** How can I stop a NEURON simulation automatically once a specific number of spikes have occurred, without checking spike counts at every timestep?

**A:**  
You can use NEURON’s event delivery system via the `NetCon` class to record spike times and trigger simulation stop once the desired number of spikes is reached. Instead of polling for spikes continuously, define a callback procedure that counts spikes and schedules a simulation stop event with a short delay to allow synaptic effects. Use `FInitializeHandler` to reset spike recording before each run.

Example usage in Hoc:

.. code-block:: hoc

    objref nc, stv
    stv = new Vector()
    // Replace 'source' with your spike source (e.g., a section or artificial cell)
    nc = new NetCon(source, nil)

    // Parameters
    define MAXNUM 3
    define WAIT 8  // ms delay to allow spike delivery

    proc wrapup() {
        printf("Spike times:\n")
        stv.printf()
        stoprun = 1
    }

    proc counter() {
        stv.append(t)
        if (stv.size() == MAXNUM) {
            cvode.event(t + WAIT, "wrapup()")
        }
    }

    // Record spikes by calling counter() at each spike event
    nc.record("counter()")

    proc reset_stv() {
        stv = new Vector()
    }

    objref fih
    fih = new FInitializeHandler("reset_stv()")


Equivalent Python example:

.. code-block:: python

    from neuron import h

    # Parameters
    MAXNUM = 3
    WAIT = 8  # ms

    stv = h.Vector()
    nc = h.NetCon(source, None)  # Replace 'source' with your spike source

    def wrapup():
        print("Spike times:")
        stv.printf()
        h.stoprun = 1

    def counter():
        stv.append(h.t)
        if stv.size() == MAXNUM:
            h.cvode.event(h.t + WAIT, wrapup)

    # Record spikes by calling counter() at each spike event
    nc.record(counter)

    def reset_stv():
        global stv
        stv = h.Vector()

    fih = h.FInitializeHandler(1, reset_stv)

This approach efficiently stops the simulation after the desired number of spikes without continuous spike count polling. The slight delay (`WAIT`) ensures that spike events are fully delivered and processed.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=138
