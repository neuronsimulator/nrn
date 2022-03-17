
.. _hoc_neuron_rxd:

Basic Reaction-Diffusion
========================

Overview
--------
NEURON provides the ``rxd`` submodule to simplify and standardize the specification of
models incorporating reaction-diffusion dynamics, including ion accumulation. 
The interface is implemented using Python, however as long as Python is available to
NEURON, reaction-diffusion dynamics may be specified using HOC.

We can access the ``rxd`` module in HOC via:

.. code::
    hoc

    objref pyobj, h, rxd

    {
        // load reaction-diffusion support and get convenient handles
        nrnpython("from neuron import h, rxd")
        pyobj = new PythonObject()
        rxd = pyobj.rxd
        h = pyobj.h
    }

The above additionally provides access to an object called ``h`` which is traditionally
how Python accesses core NEURON functionality (e.g. in Python one would use h. :class:`Vector`
instead of :hoc:class:`Vector`). You might not need to use h since when working in HOC,
but it does provide certain convenient functions like :func:`h.allsec`, which returns
an iterable of all sections usable with ``rxd`` without  having to explicitly construct
a :hoc:class:`SectionList`.

The main gotchas of using rxd in HOC is that (1) ``rxd`` in Python uses operator overloading to 
specify reactants and products; in HOC, one must use ``__add__``, etc instead.
(2) rxd in Python is usually used with keyword arguments; in HOC, everything must be 
specified using positional notation.

Here's a full working example that simulates a calcium buffering reaction: 
``Ca + Buf <> CaBuf``:

.. code::
    hoc

    objref pyobj, h, rxd, cyt, ca, buf, cabuf, buffering, g

    {
        // load reaction-diffusion support and get convenient handles
        nrnpython("from neuron import h, rxd")
        pyobj = new PythonObject()
        rxd = pyobj.rxd
        h = pyobj.h
    }

    {
        // define the domain and the dynamics
        create soma
        
        cyt = rxd.Region(h.allsec(), "i")
        ca = rxd.Species(cyt, 0, "ca", 2, 1)
        buf = rxd.Species(cyt, 0, "buf", 0, 1)
        cabuf = rxd.Species(cyt, 0, "cabuf", 0, 0)

        buffering = rxd.Reaction(ca.__add__(buf), cabuf, 1, 0.1)
    }

    {
        // if launched with nrniv, we need this to get graph to update automatically
        // and to use run()
        load_file("stdrun.hoc")
    }

    {
        // define the graph
        g = new Graph()
        g.addvar("ca", &soma.cai(0.5), 1, 1)
        g.addvar("cabuf", &soma.cabufi(0.5), 2, 1)
        g.size(0, 10, 0, 1)
        graphList[0].append(g)
    }

    {
        // run the simulation
        tstop = 20
        run()
    }

In particular, note that instead of ``ca + buf`` one must write
``ca.__add__(buf)``.


In general, a reaction-diffusion model specification involves answering three conceptual questions:

1. :ref:`Where <hoc_rxd_where>` the dynamics are occurring (specified using an :hoc:class:`rxd.Region` or :hoc:class:`rxd.Extracellular`)
2. :ref:`Who <hoc_rxd_who>` is involved (specified using an :hoc:class:`rxd.Species` or :hoc:class:`rxd.State`)
3. :ref:`What <hoc_rxd_what>` the reactions are (specified using :hoc:class:`rxd.Reaction`, :hoc:class:`rxd.Rate`, or :hoc:class:`rxd.MultiCompartmentReaction`)

Another key class is :hoc:class:`rxd.Parameter` for defining spatially varying parameters.
Integration options may be specified using :hoc:func:`rxd.set_solve_type`.


.. _hoc_rxd_where:

Specifying the domain
---------------------

NEURON provides two main classes for defining the domain where a given set of reaction-diffusion rules
applies: :hoc:class:`rxd.Region` and :hoc:class:`rxd.Extracellular` for intra- and extracellular domains,
respectively. Once defined, they are generally interchangeable in the specification of the species involved,
the reactions, etc. The exact shape of intracellular regions may be specified using any of a number of 
geometries, but the default is to include the entire intracellular space.

