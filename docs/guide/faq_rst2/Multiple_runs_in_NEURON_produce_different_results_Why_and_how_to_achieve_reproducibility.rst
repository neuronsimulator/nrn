Multiple runs in NEURON produce different results: Why and how to achieve reproducibility?
==========================================================================================

When running multiple simulations in NEURON with identical parameters, you may observe different results between runs, even if nothing in the code has changed. This variability is commonly caused by pseudorandom number generators (PRNGs) used in the model not being properly reinitialized before each run.

**Cause of variability:**

- If your model uses mechanisms such as `NetStim` objects with randomness, each run may produce different spatiotemporal patterns of synaptic activation if the pseudorandom number generators are not reseeded.
- By default, all `NetStim` objects share a common PRNG sequence. If you do not reset the seed at the start of each run, the spike trains and consequently the model outputs vary.

**How to fix this:**

- Use the `.seed()` method of `NetStim` to reset the PRNG seed before each simulation run.
- It is sufficient to seed one `NetStim` to affect all, since they share the same underlying PRNG.
- For independent spike trains per `NetStim`, assign separate random number generators to each `NetStim`.

**Example: Python usage**

.. code-block:: python

    from neuron import h, gui

    NSNUM = 150
    NetStim = [h.NetStim() for _ in range(NSNUM)]

    def run_sim(seed=31):
        # Reseed the shared PRNG for reproducibility
        NetStim[0].seed(seed)

        h.stdinit()
        h.continuerun(100)  # run simulation up to 100 ms

    for run_index in range(2):
        print(f"Run {run_index + 1}")
        run_sim()

**Example: HOC usage**

.. code-block:: hoc

    objref NetStim[150]
    for i=0,149 {
        NetStim[i] = new NetStim()
    }

    proc run_sim() {
        NetStim[0].seed(31)  // reseed shared PRNG
        stdinit()
        continuerun(100)
    }

    for i=0,1 {
        printf("Run %d\n", i+1)
        run_sim()
    }

**Additional notes:**

- Using `NetStim[ii]` directly (without creating objrefs) is possible but not recommended for robust programming.
- If you prefer each `NetStim` to be independent (e.g., for parallel simulations), you must assign each its own PRNG instance.
- Failing to reseed PRNGs can lead to irreproducible simulations, making repeated runs yield different results.

By properly reseeding the pseudorandom number generators before each run, you ensure reproducibility of your NEURON simulations.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=2747
