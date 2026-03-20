.. _prog_ref:

NEURON Programmer's Reference
=============================

This is the complete reference for NEURON's Python and HOC programming
interfaces. For hands-on introductions, see the `Python tutorials
<../tutorials/index.html>`_ or the :ref:`NEURON scripting basics
<launching_hoc_and_python_scripts>`.

NEURON supports multiple programming languages. Most users work in Python,
but HOC is also fully supported. See :ref:`python_accessing_hoc` and
:ref:`hoc_python_accessing_hoc` for interoperability.

----

At a Glance
------------

The most commonly needed classes and topics, organized by task:

.. grid:: 1 1 2 2
   :gutter: 3

   .. grid-item-card:: Build Cell Morphology
      :class-title: sd-fs-5

      Create and connect sections to define cell shape and topology.

      - `Section <modelspec/programmatic/topology/section.html>`_ ---
        the fundamental building block
      - `SectionList <modelspec/programmatic/topology/seclist.html>`_ ---
        group and iterate over sections
      - `SectionRef <modelspec/programmatic/topology/secref.html>`_ ---
        reference to a section
      - :ref:`topology` --- connecting sections
      - :ref:`geometry` --- length, diameter, 3-D points

   .. grid-item-card:: Add Biophysics
      :class-title: sd-fs-5

      Insert ion channels, set conductances, and define custom mechanisms.

      - :ref:`mech` --- ``insert()``, range variables
      - `MechanismType <modelspec/programmatic/mechtype.html>`_ ---
        enumerate available mechanisms
      - `MechanismStandard <programming/mechstan.html>`_ ---
        inspect mechanism parameters
      - `KSChan <modelspec/programmatic/kschan.html>`_ ---
        kinetic scheme channels
      - :ref:`nmodl` / :ref:`nmodl2` --- the NMODL language

   .. grid-item-card:: Stimulate & Record
      :class-title: sd-fs-5

      Inject current, clamp voltage, and capture simulation output.

      - `Vector <programming/math/vector.html>`_ ---
        ``record()``, ``play()``, data storage
      - :ref:`standardruntools` --- ``finitialize``, ``continuerun``
      - `CVode <simctrl/cvode.html>`_ --- variable time-step integration
      - `SaveState <simctrl/savstate.html>`_ /
        `BBSaveState <simctrl/bbsavestate.html>`_ --- checkpointing
      - :class:`FInitializeHandler` --- custom init actions

   .. grid-item-card:: Build Networks
      :class-title: sd-fs-5

      Wire cells together with synapses, spike detectors, and gap junctions.

      - `NetCon <modelspec/programmatic/network/netcon.html>`_ ---
        spike-triggered connections
      - `ParallelContext <modelspec/programmatic/network/parcon.html>`_ ---
        parallel and distributed simulation
      - `ParallelNetManager <modelspec/programmatic/network/parnet.html>`_ ---
        high-level network manager
      - `LinearMechanism <modelspec/programmatic/linmod.html>`_ ---
        linear circuit elements
      - `StateTransitionEvent <modelspec/programmatic/ste.html>`_ ---
        discrete event models

   .. grid-item-card:: Reaction-Diffusion
      :class-title: sd-fs-5

      Model intracellular and extracellular chemistry.

      - :ref:`neuron_rxd` --- overview of the rxd module
      - `RXD Tutorials <../rxd-tutorials/index.html>`_ ---
        hands-on examples
      - `Impedance <analysis/programmatic/impedance.html>`_ ---
        frequency-domain analysis

   .. grid-item-card:: Visualization & GUI
      :class-title: sd-fs-5

      Plot results, display morphology, and build interactive interfaces.

      - `Graph <visualization/graph.html>`_ ---
        time-series and X-Y plots
      - `Shape <visualization/shape.html>`_ /
        `PlotShape <visualization/plotshapeclass.html>`_ ---
        cell morphology display
      - `RangeVarPlot <visualization/rvarplt.html>`_ ---
        range variable visualization
      - :ref:`panel` --- custom GUI panels
      - :class:`Deck`, :class:`VBox` ---
        GUI layout containers

----

Frequently Used Classes
-----------------------