Intracellular regions and regions in Frankenhauser-Hodgkin space
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. hoc:class:: rxd.Region

    Declares a conceptual intracellular region of the neuron.

    Syntax:

        .. code::
            hoc

            r = rxd.Region(secs, nrn_region, geometry, dimension, dx, name)

        In NEURON 7.4+, ``secs`` is optional at initial region declaration, but it
        must be specified before the reaction-diffusion model is instantiated.

        All arguments are optional, but all prior arguments must be specified.
        To use the default values for the prior arguments, specify their values as
        ``pyobj.None``.
        
        Here:

        * ``secs`` is a :hoc:class:`SectionList` or any Python iterable of sections (e.g. ``h.allsec()``)
        * ``nrn_region`` specifies the classic NEURON region associated with this object and must be either ``"i"`` for the region just inside the plasma membrane, ``"o"`` for the region just outside the plasma membrane or ``pyobj.None`` for none of the above.
        * ``name`` is the name of the region (e.g. ``cyt`` or ``er``); this has no effect on the simulation results but it is helpful for debugging
        * ``dx`` deprecated; when specifying ``name`` pass in ``pyobj.None`` here
        * ``dimension`` deprecated; when specifying ``name`` pass in ``pyobj.None`` here

    .. property:: rxd.Region.nrn_region

        Get or set the classic NEURON region associated with this object.
            
        There are three possible values:

            * ``'i'`` -- just inside the plasma membrane
            * ``'o'`` -- just outside the plasma membrane
            * ``pyobj.None`` -- none of the above
        
        *Setting requires NEURON 7.4+, and then only before the reaction-diffusion model is instantiated.*

    .. property:: rxd.Region.secs

        Get or set the sections associated with this region.
        
        The sections may be expressed as a :hoc:class:`SectionList` or as any Python
        iterable of sections.
        
        Note: The return value is a copy of the internal section list; modifying
              it will not change the Region.
        
        *Setting requires NEURON 7.4+ and allowed only before the reaction-diffusion model is instantiated.*

    .. property:: rxd.Region.geometry

        Get or set the geometry associated with this region.
        
        Setting the geometry to ``None`` will cause it to default to ``rxd.geometry.inside``.
        
        *Added in NEURON 7.4. Setting allowed only before the reaction-diffusion model is instantiated.*
    
    .. property:: rxd.Region.name

        Get or set the Region's name.

        *Added in NEURON 7.4.*

For specifying the geometry
###########################

NEURON provides several built-in geometries for intracellular simulation that may be specified
by passing a ``geometry`` argument to the :class:`rxd.Region` constructor. New region shapes may
be defined by deriving from ``neuron.rxd.geometry.RxDGeometry``.

See the Python programmer's reference for more on :attr:`rxd.inside`. :attr:`rxd.membrane`,
:class:`rxd.DistributedBoundary`, :class:`rxd.FractionalVolume`, :class:`rxd.Shell`,
:class:`rxd.FixedCrossSection`, :class:`rxd.FixedPerimeter`, and 
:class:`rxd.ScalableBorder`.

Extracellular regions
---------------------

.. hoc:class:: rxd.Extracellular

    Declare a extracellular region to be simulated in 3D; 
    unlike :hoc:class:`rxd.Region`, this *always* describes the extracellular region.

    See the entry for :class:`rxd.Extracellular` in the Python programmer's reference
    for more information.

.. _hoc_rxd_who:

Defining proteins, ions, etc
----------------------------

Values that are distributed spatially on an :hoc:class:`rxd.Region` or :hoc:class:`rxd.Extracellular` may be defined using
an :hoc:class:`rxd.Species` if they represent things that change and diffuse, an :hoc:class:`rxd.State` if they're in fixed locations but changeable
(e.g. gates in an IP3R), or an :hoc:class:`rxd.Parameter` if
they are just fixed values.

.. hoc:class:: rxd.Species

    Declare an ion/protein/etc that can react and diffuse.

    See the entry for :class:`rxd.Species` in the Python programmer's reference
    for more information.

