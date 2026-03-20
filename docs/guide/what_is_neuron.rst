What is NEURON?
===============

**NEURON** is the gold standard for **biophysically realistic** modeling of neurons
and networks. Unlike point-neuron simulators, NEURON treats cells as complex, branching
3D structures, solving the cable equation at high spatial resolution. It's the bridge
between knowing "what a brain cell is" and "how to turn a brain cell into math."

----

.. grid:: 1 1 2 3
   :gutter: 2

   .. grid-item::

      :octicon:`cpu` **Anatomy & Morphology**

      `Jump to Sections & Segments ↓ <#sections-and-segments-the-building-blocks>`_

   .. grid-item::

      :octicon:`zap` **Biophysics**

      `Jump to Mechanisms ↓ <#biophysical-mechanisms-the-stuff-inside>`_

   .. grid-item::

      :octicon:`pencil` **The Math**

      `Jump to Cable Theory ↓ <#the-underlying-math-cable-theory>`_

   .. grid-item::

      :octicon:`code` **Programming**

      `Jump to Languages ↓ <#the-three-language-system>`_

   .. grid-item::

      :octicon:`database` **ModelDB**

      `Jump to Ecosystem ↓ <#the-ecosystem-modeldb>`_

   .. grid-item::

      :octicon:`telescope` **GUI & Tools**

      `Jump to Visualization ↓ <#gui-vs-coding>`_

----

Why NEURON?
-----------

NEURON is uniquely designed for neuroscientists, not just engineers. You can:

- **Model realistic anatomy:** Import morphometric data from Neurolucida, SWC, or draw cells by hand
- **Insert biophysics:** Add ion channels, synapses, pumps, and diffusion with simple commands
- **Leverage 40 years of development:** Battle-tested algorithms for numerical stability and speed
- **Access 850+ published models:** Start from `ModelDB <https://modeldb.science>`_ rather than from scratch
- **Scale seamlessly:** From a single compartment on your laptop to millions of cells on HPC clusters with GPU support
- **Stay close to experimental data:** Units, concepts, and tools mirror what you'd use in the lab

.. admonition:: Key Strengths
   :class: tip

   - **Free and open source** — runs on Windows, macOS, Linux, and HPC
   - **Widely used** — over 3,000 publications as of 2026
   - **Actively developed** — new releases ~twice per year
   - **Well documented** — programmer's reference, tutorials, videos, and *The NEURON Book*

----

Core Concepts
-------------

Sections and Segments: The Building Blocks
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

NEURON views a cell as a collection of connected cables, not a single point. This is what
makes it **spatially realistic**.

**Sections** represent unbranched pieces of the neuron:

- **Soma** — the cell body
- **Dendrites** — branching input structures
- **Axon** — the output cable

Each section is continuous and can be connected to other sections to form a branched tree.

.. code-block:: python

   from neuron import n

   soma = n.Section("soma")
   dend = n.Section("dend")
   dend.connect(soma(1))  # Connect dend to the distal end (1) of soma

**Segments (Compartments)** are the actual units of simulation.  NEURON automatically
divides each section into discrete segments where the cable equation is solved.

- **The more segments, the higher the spatial resolution** — but also the slower the simulation
- NEURON can automatically set the number of segments using the **d_lambda rule** based on the electrical length of the section
- You control segment count with ``nseg``: ``soma.nseg = 5``

.. admonition:: Why This Matters
   :class: note

   With sections and segments, you can model phenomena that require spatial resolution:
   attenuation of synaptic inputs along a dendrite, the back-propagation of action
   potentials, or differences in channel density across the cell.

See also: :ref:`topology` and :ref:`geometry` in the Programmer's Reference.

----

Biophysical Mechanisms: The "Stuff" Inside
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Once you have the shape (morphology), you must tell NEURON how that shape behaves.
This is done by **inserting mechanisms** into sections.

**Ion Channels** control the flow of ions across the membrane:

.. code-block:: python

   soma.insert(n.hh)  # Hodgkin–Huxley Na+ and K+ channels
   soma.hh.gnabar = 0.12  # Set sodium conductance

Built-in mechanisms include ``hh`` (Hodgkin–Huxley), ``pas`` (passive leak), and many
others. You can also define custom channels using **NMODL** (see below).

**Point Processes** represent localized phenomena:

- **Synapses:** ``ExpSyn``, ``Exp2Syn`` (AMPA, GABA, etc.)
- **Electrodes:** ``IClamp`` (current injection), ``SEClamp`` (voltage clamp)
- **Artificial spiking cells:** ``IntFire1``, ``IntFire2``

.. code-block:: python

   stim = n.IClamp(soma(0.5))  # Electrode at the midpoint
   stim.delay = 5  # ms
   stim.dur = 1    # ms
   stim.amp = 0.5  # nA

**User-Defined Mechanisms:** Write your own ion channels, pumps, or diffusion processes in
**NMODL**, a high-level language for expressing kinetic schemes and differential equations.
Alternatively, use the **ChannelBuilder** GUI tool for voltage- and ligand-gated channels.

.. admonition:: Where to Find Mechanisms
   :class: tip

   Don't reinvent the wheel. Most ion channels have already been implemented. Check
   `ModelDB <https://modeldb.science>`_ or the
   `NEURON mod file repository <https://github.com/neuronsimulator/nrn/tree/master/share/examples/nrniv/nmodl>`_.

See also: :ref:`mech` in the Programmer's Reference.

----

The Underlying Math: Cable Theory
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

NEURON exists to solve the **cable equation** — a partial differential equation that
describes how voltage changes over time and space in a cylindrical conductor (like a
dendrite or axon).

For each segment, NEURON solves:

.. math::

   c_m \frac{\partial V}{\partial t} + I_{\text{ion}} = \frac{a}{2R_i} \frac{\partial^2 V}{\partial x^2}

Where:
  - $c_m$ = membrane capacitance (typically 1 µF/cm²)
  - $V$ = membrane potential (mV)
  - $I_{\text{ion}}$ = sum of ionic currents from all mechanisms (mA/cm²)
  - $a$ = radius of the cable (µm)
  - $R_i$ = cytoplasmic (axial) resistivity (Ω·cm)

You don't need to be a calculus wizard to use NEURON, but understanding that the software
is numerically integrating this equation helps you appreciate why:

- **Spatial discretization matters:** Too few segments → inaccurate voltage gradients
- **Time step matters:** Too large ``dt`` → numerical instability
- **NEURON is fast:** It uses optimized algorithms that exploit the structure of branching cables

**Integration Methods:**

- **Implicit Euler** (default) — robust and stable
- **Crank-Nicolson** — second-order accuracy, but can oscillate if ``dt`` is too large
- **CVODE (adaptive)** — automatically adjusts time step and integration order for accuracy

You can switch between methods without rewriting your model:

.. code-block:: python

   cvode = n.CVode()
   cvode.active(True)  # Enable adaptive integration

See also: `CVode <../../progref/simctrl/cvode.html>`_ in the Programmer's Reference.

----

The Three-Language System
~~~~~~~~~~~~~~~~~~~~~~~~~

This is often the most confusing part for newcomers. NEURON uses **three languages**:

1. **Python** (Recommended for New Users)
   
   Modern, widely used, great for data analysis and plotting. Most tutorials now use Python.

   .. code-block:: python

      from neuron import n, gui
      soma = n.Section("soma")
      soma.L = 20  # µm
      soma.diam = 20

2. **HOC** (High-Order Calculator, pronounced "h-oak")
   
   The legacy proprietary language. You'll encounter it in older models and GUI-generated code.
   Still fully supported but not recommended for new projects.

   .. code-block:: none

      create soma
      soma { L = 20  diam = 20 }

   Python can call HOC and vice versa via dot notation, ``n("hoc_string")`` and ``nrnpython()``.

3. **NMODL** (NEURON MODel Language)
   
   A specialized language for defining **new biophysical mechanisms** (ion channels, pumps, synapses).
   You write ``.mod`` files, then compile them with ``nrnivmodl``.

   .. code-block:: none

      NEURON {
        SUFFIX myChannel
        USEION na WRITE ina
      }
      STATE { m h }

.. admonition:: Bottom Line
   :class: tip

   - **Start with Python** for scripting and model building.
   - **Know HOC exists** so you can read older code.
   - **Use NMODL only when** you need custom mechanisms not in `ModelDB <https://modeldb.science>`_.

See also: :ref:`python_accessing_hoc` and `NMODL language <../../nmodl/language.html>`_.

----

The Ecosystem: ModelDB
~~~~~~~~~~~~~~~~~~~~~~

