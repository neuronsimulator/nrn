.. _bio_faq:

Biology Modeling FAQ
====================

How do I work with neuron morphologies?
---------------------------------------

`NeuroMorpho.Org <http://neuromorpho.org>`_ hosts over 170k reconstructed neurons,
all of which are available in SWC format as well as their original format.

NEURON's Import3D tool can read SWC files as well as several other neuron morphology
types.

The Import3D tool can be used both through the GUI and programmatically, however
it is generally best to start with the GUI to explore the morphologies and make
sure they are suitable for use with simulation. See the
:ref:`Import3D GUI tutorial <import3d_tutorial>` for more on GUI usage.

Example:

    To create ``pyr``, a Pyramidal cell object with morphology from a file called 
    ``c91662.swc``:

    .. code::
        python

        from neuron import h
        h.load_file("stdlib.hoc")
        h.load_file("import3d.hoc")

        class Pyramidal:
            def __init__(self):
                self.load_morphology()
                # do discretization, ion channels, etc
            def load_morphology(self):
                cell = h.Import3d_SWC_read()
                cell.input("c91662.swc")
                i3d = h.Import3d_GUI(cell, False)
                i3d.instantiate(self)
        
        pyr = Pyramidal()

Here ``pyr`` has lists of :class:`Section` objects:
``pyr.apic``, ``pyr.axon``, ``pyr.soma``, ``pyr.all``.
Each section has the appropriate ``.name()`` and ``.cell()``.

Note: this example is for an SWC file specifically; other readers are supported
for different formats including ``h.Import3d_Neurolucida3()``,
``h.Import3d_MorphML()``, and ``h.Import3d_Eutectic_read()``.

Note: if multiple cells are instantiated from the same reconstruction, they
will occupy the same spatial locations unless they are explicitly translated.

How do I simulate a current clamp pulse experiment?
---------------------------------------------------

Use an :class:`IClamp` at a segment of your choice. You can specify the ``delay`` (in ms) (when the clamp starts),
the ``amp`` (amplitude in nA) of the current pulse and the ``dur`` (duration in ms). The injected current may
be monitored through the IClamp's ``i`` state, or recorded using its ``_ref_i``.

For example, the following code specifies that a current of 10 nA will be injected directly into the
center of the soma from t = 1 ms to t = 1.1 ms:

.. code::
   python

   from neuron import h
   from neuron.units import ms
   
   # setup the model here

   ic = h.IClamp(soma(0.5))
   ic.amp = 10
   ic.dur = 0.1 * ms
   ic.delay = 1 * ms

.. note::

    You must keep the ``ic`` object accessible in some way (either by assigning it to a variable
    or storing it in a list); if it becomes inaccessible, then Python will "garbage collect" it and it
    will be removed from the simulation.

For a runnable example, see 
`these tutorial exercises <https://colab.research.google.com/drive/1W1szHYfl8jjOlmZ270Jmk-qOrp9UMDr6?usp=sharing>`_.

How do I simulate a current clamp with non-pulse behavior?
----------------------------------------------------------

In this scenario, you likely have or can construct two pairs of :class:`Vector` objects:
``i_stim`` with the injected current (in nA) measured at various time points and ``t_stim`` the corresponding
time points (in ms).

.. note::

    If instead of Vectors, you have Python lists, numpy arrays, or other iterables,
    you can get an equivalent Vector via

    .. code::
        python

        t_stim_vec = h.Vector(t_stim)

Use an h. :class:`IClamp` at a segment of your choice as described above, set the ``delay`` (start time) to 0,
the ``dur`` ation to a large number (e.g. 1e9) and use :meth:`Vector.play` to play into the ``_ref_amp`` field
using interpolation (the ``True`` in the following); e.g.

.. code::
    python

    ic = h.IClamp(soma(0.5))
    ic.delay = 0
    ic.dur = 1e9
    i_stim.play(ic._ref_amp, t_stim, True)

For a runnable example, see 
`this example <https://colab.research.google.com/drive/1Jj7Ke1kZSGja1FNNj66XGCdOruKY_oqS?usp=sharing>`_.

.. _ion_channel_accumulation_bio_faq:
How do I make cytosolic concentrations change in response to ion channel activity?
----------------------------------------------------------------------------------

NEURON defaults to assuming homeostatic mechanisms maintain intracellular concentration as that
is often the assumption made by modelers, however this can easily be changed when the circumstances
warrant.

For certain ions (e.g. calcium) the changes due to channel activity are significant.
Likewise, in pathological conditions (e.g. ischemic stroke), even ions like sodium and potassium
may show significant change.

To specify that intracellular sodium concentration on all sections (:func:`allsec`)
is to be affected by ion channel activity:

.. code::
    python

    from neuron import rxd
    cyt = rxd.Region(h.allsec(), name="cyt", nrn_region="i")
    na = rxd.Species(cyt, name="na", charge=1)

Here the ``nrn_region="i"`` indicates that we are talking about the intracellular concentration.
The ``name`` argument to :class:`rxd.Species` specifies the name of the ion.
By default :class:`rxd.Region` assumes that we're describing a Region filling the entire Section;
but this can be altered with additional arguments. The ``charge=1`` corresponds to the fact that sodium
ions have a charge of +1. By contrast calcium ions have a charge of +2, and thus to tell NEURON to consider
how calcium changes due to ion channel activity we write:

.. code::
    python

    ca = rxd.Species(cyt, name="ca", atolscale=1e-6, charge=2)

Here we have also added the optional parameter ``atolscale``. It has no effect in fixed-step
simulations, but for variable step simulations (see :class:`CVode`) it is a hint that concentrations
for calcium are often much smaller than those for sodium and that it should seek much smaller
errors in calcium in terms of absolute numbers.

(As an aside, it is generally good practice *not* to use :func:`allsec` but to instead explicitly
identify the sections to be used. NEURON provides the :meth:`Section.wholetree` method for getting
a Python list of all sections that belong to a cell containing a specified section. It would be natural
to include specification that concentration is to change on a per-cell basis within a cell class; this
compartmentalization allows combining cells from different models where we may want to make different
assumptions.)

.. _ion_diffusion_bio_faq:
How do I make cytosolic concentrations diffuse and respond to ion channel activity?
-----------------------------------------------------------------------------------

We modify the above example by specifying a diffusion constant ``d`` e.g.

.. code::
    python

    from neuron.units import um, ms
    ca = rxd.Species(cyt, name="ca", d=1.3 * um**2/ms, charge=2)

The units used here -- µm :superscript:`2` / ms -- are the default and would be assumed if not
specified, but it is generally good practice to include units. We note that the ``neuron.units``
module provides both ``µm`` and ``um``; these are synonyms with the latter made available to
facilitate typing.