.. hoc:class:: rxd.State

    Like an :hoc:class:`rxd.Species` but indicates the semantics
    of something that is not intended to diffuse.
    
    See the entry for :class:`rxd.State` in the Python programmer's reference
    for more information.

.. hoc:class:: rxd.Parameter

    Declares a parameter, that can be used in place of a :hoc:class:`rxd.Species`, but unlike :hoc:class:`rxd.Species` a parameter will not change.

    See the entry for :class:`rxd.Parameter` in the Python programmer's reference
    for more information.


.. _hoc_rxd_what:

Defining reactions
------------------

NEURON provides three classes for specifying reaction dynamics: :hoc:class:`rxd.Reaction` for single-compartment (local)
reactions; :hoc:class:`rxd.MultiCompartmentReaction` for reactions spanning multiple compartments (e.g. a pump that 
moves calcium from the cytosol into the ER changes concentration in two regions), and :hoc:class:`rxd.Rate` for
specifying changes to a state variable directly by an expression to be added to a differential equation.
Developers may introduce new forms of reaction specification by subclassing
``neuron.rxd.generalizedReaction.GeneralizedReaction``, but this is not necessary for typical modeling usage.

It is sometimes necessary to build rate expressions including mathematical functions. To do so, use the
functions defined in ``neuron.rxd.rxdmath`` as listed :ref:`below <hoc_rxdmath_prog_ref>`.

.. hoc:class:: rxd.Reaction

    Reaction at a point. May be mass-action or defined via custom dynamics.

    Syntax:

        .. code::
            hoc

            r1 = rxd.Reaction(reactant_sum, product_sum, forward_rate, 
                              backward_rate, regions, custom_dynamics)      
    
    ``backward_rate``, ``regions``, and ``custom_dynamics`` are optional, but
    when used from HOC, all previous parameters must be specified. To specify
    that the dynamics should be custom (i.e. fully defined by the rates) without
    a ``backward_rate`` or restricting to specific regions, pass ``0`` for
    ``backward_rate`` and ``pyobj.None`` for ``regions``.

    Example:

        .. code::
            hoc

            // here: ca + buf <> cabuf, kf = 1, kb = 0.1
            buffering = rxd.Reaction(ca.__add__(buf), cabuf, 1, 0.1)
    
    Note the need to use ``__add__`` instead of ``+``. To avoid this cumbersome
    notation, consider defining the rate expression in Python via :hoc:func:`nrnpython`.
    That is, we could write

        .. code::
            hoc

            // here: ca + buf <> cabuf, kf = 1, kb = 0.1
            nrnpython("from neuron import h")
            nrnpython("ca_plus_buf = h.ca + h.buf")
            buffering = rxd.Reaction(pyobj.ca_plus_buf, cabuf, 1, 0.1)

    This is admittedly longer than the previous example, but it allows the creation
    of relatively complicated expressions for rate constants:

        .. code::
            hoc

            nrnpython("from neuron import h")
            nrnpython("kf = h.ca ** 2 / (h.ca ** 2 + (1e-3) ** 2)")
            // and then work with pyobj.kf

    For more, see the :class:`rxd.Reaction` entry in the Python Programmer's reference.

.. hoc:class:: rxd.Rate

    Declare a contribution to the rate of change of a species or other state variable.

    Syntax:

        .. code::
            hoc

            r = rxd.Rate(species, rate, regions, membrane_flux)
        
        ``regions`` and ``membrane_flux`` are optional, but if ``membrane_flux``
        is specified, then ``regions`` (the set of regions where the rate occurs)
        must also be specified. The default behavior is that the rate applies on
        all regions where all involved species are present; this region rule applies
        when ``regions`` is ommitted or ``pyobj.None``.

    Example:

        .. code::
            hoc

            constant_production = rxd.Rate(protein, k)

    If this was the only contribution to protein dynamics and there was no
    diffusion, the above would be equivalent to:

        .. code::

            dprotein/dt = k

    If there are multiple :hoc:class:`rxd.Rate` objects (or an 
    :hoc:class:`rxd.Reaction`, etc) acting on
    the same species, then their effects are summed.