.. list-table::
   :header-rows: 1
   :widths: 25 50 25

   * - Class
     - Purpose
     - Category
   * - `Vector <programming/math/vector.html>`_
     - Record, store, and manipulate arrays of doubles
     - Data
   * - `NetCon <modelspec/programmatic/network/netcon.html>`_
     - Connect a spike source to a synaptic target
     - Network
   * - `ParallelContext <modelspec/programmatic/network/parcon.html>`_
     - Parallel and distributed simulation
     - Network
   * - `SectionList <modelspec/programmatic/topology/seclist.html>`_
     - Group sections for iteration
     - Morphology
   * - `CVode <simctrl/cvode.html>`_
     - Variable time-step integrator
     - Simulation
   * - `Random <programming/math/random.html>`_
     - Random number generation
     - Math
   * - `Matrix <programming/math/matrix.html>`_
     - Matrix operations
     - Math
   * - `Graph <visualization/graph.html>`_
     - Plot data
     - Visualization
   * - `Shape <visualization/shape.html>`_
     - Display cell morphology
     - Visualization
   * - `File <programming/io/file.html>`_
     - File I/O operations
     - Programming
   * - `MechanismType <modelspec/programmatic/mechtype.html>`_
     - Query available mechanisms
     - Model

Additional commonly used classes:
`Glyph <visualization/glyph.html>`_,
`GUIMath <programming/math/guimath.html>`_,
`Impedance <analysis/programmatic/impedance.html>`_,
`KSChan <modelspec/programmatic/kschan.html>`_,
`LinearMechanism <modelspec/programmatic/linmod.html>`_,
`List <programming/gui/list.html>`_,
`MechanismStandard <programming/mechstan.html>`_,
`ParallelNetManager <modelspec/programmatic/network/parnet.html>`_,
`PlotShape <visualization/plotshapeclass.html>`_,
`Pointer <programming/pointers/pointer.html>`_,
`PtrVector <programming/pointers/ptrvector.html>`_,
`PWManager <programming/gui/pwman.html>`_,
`RangeVarPlot <visualization/rvarplt.html>`_,
`SectionBrowser <programming/gui/secbrows.html>`_,
`SectionRef <modelspec/programmatic/topology/secref.html>`_,
`StringFunctions <programming/strfun.html>`_,
`SymChooser <programming/gui/symchooser.html>`_,
`TextEditor <programming/gui/texteditor.html>`_,
`Timer <programming/timer.html>`_,
`ValueFieldEditor <programming/gui/vfe.html>`_.

Commonly referenced topics:
:ref:`funfit`, :ref:`printf_doc`, :ref:`math`.

See also the :ref:`genindex` for a complete alphabetical listing.

----

Basic Programming
-----------------

General-purpose programming facilities: math, strings, I/O, GUI design,
dynamic code loading, and Python--HOC interoperability.

.. toctree::
    :maxdepth: 1

    programming/mathematics.rst
    programming/strings.rst
    programming/guidesign.rst
    programming/references.rst
    programming/timer.rst
    programming/system.rst
    programming/errors.rst
    programming/dynamiccode.rst
    programming/projectmanagement.rst
    programming/internals.rst
    programming/neuron_classes.rst
    programming/hoc-from-python.rst

Model Specification
-------------------

Define cell morphology, insert biophysical mechanisms, specify ions and
channels, build networks, and model reaction-diffusion.

.. toctree::
    :maxdepth: 1

    modelspec/guitools.rst
    modelspec/programmatic.rst

Simulation Control
------------------

Initialize, run, and manage simulations. Configure integrators, set
environment variables, and control the interpreter.

.. toctree::
    :maxdepth: 1

    simctrl/programmatic.rst
    simctrl/stdrun.rst
    compilationoptions.rst
    envvariables.rst
    simctrl/interpretermanagement.rst

Visualization
-------------

Plot simulation results, display cell morphology, and create interactive
graphical interfaces.

.. toctree::
    :maxdepth: 1

    visualization/glyph.rst
    visualization/graph.rst
    visualization/grapher.rst
    visualization/plotshapeclass.rst
    visualization/plotshape.rst
    visualization/rvarplt.rst
    visualization/shape.rst
    visualization/notify.rst
    visualization/gui.rst
    visualization/shapebox.rst
    visualization/oldgrph.rst

Analysis
--------

Programmatic and GUI-based tools for analyzing simulation output, including
impedance analysis and curve fitting.

.. toctree::
    :maxdepth: 1

    analysis/programmatic.rst
    analysis/guitools.rst