You're not starting from scratch. `ModelDB <https://modeldb.science>`_ is a massive
repository of published models — over 1,900 computational neuroscience models, many
written in NEURON.

**How to use ModelDB:**

1. Search by author, cell type, brain region, ion channel, or publication
2. Download the source code (often includes data and scripts)
3. Run it to see if it does what the paper claims
4. **Adapt it** — most researchers find a model "close enough" and modify it

.. admonition:: ModelDB is Your Starting Point
   :class: note

   Very few researchers write a model from scratch. Instead, they find a similar model,
   verify it works, then adapt the morphology, ion channels, or network connectivity
   for their own research question.

----

GUI vs. Coding
~~~~~~~~~~~~~~

NEURON's **Graphical User Interface (GUI)** is incredibly powerful for:

- Visualizing cell morphology in 3D
- Watching voltage plots update in real time
- Quickly injecting current or applying voltage clamps
- Building cells with the **CellBuilder**
- Importing morphometric data with **Import3D**
- Creating networks with the **Network Builder**

.. code-block:: python

   from neuron import n, gui  # Launch the GUI

.. admonition:: When to Use the GUI
   :class: tip

   - **Learning:** Use the GUI to *understand* NEURON's concepts and explore parameter space.
   - **Quick tests:** Inject current, watch a spike, adjust a conductance — all with sliders.
   - **Import morphology:** The Import3D tool is easier than writing code to parse SWC files.

   **But for reproducibility and automation**, write Python scripts. The GUI is great for
   exploration; code is essential for publishable, reproducible science.

**Key GUI Tools:**

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Tool
     - Purpose
   * - **CellBuilder**
     - Create or modify cell models without code
   * - **Import3D**
     - Load morphometric data (Neurolucida, SWC, Eutectic formats)
   * - **ChannelBuilder**
     - Design ion channels with kinetic schemes or ODEs
   * - **Network Builder**
     - Prototype small networks graphically
   * - **RunControl**
     - Start/stop simulations, set ``dt`` and duration
   * - **Graph**
     - Plot voltage, current, or any variable vs. time
   * - **Shape Plot**
     - Visualize cell morphology and spatial voltage

See also: :doc:`cellbuilder`, :doc:`import3d`, and :doc:`network_builder_tutorials`.

----

Advanced Features
-----------------

Advantages over General-Purpose Simulators
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Advanced Features
-----------------

Advantages over General-Purpose Simulators
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

NEURON was born in John W. Moore's lab at Duke University in the 1980s. Unlike general-purpose
simulators, NEURON's algorithms are **optimized for the structure of neuronal cable equations**,
making it orders of magnitude faster for biophysically detailed models.

**Key advantages:**

- **Natural syntax:** Specify models using neuroscience concepts (sections, ion channels, synapses) instead of generic differential equations
- **Integration-independent model specification:** Switch between Euler, Crank-Nicolson, or CVODE without rewriting your model
- **Automatic spatial discretization:** NEURON can apply the **d_lambda rule** to set segment counts automatically based on electrical length
- **Optimized for neurons:** Special algorithms exploit the branching cable structure for speed

Network Modeling
~~~~~~~~~~~~~~~~

Although NEURON started with single-cell models, it now excels at **large-scale networks**:

- **Event-driven synapses:** Efficient spike delivery system with ``NetCon`` objects
- **Artificial cells:** Fast integrate-and-fire neurons (``IntFire1``, ``IntFire2``) can be mixed with biophysical cells
- **Parallel simulation:** Distribute networks across CPU cores or HPC clusters with ``ParallelContext``
- **Synaptic plasticity:** Stream-specific STDP, depression, facilitation

.. code-block:: python

   # Connect two cells with a synapse
   syn = n.ExpSyn(target_cell.soma(0.5))
   nc = n.NetCon(source_cell.soma(0.5)._ref_v, syn, sec=source_cell.soma)
   nc.weight[0] = 0.01  # µS
   nc.delay = 1         # ms

See also: `NetCon <../../progref/modelspec/programmatic/network/netcon.html>`_
and `ParallelContext <../../progref/modelspec/programmatic/network/parcon.html>`_.

Reaction-Diffusion (rxd)
~~~~~~~~~~~~~~~~~~~~~~~~~

Model intracellular and extracellular chemistry alongside electrophysiology:

- Calcium dynamics, buffering, and pumps
- IP₃ signaling and calcium waves
- Neurotransmitter diffusion in the extracellular space
- Gene regulation and metabolism