.. hoc:class:: rxd.MultiCompartmentReaction

    Specify a reaction spanning multiple regions to be added to the system.
    Use this for, for example, pumps and channels, or interactions between
    species living in a volume (e.g. the cytosol) and species on a
    membrane (e.g. the plasma membrane).
    For each species/state/parameter, you must specify what region you are
    referring to, as it could be present in multiple regions. You must
    also specify a `membrane` or a `border` (these are treated as synonyms)
    that separates the regions involved in your reaction. This is necessary
    because the default behavior is to scale the reaction rate by the
    border area, as would be expected if one of the species involved is a
    pump that is binding to a species in the volume. If this is not the
    desired behavior, pass the argument ``0`` for the ``scale_by_area`` field.
    Pass the argument ``1`` for ``membrane_flux`` if the reaction produces a current across
    the plasma membrane that should affect the membrane potential.
    Unlike :hoc:class:`rxd.Reaction` objects, the base units for the rates are in terms of
    molecules per square micron per ms.

Mathematical functions for rate expressions
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

NEURON's ``neuron.rxd.rxdmath`` module provides a number of mathematical functions that
can be used to define reaction rates. These generally mirror the functions available
through Python's ``math`` module but support :hoc:class:`rxd.Species` objects.

To use any of these, first do:

    .. code::
        hoc

        objref pyobj, rxdmath

        {
            // load rxdmath
            nrnpython("from neuron.rxd import rxdmath")
            pyobj = new PythonObject()
            rxdmath = pyobj.rxdmath
        }

    For a full runnable example, see `this Python tutorial <../../../rxd-tutorials/thresholds.html>`_
    which as here uses the hyperbolic tangent as an approximate on/off switch for the reaction.

