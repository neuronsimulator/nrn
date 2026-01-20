New installation of NEURON with Python
======================================

How do I install NEURON with Python on Windows 10 using Anaconda?

To install NEURON with Python support on Windows 10 (64-bit) using Anaconda Python 3.11, follow these steps:

1. Install Anaconda Python 3.11 if not already installed.
2. Install the latest NEURON version (8.2.3 or later) from the official documentation site or via the recommended installer.
3. Verify the installation by running NEURON demos and importing NEURON modules in Python.

**Step-by-step verification:**

- Run NEURON demos:

  Open the NEURON bash terminal from the NEURON program group and execute:

  .. code-block:: bash

      neurondemo

  This will open multiple demonstration panels. Use the GUI controls to run simulations and visualize results.

- Verify NEURON Python module in Anaconda Prompt:

  Open Anaconda Prompt and start Python:

  .. code-block:: bash

      python

  In the Python prompt, import NEURON modules:

  .. code-block:: python

      from neuron import h, gui

  The NEURON Main Menu GUI should appear, confirming that NEURON is properly integrated with Python.

**Note:**  
If you encounter errors such as "no module named hoc", ensure you have installed the latest NEURON version (8.2.3 or newer) from the official recommended source rather than older installers.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=4628
