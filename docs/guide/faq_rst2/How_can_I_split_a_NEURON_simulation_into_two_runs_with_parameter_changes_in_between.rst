How can I split a NEURON simulation into two runs with parameter changes in between?
=====================================================================================

You can run a NEURON simulation in parts, modify parameters after the first part, and then continue the simulation such that the final state of the first run becomes the initial state for the second. To do this correctly, especially when using CVODE, you should reinitialize the solver or update the currents before continuing the simulation.

Example in Python
-----------------
.. code-block:: python

  from neuron import h

  # Initialize and run the first part of the simulation
  h.finitialize(-65)
  h.continuerun(300)

  # Make any parameter changes here
  # e.g., my_cell.soma(0.5).gbar_na *= 1.2

  # Reinitialize CVODE or update currents before continuing
  if h.cvode.active():
      h.cvode.re_init()
  else:
      h.fcurrent()

  # Continue the simulation to desired new stop time
  h.continuerun(1000)

Example in Hoc
--------------
.. code-block:: hoc

  finitialize(-65)
  continuerun(300)

  /* Make parameter changes here */
  /* e.g. soma gbar_na = gbar_na*1.2 */

  if (cvode.active()) {
    cvode.re_init()
  } else {
    fcurrent()
  }

  continuerun(1000)

Recording Variables Over Split Simulations
------------------------------------------
Define your `Vector.record` objects before the first initialization and run. These vectors will record data over the entire simulation (both runs). If you want to reset recording vectors to store only the second segment, use `h.frecord_init()` before the second `continuerun()` call.

Example:

.. code-block:: python
    
  soma_v = h.Vector().record(my_cell.soma(0.5)._ref_v)
  t = h.Vector().record(h._ref_t)

  h.finitialize(-65)
  h.continuerun(300)

  # Change parameters here
  if h.cvode.active():
      h.cvode.re_init()
  else:
      h.fcurrent()

  # Reset recording to capture only the second run (optional)
  h.frecord_init()

  h.continuerun(1000)


Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=4593