.. code-block:: python

   from neuron import n, rxd
   cyt = rxd.Region(n.allsec(), name="cyt", nrn_region="i")
   ca = rxd.Species(cyt, name="ca", charge=2, initial=50e-6)  # 50 nM

See also: :ref:`neuron_rxd` and the `RXD Tutorials <../../rxd-tutorials/index.html>`_.

----

Support and Resources
---------------------

NEURON is Actively Developed
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- **New releases** appear ~twice per year, with bug fixes as needed
- **GitHub repository:** `github.com/neuronsimulator/nrn <https://github.com/neuronsimulator/nrn>`_
- **Discussion Board:** Ask questions at `github.com/neuronsimulator/nrn/discussions <https://github.com/neuronsimulator/nrn/discussions>`_
- **Legacy Forum:** Browse archived discussions at `neuron.yale.edu/phpBB <https://neuron.yale.edu/phpBB>`_

Documentation and Training
~~~~~~~~~~~~~~~~~~~~~~~~~~

- **The NEURON Book** (Carnevale and Hines, 2006) — the authoritative reference
- **Online tutorials:** Python Ball-and-Stick series, RXD tutorials
- **Training videos:** Recorded courses from 2021-2022 on YouTube
- **One-day SfN workshops** and **five-day summer courses** (UCSD and other locations)
- **Programmer's Reference:** :ref:`Complete API documentation <prog_ref>`

.. admonition:: Over 3,000 Publications
   :class: note

   As of March 2026, over 3,000 scientific articles and books have reported work done
   with NEURON. See :doc:`../publications-using-neuron` for a partial list.

----

System Requirements
-------------------

NEURON is **free, open source**, and runs on:

- **Windows** (10, 11)
- **macOS** (Intel and Apple Silicon)
- **Linux** (all major distributions)
- **HPC clusters** (Beowulf, IBM Blue Gene, Cray, NVIDIA GPU support via CoreNEURON)

**Installation:**

For macOS, Linux, and most cloud compute environments:

.. code-block:: bash

   pip install neuron

or (Windows) download binary installers from `www.neuronsimulator.org <https://www.neuronsimulator.org>`_.

See :doc:`../install/install` for detailed instructions.

----

References and Further Reading
-------------------------------

**Key Publications:**

- Carnevale, N.T. and Hines, M.L. (2006). *The NEURON Book.* Cambridge University Press.
- Hines, M.L. and Carnevale, N.T. (2001). NEURON: a tool for neuroscientists. *The Neuroscientist* 7:123-135.
- Migliore, M., Cannia, C., Lytton, W.W., Markram, H., and Hines, M.L. (2006). Parallel network simulations with NEURON. *Journal of Computational Neuroscience* 21:110-119.

**Other Books Using NEURON:**

- Destexhe, A. and Sejnowski, T.J. (2001). *Thalamocortical Assemblies.* Oxford University Press.
- Johnston, D. and Wu, S.M.-S. (1995). *Foundations of Cellular Neurophysiology.* MIT Press.
- Lytton, W.W. (2002). *From Computer to Brain.* Springer-Verlag.
- Moore, J.W. and Stuart, A.E. (2000). *Neurons in Action: Computer Simulations with NeuroLab.* Sinauer Associates.

**Numerical Methods:**

- Hindmarsh, A.C. and Serban, R. (2002). User documentation for CVODES. Lawrence Livermore National Laboratory. (`SUNDIALS <https://computation.llnl.gov/projects/sundials>`_)
- Hindmarsh, A.C. and Taylor, A.G. (1999). User documentation for IDA. Lawrence Livermore National Laboratory.

----

Next Steps
----------

.. grid:: 1 1 2 2
   :gutter: 3

   .. grid-item-card:: :octicon:`rocket` Get Started
      :link: how_to_get_started
      :link-type: doc

      Installation, first steps, and where to find help.

   .. grid-item-card:: :octicon:`mortar-board` Tutorials
      :link: ../tutorials/index
      :link-type: doc

      Work through the Ball-and-Stick series to build your first model.

   .. grid-item-card:: :octicon:`code` Programmer's Reference
      :link: ../progref/index
      :link-type: doc

      Complete API documentation for all classes and functions.

   .. grid-item-card:: :octicon:`database` ModelDB
      :link: https://modeldb.science
      :link-type: url

      Browse and download published NEURON models.