Full documentation on this submodule (under the Python programmer's reference, but the HOC interface is identical) is available :ref:`here <rxdmath_prog_ref>` or you may go
directly to the documentation for any of its specific functions:
:func:`rxdmath.acos`, :func:`rxdmath.acosh`, :func:`rxdmath.asin`, 
:func:`rxdmath.asinh`, :func:`rxdmath.atan`, :func:`rxdmath.atan2`,
:func:`rxdmath.ceil`, :func:`rxdmath.copysign`,
:func:`rxdmath.cos`, :func:`rxdmath.cosh`,
:func:`rxdmath.degrees`, :func:`rxdmath.erf`,
:func:`rxdmath.erfc`, :func:`rxdmath.exp`,
:func:`rxdmath.expm1`, :func:`rxdmath.fabs`,
:func:`rxdmath.factorial`, :func:`rxdmath.floor`,
:func:`rxdmath.fmod`, :func:`rxdmath.gamma`,
:func:`rxdmath.lgamma`, :func:`rxdmath.log`,
:func:`rxdmath.log10`, :func:`rxdmath.log1p`,
:func:`rxdmath.pow`, :func:`rxdmath.pow`,
:func:`rxdmath.sin`, :func:`rxdmath.sinh`,
:func:`rxdmath.sqrt`, :func:`rxdmath.tan`,
:func:`rxdmath.tanh`, :func:`rxdmath.trunc`,
:func:`rxdmath.vtrap`

Manipulating nodes
------------------

A :hoc:class:`rxd.node.Node` represents a particular state value or :class:`rxd.Parameter` in a particular location. Individual :hoc:class:`rxd.node.Node` objects are typically obtained either from being passed to an initialization function or by filtering or selecting from an :hoc:class:`rxd.nodelist.NodeList` returned by :attr:`rxd.Species.nodes`. Node objects are often used for recording concentration using :attr:`rxd.node.Node._ref_concentration`.

.. hoc:class:: rxd.nodelist.NodeList

    An :hoc:class:`rxd.nodelist.NodeList` is a subclass of a Python `list <https://docs.python.org/3/tutorial/datastructures.html#more-on-lists>`_
    containing :hoc:class:`rxd.node.Node` objects. It is not intended to be created directly in a model, but rather is returned by 
    :hoc:attr:`rxd.Species.nodes`.

    Standard Python list methods are supported, including ``.append(node)``, ``.extend(node_list)``, 
    ``.insert(i, node)``, ``.index(node)``. To access the item in the ith position (0-indexed) of a NodeList
    ``nl`` in HOC, use ``nl.__get__item(i)``. (In Python, one could say ``nl[i]``, but
    this notation is not supported by HOC.)

    A key added functionality is the ability to filter the
    nodes by rxd property (returning a new 
    :hoc:class:`rxd.nodelist.NodeList`). Any filter object supported
    by the ``.satifies`` method of the node types present in the
    :hoc:class:`rxd.nodelist.NodeList` may be passed in parentheses;
    e.g.

        To filter a ``new_node_list`` to only contain nodes
        present in the :hoc:class:`rxd.Region` ``er``:

            .. code::
                hoc

                just_er = new_node_list(er)
    
    See the entry for :class:`rxd.nodelist.NodeList` in the Python programmer's
    reference for more information.

Membrane potential
------------------

.. property:: rxd.v

    A special object representing the local membrane potential in a reaction-rate
    expression. This can be used with :hoc:class:`rxd.Rate` and 
    :hoc:class:`rxd.MultiCompartmentReaction` to build ion channel models as an alternative
    to using NMODL, NeuroML (and converting to NMODL via `jneuroml <https://github.com/NeuroML/jNeuroML>`_), the ChannelBuilder,
    or :hoc:class:`KSChan`.
    
    (If you want a numeric value for the current membrane potential at a
    segment ``sec(x)`` use ``sec.v(x)`` instead; this syntax is slightly different
    from the Python convention for accessing membrane potential.) 

Synchronization with segments
-----------------------------

Changes to :hoc:class:`rxd.Species` node concentrations are propagated to segment-level concentrations automatically no later
than the next time step. This is generally the right direction for information to flow, however NEURON also provides
a :hoc:func:`rxd.re_init` function to transfer data from segments to :hoc:class:`rxd.Species`.

.. hoc:function:: rxd.re_init

    Reinitialize all :hoc:class:`rxd.Species`, :hoc:class:`rxd.State`, and :hoc:class:`rxd.Parameter` from changes made
    to NEURON segment-level concentrations. This calls the corresponding :hoc:meth:`rxd.Species.re_init` methods.
    Note that reaction-diffusion models may contain concentration data at a finer-resolution than that of a
    segment (e.g. for models being simulated in 3D).

    Syntax:

        .. code::
            hoc

            rxd.re_init()


Numerical options
-----------------

.. hoc:function:: rxd.nthread

    Specify a number of threads to use for extracellular and 3D intracellular simulation. Currently has
    no effect on 1D reaction-diffusion models.

    Syntax:

        .. code::
            hoc

            rxd.nthread(num_threads)
    
    Example:

        To simulate using 4 threads:

        .. code::
            hoc

            rxd.nthread(4)

    Thread scaling performance is discussed in the NEURON
    `extracellular <https://doi.org/10.3389/fninf.2018.00041>`_ and
    `3D intracellular <https://doi.org/10.1101/2022.01.01.474683>`_ methods papers.

.. hoc:function:: rxd.set_solve_type

    Specify numerical discretization and solver options. Currently the main use is to indicate
    Sections where reaction-diffusion should be simulated in 3D.

    Syntax:

        .. code::
            hoc

            rxd.set_solve_type(domain, dimension)

        where:

            - ``domain`` -- a :hoc:class:`SectionList` or other iterable of sections. Pass ``pyobj.None`` to apply the specification to the entire model.
            - ``dimension`` -- 1 or 3
    
    This function may be called multiple times; the last setting for dimension for a given section will apply.
    Different sections may be simulated in different dimensions (a so-called hybrid model).

Error handling
--------------

Most errors in the usage of the ``rxd`` module should
raise a :hoc:class:`rxd.RxDException`.

.. hoc:class:: rxd.RxDException

    An exception originating from the ``rxd`` module
    due to invalid usage. This allows distinguishing such
    exceptions from other errors.
    HOC's support for Error Handling is limited, so it is generally difficult
    to get access to these objects inside HOC, but they might be passed to HOC
    via a function called in Python.

    The text message of an :hoc:class:`rxd.RxDException` ``e`` may be read in HOC as ``e.__str__()``.
