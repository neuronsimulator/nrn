.. _python_prog_ref:

NEURON Python documentation
===========================
(:ref:`Switch to HOC documentation <hoc_prog_ref>`)

For a basic introduction on how to run Python scripts with NEURON, see
:ref:`running Python scripts with NEURON <launching_hoc_and_python_scripts>`,
and you may also like to refer to the `NEURON Python tutorial
<../tutorials/scripting-neuron-basics.html>`_.

Quick Links
-----------
- :ref:`genindex`
- Commonly used:
    :class:`Deck`, `File <programming/io/file.html>`_, `Glyph <visualization/glyph.html>`_,
    `Graph <visualization/graph.html>`_, `GUIMath <programming/math/guimath.html>`_,
    `List <programming/gui/list.html>`_, `Matrix <programming/math/matrix.html>`_,
    `Pointer <programming/pointers/pointer.html>`_,
    `PtrVector <programming/pointers/ptrvector.html>`_,
    `PWManager <programming/gui/pwman.html>`_,
    `Random <programming/math/random.html>`_, `StringFunctions <programming/strfun.html>`_,
    `SymChooser <programming/gui/symchooser.html>`_,
    `TextEditor <programming/gui/texteditor.html>`_, `Timer <programming/timer.html>`_, `ValueFieldEditor <programming/gui/vfe.html>`_,
    :class:`VBox`, `Vector <programming/math/vector.html>`_
    
    `BBSaveState <simctrl/bbsavestate.html>`_, `CVode <simctrl/cvode.html>`_, :class:`FInitializeHandler`, `Impedance <analysis/programmatic/impedance.html>`_,
    `KSChan <modelspec/programmatic/kschan.html>`_, `LinearMechanism <modelspec/programmatic/linmod.html>`_,
    `MechanismStandard <programming/mechstan.html>`_,
    `MechanismType <modelspec/programmatic/mechtype.html>`_, `NetCon <modelspec/programmatic/network/netcon.html>`_, `ParallelContext <modelspec/programmatic/network/parcon.html>`_,
    `ParallelNetManager <modelspec/programmatic/network/parnet.html>`_, `PlotShape <visualization/plotshapeclass.html>`_,
    `RangeVarPlot <visualization/rvarplt.html>`_, `SaveState <simctrl/savstate.html>`_, `SectionBrowser <programming/gui/secbrows.html>`_,
    `SectionList <modelspec/programmatic/topology/seclist.html>`_, `SectionRef <modelspec/programmatic/topology/secref.html>`_, `Shape <visualization/shape.html>`_,
    `StateTransitionEvent <modelspec/programmatic/ste.html>`_

    :ref:`panel`, :ref:`funfit`, :ref:`geometry`, :ref:`printf_doc`, :ref:`math`,
    :ref:`nmodl2`, :ref:`nmodl`, :ref:`mech`, :ref:`standardruntools`,
    :ref:`topology`
    
    :ref:`neuron_rxd`

Basic Programming
-----------------

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

.. toctree::
    :maxdepth: 1
    
    modelspec/guitools.rst
    modelspec/programmatic.rst

Simulation Control
------------------

.. toctree::
    :maxdepth: 1
    
    simctrl/programmatic.rst
    simctrl/stdrun.rst
    compilationoptions.rst
    envvariables.rst
    simctrl/interpretermanagement.rst

Visualization
-------------

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

.. toctree::
    :maxdepth: 1
    
    analysis/programmatic.rst
    analysis/guitools.rst
