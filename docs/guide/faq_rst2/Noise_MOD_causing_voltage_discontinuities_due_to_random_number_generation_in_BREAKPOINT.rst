Noise MOD causing voltage discontinuities due to random number generation in BREAKPOINT
==========================================================================================

**Question:**  
Why does calling a random number generator (RNG) directly in the BREAKPOINT block of a mod file cause strange voltage discontinuities, and how can this be fixed?

**Answer:**  
When a random number generator is called directly in the BREAKPOINT block, NEURON executes this block twice per time step to estimate the slope conductance (di/dv). Because the random value changes between these two calls, the conductance estimate becomes incorrect, leading to discontinuities or unexpected voltage behavior.

**Fix:**  
Move the RNG call out of the BREAKPOINT block into a separate PROCEDURE, then call this procedure using `SOLVE` inside BREAKPOINT. This ensures the RNG call happens only once per time step, preventing inconsistent conductance estimation.

**Example MOD file snippet before fix:**

.. code-block:: hoc

    BREAKPOINT {
      i = i - (dt * i) / tau + amp * normrand(0, 1)
    }

**Modified MOD file snippet after fix:**

.. code-block:: hoc

    BREAKPOINT {
      SOLVE noise
    }
    
    PROCEDURE noise() {
      i = i - (dt * i) / tau + amp * normrand(0, 1)
    }

**Note:**  
Because this fix changes the statistical properties of the generated current, you may need to adjust amplitudes (e.g., reduce `amp` by a factor of ~50) and repeat simulations for consistent results.

**Python usage example:**

.. code-block:: python

    from neuron import h
    h.load_file('stdrun.hoc')
    
    stim = h.Inoise2(0.5)  # assuming Inoise2 is compiled and loaded
    stim.tau = 30
    stim.amp = 0.002  # reduced amplitude after fix
    
    h.finitialize(-65)
    h.continuerun(100)

**Hoc usage example:**

.. code-block:: hoc

    objref stim
    stim = new Inoise2(0.5)
    stim.tau = 30
    stim.amp = 0.002  // reduced amplitude after fix
    
    initialize()
    run(100)

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=3455
