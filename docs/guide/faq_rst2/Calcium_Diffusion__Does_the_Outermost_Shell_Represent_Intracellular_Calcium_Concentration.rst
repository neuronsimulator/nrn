Calcium Diffusion: Does the Outermost Shell Represent Intracellular Calcium Concentration?
=========================================================================================

In multi-shell calcium diffusion models such as the “cdp” example (NEURON Book, Example 9.8, Chapter 9), the calcium concentration in the outermost shell (denoted `cai`) is often used as the calcium signal interacting with membrane mechanisms. However, this value is a local concentration near the membrane interior and is not equivalent to the average intracellular calcium concentration.

- `cai` represents the calcium concentration in the shell adjacent to the membrane where ion channels and pumps "sense" calcium via the `USEION` mechanism.
- The average intracellular calcium concentration depends on the volume-weighted sum of all shell concentrations.
- Calcium imaging experiments typically measure a signal related to the average calcium-bound dye concentration across the cytoplasm, *not* the local concentration in the outermost shell.
- Comparing model output to experiment should consider these differences: to compare with calibrated fluorescence data, compute the volume-weighted average calcium concentration over all shells.

How to compute the average calcium concentration in a compartment with radial shells:

**Hoc example to record and weight shell calcium concentrations:**

.. code-block:: hoc

    // Assume 'numshells' is the number of radial shells
    obfunc mkcaveclist() { local i localobj tmpvec, listvec
      listvec = new List()
      for i=0, numshells-1 {
        tmpvec = new Vector()
        tmpvec.record(&ca[i]_cadifus(0.5))  // record shell Ca at segment 0.5
        listvec.append(tmpvec)
      }
      return listvec
    }

    objref cavecs, tvec
    tvec = new Vector()
    tvec.record(&t)
    cavecs = mkcaveclist()

    // Weights proportional to shell volumes (ro^2 - ri^2)
    // weights is a Vector with length numshells
    objref weights
    weights = new Vector(numshells)
    // Fill weights with appropriate volume fractions

    // Function to compute weighted sum of shell calcium concentrations
    obfunc wtsum() { local i localobj tmpvec, sumvec
      sumvec = new Vector(cavecs.o(0).size(), 0)
      for i=0, weights.size()-1 {
        tmpvec = cavecs.o(i).c
        tmpvec.mul(weights.x[i])
        sumvec.add(tmpvec)
      }
      return sumvec
    }

    objref meanca
    meanca = wtsum()


**Python example for accessing shell calcium concentrations and computing weighted average:**

.. code-block:: python

    from neuron import h

    numshells = 4  # for example
    sec = h.Section(name='dend')
    sec.nseg = 1
    sec.insert('cadifus')  # insert multi-shell calcium diffusion mechanism

    # Record calcium in each shell at segment 0.5
    cavecs = [h.Vector().record(getattr(sec(0.5), f'ca[{i}]_cadifus')) for i in range(numshells)]
    tvec = h.Vector().record(h._ref_t)

    # Define shell volume weights: ro^2 - ri^2 for each shell
    # Example weights vector (needs actual radii values)
    import numpy as np
    ri = np.array([0.0, 0.1, 0.2, 0.3])
    ro = np.array([0.1, 0.2, 0.3, 0.4])
    weights = ro**2 - ri**2
    weights /= weights.sum()  # normalize weights to sum = 1

    # After simulation, compute weighted average calcium
    def weighted_average_calcium():
        n = len(cavecs[0])
        avg_ca = [0.0] * n
        for i in range(n):
            avg_ca[i] = sum(cavecs[j][i] * weights[j] for j in range(numshells))
        return avg_ca

**Summary:**

- Use the outermost shell calcium concentration (`cai`) to model calcium-dependent membrane mechanisms, as it represents the local calcium sensed by channels and pumps.
- Use volume-weighted averages across all shells to compare model output with calcium imaging experiments that reflect average intracellular calcium.
- Consider calcium buffering by endogenous buffers and dyes to interpret the relationship between free calcium, dye-bound calcium, and recorded fluorescence signals.

This approach aligns model interpretation with experimental data and improves the physiological relevance of simulation results involving calcium dynamics.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=3422
