.. _hoc_prog_ref:

NEURON HOC documentation
========================
(:ref:`Switch to Python documentation <python_prog_ref>`)

Quick Links
-----------
- :ref:`genindex`, `Download in zip format <hoc_http://www.neuron.yale.edu/neuron/static/new_doc/nrnhelp.zip>`_
- Old documentation: `Quick Index <http://www.neuron.yale.edu/neuron/static/docs/help/quick_reference.html>`_ (contributed by `Mike Neubig <neubig@salk.edu>`_\ ),
  `Index <http://www.neuron.yale.edu/neuron/static/docs/help/index.html>`_\ ,
  `Contents <http://www.neuron.yale.edu/neuron/static/docs/help/hier.html>`_,
  `Download in zip format <http://www.neuron.yale.edu/neuron/static/docs/nrnhelp.zip>`_
- Commonly used:
    :hoc:class:`Deck`, `File <programming/io/file.html>`_, `Glyph <visualization/glyph.html>`_,
    `Graph <visualization/graph.html>`_, `GUIMath <programming/math/guimath.html>`_,
    `List <programming/gui/list.html>`_, `Matrix <programming/math/matrix.html>`_,
    `Pointer <programming/pointers/pointer.html>`_,
    `PtrVector <programming/pointers/ptrvector.html>`_,
    `PWManager <programming/gui/pwman.html>`_,
    `Random <programming/math/random.html>`_, `StringFunctions <programming/strfun.html>`_,
    `SymChooser <programming/gui/symchooser.html>`_,
    `TextEditor <programming/gui/texteditor.html>`_, `Timer <programming/timer.html>`_, `ValueFieldEditor <programming/gui/vfe.html>`_,
    :hoc:class:`VBox`, `Vector <programming/math/vector.html>`_
    
    `BBSaveState <simctrl/bbsavestate.html>`_, `CVode <simctrl/cvode.html>`_, :hoc:class:`FInitializeHandler`, `Impedance <analysis/programmatic/impedance.html>`_,
    `KSChan <modelspec/programmatic/kschan.html>`_, `LinearMechanism <modelspec/programmatic/linmod.html>`_,
    `MechanismStandard <programming/mechstan.html>`_,
    `MechanismType <modelspec/programmatic/mechtype.html>`_, `NetCon <modelspec/programmatic/network/netcon.html>`_, `ParallelContext <modelspec/programmatic/network/parcon.html>`_,
    `ParallelNetManager <modelspec/programmatic/network/parnet.html>`_, `PlotShape <visualization/plotshapeclass.html>`_, :ref:`hoc_Python`,
    `RangeVarPlot <visualization/rvarplt.html>`_, `SaveState <simctrl/savstate.html>`_, `SectionBrowser <programming/gui/secbrows.html>`_,
    `SectionList <modelspec/programmatic/topology/seclist.html>`_, `SectionRef <modelspec/programmatic/topology/secref.html>`_, `Shape <visualization/shape.html>`_,
    `StateTransitionEvent <modelspec/programmatic/ste.html>`_

    :ref:`hoc_panel`, :ref:`hoc_funfit`, :ref:`hoc_geometry`, :ref:`hoc_printf_doc`, :ref:`hoc_ockeywor`, :ref:`hoc_math`,
    :ref:`hoc_nmodl2`, :ref:`hoc_nmodl`, :ref:`hoc_mech`, :ref:`hoc_predec`, :ref:`hoc_standardruntools`,
    :ref:`hoc_ocsyntax`, :ref:`hoc_topology`
    
    :ref:`hoc_neuron_rxd`

Basic Programming
-----------------

.. toctree::
    :maxdepth: 1
    
    programming/hoc.rst
    programming/mathematics.rst
    programming/strings.rst
    programming/guidesign.rst
    programming/references.rst
    programming/timer.rst
    programming/system.rst
    programming/errors.rst
    programming/dynamiccode.rst
    programming/projectmanagement.rst
    programming/python-from-hoc.rst
    programming/internals.rst

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
