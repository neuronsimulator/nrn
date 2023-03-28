.. _neuron_rxd:

Basic Reaction-Diffusion
========================

Overview
--------
NEURON provides the ``rxd`` submodule to simplify and standardize the specification of
models incorporating reaction-diffusion dynamics, including ion accumulation. To load
this module, use:

.. code::
    python

    from neuron import rxd

Note: In older code, you may find ``from neuron import crxd as rxd`` but this is equivalent to the above as ``crxd`` has been an alias for ``rxd`` for several years.

In general, a reaction-diffusion model specification involves answering three conceptual questions:

1. :ref:`Where <rxd_where>` the dynamics are occurring (specified using an :class:`rxd.Region` or :class:`rxd.Extracellular`)
2. :ref:`Who <rxd_who>` is involved (specified using an :class:`rxd.Species` or :class:`rxd.State`)
3. :ref:`What <rxd_what>` the reactions are (specified using :class:`rxd.Reaction`, :class:`rxd.Rate`, or :class:`rxd.MultiCompartmentReaction`)

Another key class is :class:`rxd.Parameter` for defining spatially varying parameters.
Integration options may be specified using :func:`rxd.set_solve_type`.

Related resources
~~~~~~~~~~~~~~~~~

See also our `reaction-diffusion tutorials <../../../rxd-tutorials/index.html>`_,
the discussion about :ref:`ion accumulation <ion_channel_accumulation_bio_faq>` and
:ref:`ion diffusion <ion_diffusion_bio_faq>`, and the 2021 NetPyNE course
:ref:`lecture and exercise <netpyne_neuron_rxd_video>` videos on reaction-diffusion in NEURON.

.. _rxd_where:

Specifying the domain
---------------------

NEURON provides two main classes for defining the domain where a given set of reaction-diffusion rules
applies: :class:`rxd.Region` and :class:`rxd.Extracellular` for intra- and extracellular domains,
respectively. Once defined, they are generally interchangeable in the specification of the species involved,
the reactions, etc. The exact shape of intracellular regions may be specified using any of a number of 
geometries, but the default is to include the entire intracellular space.

Intracellular regions and regions in Frankenhauser-Hodgkin space
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. class:: rxd.Region

    Declares a conceptual intracellular region of the neuron.

    Syntax:

        .. code::
            python

            r = rxd.Region(secs=None, nrn_region=None, geometry=None, dimension=None, dx=None, name=None)

        In NEURON 7.4+, ``secs`` is optional at initial region declaration, but it
        must be specified before the reaction-diffusion model is instantiated.
        
        Here:

        * ``secs`` is any Python iterable of sections (e.g. ``soma.wholetree()`` or ``[soma, apical, basal]`` or ``h.allsec()``)
        * ``nrn_region`` specifies the classic NEURON region associated with this object and must be either ``"i"`` for the region just inside the plasma membrane, ``"o"`` for the region just outside the plasma membrane or ``None`` for none of the above.
        * ``name`` is the name of the region (e.g. ``cyt`` or ``er``); this has no effect on the simulation results but it is helpful for debugging
        * ``dx`` specifies the 3D voxel edge length. Models in NEURON 9.0+ allow multiple values of dx per model, as long as 3D sections with different ``dx`` values do not connect to each other. If this condition is not true, an exception is raised during simulation. (Prior to NEURON 9.0, behavior of models with multiple values of ``dx`` is undefined, and no error checking was provided.)
        * ``dimension`` deprecated; do not use

    .. property:: rxd.Region.nrn_region

        Get or set the classic NEURON region associated with this object.
            
        There are three possible values:

            * ``'i'`` -- just inside the plasma membrane
            * ``'o'`` -- just outside the plasma membrane
            * ``None`` -- none of the above
        
        *Setting requires NEURON 7.4+, and then only before the reaction-diffusion model is instantiated.*

    .. property:: rxd.Region.secs

        Get or set the sections associated with this region.
        
        The sections may be expressed as a NEURON :class:`SectionList` or as any Python
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

.. attribute:: rxd.inside

    The entire region inside the cytosol. This is the default. Use via e.g.

    .. code::
        python

        cyt = rxd.Region(h.allsec(), name="cyt", nrn_region="i", geometry=rxd.inside)

.. attribute:: rxd.membrane

    The entire plasma membrane.

    .. code::
        python

        cyt = rxd.Region(h.allsec(), name="cyt", nrn_region="i", geometry=rxd.membrane)


.. class:: rxd.DistributedBoundary

    Boundary that scales with area.
    
    .. code::
        python

        b = rxd.DistributedBoundary(area_per_vol, perim_per_area=0)
    
    area_per_vol is the area of the boundary as a function of the volume
    containing it. e.g.    
    
    ``g = rxd.DistributedBoundary(2)`` defines a geometry with 2 um^2 of area per
    every um^3 of volume.
    
    perim_per_area is the perimeter (in µm) per 1 µm^2 cross section of the
    volume. For use in reaction-diffusion problems, it may be safely omitted
    if and only if no species in the corresponding region diffuses.
    
    This is often useful for separating :class:`rxd.FractionalVolume` objects.
    
    It is assumed that the area is always strictly on the interior.

.. class:: rxd.FractionalVolume

    Defines a geometry occupying a set fraction of the cross-sectional area.
    e.g. perhaps the cytosol would occupy 0.83 of the cross-section (and all the surface)
    but the ER would only occupy 0.17 of the cross-section and none of the surface.

    Syntax:

        .. code::
            python

            r = rxd.FractionalVolume(volume_fraction=1, surface_fraction=0, neighbor_areas_fraction=None)

.. class:: rxd.Shell

    Defines a radial shell inside or outside of the plasma membrane. This is sometimes used to simulate
    a 2D-style diffusion where molecules move both longitudinally and into/out of the center of the dendrite.
    Consider using 3D simulation instead, which is better able to represent branch point dynamics.

    Syntax:

        .. code::
            python

            r = rxd.Shell(lo=None, hi=None)
    
    Example:

        See the `radial diffusion <https://neuron.yale.edu/neuron/docs/radial-diffusion>`_ tutorial.

.. class:: rxd.FixedCrossSection

    Syntax:

        .. code::
            python

            r = rxd.FixedCrossSection(cross_area, surface_area=0)
    
.. class:: rxd.FixedPerimeter

    Syntax:

        .. code::
            python

            r = rxd.FixedPerimeter(perimeter, on_cell_surface=False)
    
.. class:: rxd.ScalableBorder

    A membrane that scales proportionally with the diameter
    
    Example use:
    
    - the boundary between radial shells
    
    Sometimes useful for the boundary between :class:`rxd.FractionalVolume` objects, but
    see also :class:`rxd.DistributedBoundary` which scales with area.



Extracellular regions
---------------------

.. class:: rxd.Extracellular

    Declare a extracellular region to be simulated in 3D; 
    unlike :class:`rxd.Region`, this *always* describes the extracellular region.

    Syntax:

        .. code::
            python

            r = rxd.Extracellular(xlo, ylo, zlo, xhi, yhi, zhi, dx, 
                                  volume_fraction=1, tortuosity=None, permeability=None)

        Here:

            * ``xlo``, ..., ``zhi`` -- define the bounding box of the domain; it should typically contain the entire morphology... by default NEURON assumes reflective (Neumann) boundary conditions, so you may want the box to extend well beyond the cell morphology depending on your use case
            * ``dx`` -- voxel edge size in µm
            * ``tortuosity`` -- increase factor in path length due to obstacles, effective diffusion coefficient d/tortuosity^2; either a single value for the whole region or a Vector giving a value for each voxel. Default is 1 (no change).
            * ``volume_fraction`` -- the free fraction of extracellular space; a volume_fraction of 1 assumes no cells; lower values are probably warranted for most simulations
    
    Example:

        A tutorial demonstrating extracellular diffusion
        is available `here <../../../rxd-tutorials/extracellular.html>`_.

----

.. _rxd_who:

Defining proteins, ions, etc
----------------------------

Values that are distributed spatially on an :class:`rxd.Region` or :class:`rxd.Extracellular` may be defined using
an :class:`rxd.Species` if they represent things that change and diffuse, an :class:`rxd.State` if they're in fixed locations but changeable
(e.g. gates in an IP3R), or an :class:`rxd.Parameter` if
they are just fixed values.

.. class:: rxd.Species

    Declare an ion/protein/etc that can react and diffuse.

    Syntax:

        .. code::
            python

            s = rxd.Species(regions=None,
                            d=0,
                            name=None,
                            charge=0,
                            initial=None,
                            atolscale=1,
                            ecs_boundary_conditions=None,
                            represents=None
            )

        Parameters:

            * ``regions`` -- a Region or list of Region objects containing the species
            * ``d`` -- the diffusion constant of the species (optional; default is 0, i.e. non-diffusing)
            * ``name`` -- the name of the Species; used for syncing with NMODL and HOC (optional; default is none)
            * ``charge`` -- the charge of the Species (optional; default is 0)
            * ``initial`` -- the initial concentration or None (if None, then imports from HOC if the species is defined at finitialize, else 0); can be specified as a constant or as a function of an :class:`rxd.Node`
            * ``atolscale`` -- scale factor for absolute tolerance in variable step integrations
            * ``ecs_boundary_conditions`` -- if Extracellular rxd is used ecs_boundary_conditions=None for zero flux boundaries or if ecs_boundary_conditions=the concentration at the boundary.
            * ``represents`` -- optionally provide CURIE (Compact URI) to annotate what the species represents e.g. CHEBI:29101 for sodium(1+)

    .. note::

        Charge must match the charges specified in NMODL files for the same ion, if any. Common species charges include: sodium (+1), potassium (+1), calcium (+2), magnesium (+2), chloride (-1).

        You probably want to adjust atolscale for species present at low concentrations (e.g. calcium).

        NEURON does not require any specific ontology for identifiers, however CHEBI contains identifiers for many substances of interest in reaction-diffusion modeling. A number of ontology search providers are available on the internet, including `BioPortal <https://bioportal.bioontology.org/search>`_.

        To refer to a given Species restricted to a certain region, specify the region in square brackets. e.g. ``er_calcium = ca[er]``.
    
    .. property:: rxd.Species.nodes

        An :class:`rxd.nodelist.NodeList` of all the nodes corresponding to the species.

        This can then be further restricted using the callable property of NodeList objects.

        Example: All nodes from ``sp`` on the Section ``dend``:

            .. code::
                python

                nodes_on_dend = sp.nodes(dend)

        Example: All nodes from ``sp`` on the Segment ``dend(0.5)``:

            .. code::
                python

                nodes_on_dend_center = sp.nodes(dend(0.5))
            
            For 1D simulation with a species defined on 1 region, this will be a :class:`rxd.nodelist.NodeList` of length 1 (or 0 if the species is not defined on the segment); for 3D simulation or with multiple regions, the list may be longer and further filtering may be required.
        
        .. note::

            The code

            .. code::
                python

                node_list = ca[cyt].nodes
            
            is more efficient than the otherwise equivalent

            .. code::
                python

                node_list = ca.nodes(cyt)
            
            because the former only creates the :class:`rxd.Node` objects belonging to the restriction ``ca[cyt]`` whereas the second option constructs all :class:`rxd.Node` objects belonging to the :class:`rxd.Species` ``ca`` and then culls the list to only include those also belonging to the :class:`rxd.Region` cyt.

    .. property:: rxd.Species.states

        A list of a copy of the state values corresponding to this species; modifying the values in the list will not change the values used in the simulation.
    
    .. method:: rxd.Species.re_init

        Syntax:

            .. code::
                python

                sp.re_init()
            
            where ``sp`` is an instance of an :class:`rxd.Species`.

        Reinitialize the rxd concentration of this species to match the NEURON grid. Used when e.g. ``cai`` is modified directly instead of through a corresponding :class:`rxd.Species`.

        .. seealso:

            :func:`rxd.re_init`
    
    .. method:: rxd.Species.concentrations

        Deprecated. Do not use.
    
    .. property:: rxd.Species.charge

        Get or set the charge of the Species.

        Setting was added in NEURON 7.4 and is allowed only before the reaction-diffusion model is instantiated.
 
    .. property:: rxd.Species.regions

        Get or set the regions where the Species is present.

        Setting was added in NEURON 7.4 and is allowed only before the reaction-diffusion model is instantiated.
        Getting was added in NEURON 7.5.

    .. method:: rxd.Species.indices

        Return the indices corresponding to this species in the given region.

        This is occasionally useful, but it's generally best to avoid explicit indexing when developing models.

        Syntax:

            .. code::
                python

                sp.indices(r=None, secs=None)

        where ``sp`` is an instance of an :class:`rxd.Species`.

        If ``r`` is ``None``, then returns all species indices.
        If ``r`` is a list of regions return indices for only those sections that are on all the regions.
        If ``secs`` is a set return all indices on the regions for those sections.
    
    .. method:: rxd.Species.defined_on_region

        Returns ``True`` if the :class:`rxd.Species` ``sp`` is present on ``r``, else ``False``.

        Syntax:

            .. code::
                python

                result = sp.defined_on_region(r)
    
    .. property:: rxd.Species.name

        Get or set the name of the Species.

        This is used only for syncing with the non-reaction-diffusion parts of NEURON (NMODL, HOC).

        *Setting requires NEURON 7.4+, and then only before the reaction-diffusion model is instantiated.*

.. class:: rxd.State

    Like an :class:`rxd.Species` but indicates the semantics
    of something that is not intended to diffuse.

.. class:: rxd.Parameter

    Declares a parameter, that can be used in place of a :class:`rxd.Species`, but unlike :class:`rxd.Species` a parameter will not change.

    Syntax:

        .. code::
            python

            s = rxd.Parameter(regions, name=None, charge=0,
                              value=None, represents=None)



    Parameters:

        * ``regions`` -- a :class:`rxd.Region` or list of :class:`rxd.Region` objects containing the parameter
        * ``name`` -- the name of the parameter; used for syncing with NMODL and HOC (optional; default is none)
        * ``charge`` -- the charge of the Parameter (optional, probably only rarely needed; default is 0)
        * ``value`` -- the value or None (if None, then imports from HOC if the parameter is defined at :func:`finitialize`, else 0)
        * ``represents`` -- optionally provide CURIE (Compact URI) to annotate what the parameter represents e.g. CHEBI:29101 for sodium(1+)

    .. note::

        Charge must match the charges specified in NMODL files for the same ion, if any.

        Attempting to specify a non-zero diffusion rate for an :class:`rxd.Parameter` will raise an :class:`rxd.RxDException`.



.. _rxd_what:
Defining reactions
------------------

NEURON provides three classes for specifying reaction dynamics: :class:`rxd.Reaction` for single-compartment (local)
reactions; :class:`rxd.MultiCompartmentReaction` for reactions spanning multiple compartments (e.g. a pump that 
moves calcium from the cytosol into the ER changes concentration in two regions), and :class:`rxd.Rate` for
specifying changes to a state variable directly by an expression to be added to a differential equation.
Developers may introduce new forms of reaction specification by subclassing
``neuron.rxd.generalizedReaction.GeneralizedReaction``, but this is not necessary for typical modeling usage.

It is sometimes necessary to build rate expressions including mathematical functions. To do so, use the
functions defined in ``neuron.rxd.rxdmath`` as listed :ref:`below <rxdmath_prog_ref>`.

.. class:: rxd.Reaction

    Syntax:

        .. code::
            python

            r1 = rxd.Reaction(reactant_sum, product_sum, forward_rate, 
                              backward_rate=0, regions=region_list, custom_dynamics=False)  

        Here:

            * ``reactant_sum`` is a combination of :class:`rxd.Species`, :class:`rxd.State`, 
              :class:`rxd.Parameter`, e.g. ``ca + 2 * cl`` representing the left-hand-side of
              the reaction.
            * ``product_sum`` is like ``reactant_sum`` but represdenting the right-hand-side.
            * ``forward_rate`` and ``backward_rate`` represent the reaction rates; reactions are assumed to be governed by mass-action kinetics with these as the rate constants unless ``custom_dynamics`` is true, in which case these are expressions fully defining the rate of change. In particular, these can be constants or expressions combining :class:`rxd.Species` etc with constants, arithmetic, and :attr:`rxd.v`
            * ``region_list`` is a list of regions on which this reaction occurs. If ommitted or ``None``, the reaction occurs on all regions where all involved species are defined.

    Examples:

        For the following, we assume that ``H``, ``O``, ``H2O``, ``dimer``, and 
        ``decomposed`` are instances of :class:`rxd.Species`.

        For 2 * H + O > H2O in a mass action reaction at rate k:

            .. code::
                python

                r = rxd.Reaction(2 * H + O, H2O, k)
            
        To constrain the reaction to a specified list of regions,
        say to just the extracellular space (ext) and the cytosol (cyt),
        use the regions keyword, e.g.
            .. code::
                python

                r = rxd.Reaction(2 * H + O, H2O, k, regions=[ext, cyt])
        
        For a bi-directional reaction, specify a backward reaction rate.
        e.g. if kf is the forward rate and kb is the backward rate, then:
            .. code:: 
                python

                r = rxd.Reaction(2 * H + O, H2O, kf, kb)
        
        To use dynamics other than mass-action, add that mass_action=False
        flag and put the full formula instead of a mass-action rate for
        kf (and kb). E.g. Michaelis-Menten degradation

            .. code::
                python

                r = rxd.Reaction(
                    dimer, decomposed, dimer / (k + diamer), mass_action=False
                )
    
    .. property:: f_rate

        Get or set the forward reaction rate.
    
    .. property:: b_rate

        Get or set the backward reaction rate.


.. class:: rxd.Rate

    Declare a contribution to the rate of change of a species or other state variable.

    Syntax:

        .. code::
            python

            r = rxd.Rate(species, rate, regions=None, membrane_flux=False)
        
        If ``regions`` is ``None``, then the rate applies on all regions.

    Example:

        .. code::
            python

            constant_production = rxd.Rate(protein, k)

    If this was the only contribution to protein dynamics and there was no
    diffusion, the above would be equivalent to:

        .. code::

            dprotein/dt = k

    If there are multiple rxd.Rate objects (or an rxd.Reaction, etc) acting on
    the same species, then their effects are summed.


.. class:: rxd.MultiCompartmentReaction

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
    desired behavior, pass the keyword argument ``scale_by_area=False``.
    Pass in ``membrane_flux=True`` if the reaction produces a current across
    the plasma membrane that should affect the membrane potential.
    Unlike :class:`rxd.Reaction` objects, the base units for the rates are in terms of
    molecules per square micron per ms.



Mathematical functions for rate expressions
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

NEURON's ``neuron.rxd.rxdmath`` module provides a number of mathematical functions that
can be used to define reaction rates. These generally mirror the functions available
through Python's ``math`` module but support :class:`rxd.Species` objects.

To use any of these, first do:

    .. code::
        python

        from neuron.rxd import rxdmath

Example:

    .. code::
        python

        degradation_switch = (1 + rxdmath.tanh((ip3 - threshold) * 2 * m)) / 2
        degradation = rxd.Rate(ip3, -k * ip3 * degradation_switch)

    For a full runnable example, see `this tutorial <../../../rxd-tutorials/thresholds.html>`_
    which as here uses the hyperbolic tangent as an approximate on/off switch for the reaction.

Full documentation on this submodule is available :ref:`here <rxdmath_prog_ref>` or you may go
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


.. toctree::
    :hidden:

    rxdmath

Manipulating nodes
------------------

A :class:`rxd.node.Node` represents a particular state value or :class:`rxd.Parameter` in a particular location. Individual :class:`rxd.node.Node` objects are typically obtained either from being passed to an initialization function or by filtering or selecting from an :class:`rxd.nodelist.NodeList` returned by :attr:`rxd.Species.nodes`. Node objects are often used for recording concentration using :attr:`rxd.node.Node._ref_concentration`.

.. class:: rxd.nodelist.NodeList

    An :class:`rxd.nodelist.NodeList` is a subclass of a Python `list <https://docs.python.org/3/tutorial/datastructures.html#more-on-lists>`_
    containing :class:`rxd.node.Node` objects. It is not intended to be created directly in a model, but rather is returned by 
    :attr:`rxd.Species.nodes`.
    
    Standard Python list methods are supported, including ``.append(node)``, ``.extend(node_list)``, 
    ``.insert(i, node)``, ``.index(node)``, and manipulation of lists like ``len(node_list)``, ``node_list[0]``, or ``node_list[5:12]``. Additionally one may iterate over a NodeList as in:

        .. code::
            python

            for node in ca.nodes:
                ...
    
    (Here ``ca`` is assumed to be an :class:`rxd.Species` and thus ``ca.nodes`` is an
    :class:`rxd.nodelist.NodeList`.)

    A key added functionality is the ability to filter the
    nodes by rxd property (returning a new 
    :class:`rxd.nodelist.NodeList`). Any filter object supported
    by the ``.satifies`` method of the node types present in the
    :class:`rxd.nodelist.NodeList` may be passed in parentheses;
    e.g.

        To filter the :class:`rxd.Species` ``ca``'s nodes for
        just the ones present on the :class:`nrn.Segment`
        ``dend(0.5)``, use:

            .. code::
                python

                new_node_list = ca.nodes(dend(0.5))
        
        To filter the ``new_node_list`` to only contain nodes
        present in the :class:`rxd.Region` ``er``:

            .. code::
                python

                just_er = new_node_list(er)
        
    In addition, the following methods and properties are supported:

    .. property:: rxd.nodelist.NodeList.value

        Gets or sets the values associated with the stored nodes.
        Getting always returns a list, even if the :class:`rxd.nodelist.NodeList` has
        length 0 or 1. Setting may be to a constant (in which case all nodes are set to
        the same value) or to a list (in which case the list values are assigned in order
        to the nodes). In the latter case, if the length of the list does not match the length
        of the node list, an :class:`rxd.RxDException` is raised.

        The list that is returned by reading this property is a copy of the underlying data; 
        that is, changing it will have no effect on the values stored.

        This currently has the same behavior as :attr:`rxd.nodelist.NodeList.concentration`
        however in the future these are intended to be different for stochastic simulation.
    
    .. property:: rxd.nodelist.NodeList.concentration

        Gets or sets the concentration associated with the stored nodes.
        Getting always returns a list, even if the :class:`rxd.nodelist.NodeList` has
        length 0 or 1. Setting may be to a constant (in which case all nodes are set to
        the same value) or to a list (in which case the list values are assigned in order
        to the nodes). In the latter case, if the length of the list does not match the length
        of the node list, an :class:`rxd.RxDException` is raised.

        The list that is returned by reading this property is a copy of the underlying data; 
        that is, changing it will have no effect on the values stored.

        This currently has the same behavior as :attr:`rxd.nodelist.NodeList.value`
        however in the future these are intended to be different for stochastic simulation.
    
    .. property:: rxd.nodelist.NodeList.segment

        Returns a list of the :class:`nrn.Segment` objects associated with the nodes in the
        NodeList. 

        The list that is returned by reading this property is a copy of the underlying data; 
        that is, changing it will have no effect on the values stored.

    .. property:: rxd.nodelist.NodeList._ref_value

        A pointer to the memory location storing the :attr:`rxd.node.Node.value` when
        the NodeList has length 1; otherwise an :class:`rxd.RxDException` is raised.
    
    .. property:: rxd.nodelist.NodeList._ref_concentration

        A pointer to the memory location storing the :attr:`rxd.node.Node.concentration` when
        the NodeList has length 1; otherwise an :class:`rxd.RxDException` is raised.

    .. property:: rxd.nodelist.NodeList.diff

        Get or set the diffusion constants of the contained Node objects.
        Getting returns a list that is a copy of the underlying data. Setting accepts either
        a constant or a list of matching length; passing a list of a different length raises
        an :class:`rxd.RxDException`.

    .. property:: rxd.nodelist.NodeList.volume

        An iterable of the volumes of the Node objects in the NodeList.

        Read only.

    .. property:: rxd.nodelist.NodeList.surface_area

        An iterable of the surface areas of the Node objects in the NodeList.

        Read only.

    .. property:: rxd.nodelist.NodeList.region

        An iterable of the :class:`rxd.Region` (or :class:`rxd.Extracellular`) objects of the Node objects in the NodeList.

        Read only.


    .. property:: rxd.nodelist.NodeList.species

        An iterable of the :class:`rxd.Species` (or :class:`rxd.State` or :class:`rxd.Parameter`, as appropriate) objects of the Node objects in the NodeList.

        Read only.

    .. property:: rxd.nodelist.NodeList.x

        An iterable of the normalized positions of the Node objects in the NodeList.
        Note: these values are always between 0 and 1 and represent positions within
        the corresponding :class:`Section`. For 3D position, query the ``x3d`` property
        of the :class:`rxd.node.Node` objects themselves.

        Read only.

    .. method:: rxd.nodelist.NodeList.include_flux

        Includes the specified flux on all nodes in the NodeList. All arguments are passed
        directly to the underlying :class:`rxd.node.Node` objects.
    
    .. method:: rxd.nodelist.NodeList.value_to_grid

        Returns a regular grid with the values of the 3D nodes in the list. This is
        sometimes useful for volumetric visualization however the generated array
        may be large in certain models.

        The grid is a copy only.

        Grid points not belonging to the object are assigned a value of NaN.

        Nodes that are not 3d will be ignored. If there are no 3D nodes, returns
        a 0x0x0 numpy array.

        Warning: Currently only supports nodelists over 1 region.


.. class:: rxd.node.Node

    The combination of a single :class:`rxd.Species` etc and a unique spatial location
    at whatever resolution (i.e. could be a segment and a region, or could be a 3D voxel
    and a region).

    These objects are passed to an initialization function for rxd Species, States, and
    Parameters as ways of identifying a location.
    They are also useful for specifying localized fluxes or to record state variables.

    There are three subtypes: :class:`rxd.node.Node1D`, :class:`rxd.node.Node3D`, and
    :class:`rxd.node.NodeExtracellular`.
    They all support the methods and properties described here
    as well as some unique to their case features.

    .. method:: rxd.node.Node.include_flux

        Include a flux contribution to a specific node.

        The flux can be described as a NEURON reference, a point process and a
        property, a Python function, or something that evaluates to a constant
        Python float.

        Supported units: molecule/ms, mol/ms, mmol/ms == millimol/ms == mol/s

        Examples:

            .. code::
                python

                node.include_flux(mglur, 'ip3flux')           # default units: molecule/ms
                node.include_flux(mglur, 'ip3flux', units='mol/ms') # units: moles/ms
                node.include_flux(mglur._ref_ip3flux, units='molecule/ms')
                node.include_flux(lambda: mglur.ip3flux)
                node.include_flux(lambda: math.sin(h.t))
                node.include_flux(47)

        Warning:

            Flux denotes a change in *mass* not a change in concentration.
            For example, a metabotropic synapse produces a certain amount of
            substance when activated. The corresponding effect on the node's
            concentration depends on the volume of the node. (This scaling is
            handled automatically by NEURON's rxd module.)

    .. method:: rxd.node.Node.satisfies

        Tests if a Node satisfies a given condition.

        Syntax:

            .. code::
                python

                result = node.satisfies(condition)

        If a :class:`Section` object or RxDSection is provided, returns ``True`` if the Node lies in the section; else ``False``.

        If a :class:`rxd.Region` object is provided, returns ``True`` if the Node lies in the Region; else ``False``.

        Additional options are supported by specific subclasses, see
        :meth:`rxd.node.Node1D.satisfies`, :meth:`rxd.node.Node3D.satisfies`, and
        :meth:`rxd.node.NodeExtracellular.satisfies`.
    
    .. property:: rxd.node.Node._ref_concentration

        Returns a NEURON reference to the Node's concentration.
        This result is typically passed to :meth:`Vector.record` to record the concentration
        changes at a location over time.

        (The node must store concentration data. Use :attr:`rxd.node.Node._ref_molecules` for nodes
        storing molecule counts.)
    
    .. property:: rxd.node.Node._ref_molecules

        Returns a NEURON reference to the Node's concentration

        (The node must store molecule counts. Use _ref_concentrations for nodes
        storing concentration.)

    .. property:: rxd.node.Node._ref_value

        Returns a NEURON reference to the Node's value. This method always works,
        regardless of if the node stores a concentration or not.
    
    .. property:: rxd.node.Node.d

        Get or set the diffusion rate within the compartment.
    
    .. property:: rxd.node.Node.concentration

        Get or set the concentration at the Node.
    
        Currently does not support nodes storing molecule counts. Use :attr:`rxd.node.Node.molecules` instead; attempting to use with a molecule count node will raise
        an :class:`rxd.RxDException`.
    
    .. property:: rxd.node.Node.molecules

        Get or set the number of molecules at the Node.

        Currently does not support nodes storing concentrations. Use :attr:`rxd.node.Node.concentration` instead; attempting to use with a concentration node will raise
        an :class:`rxd.RxDException`.

    .. property:: rxd.node.Node.value

        Get or set the value associated with this Node.

        For Species nodes belonging to a deterministic simulation, this is a concentration.
        For Species nodes belonging to a stochastic simulation, this is the molecule count.
    
    .. property:: rxd.node.Node.x3d

        The 3D x-coordinate of the center of this Node.

    .. property:: rxd.node.Node.y3d

        The 3D y-coordinate of the center of this Node.

    .. property:: rxd.node.Node.z3d

        The 3D z-coordinate of the center of this Node.
    
    .. property:: rxd.node.Node.region

        The :class:`rxd.Region` or :class:`rxd.Extracellular` containing the compartment.
        Read only.

    .. property:: rxd.node.Node.species

        The :class:`rxd.Species`, :class:`rxd.State`, or :class:`rxd.Parameter` containing the compartment. Read only.

    .. property:: rxd.node.Node.volume

        The volume of the region spanned by the Node.

.. class:: rxd.node.Node1D

    A subclass of :class:`rxd.node.Node` used only for nodes being simulated in 1D.

    .. method:: rxd.node.Node1D.satisfies

        Supports the options of :meth:`rxd.node.Node.satisfies` and:

        If a number between 0 and 1 is provided, returns ``True`` if the normalized position lies within the Node; else ``False``.
    
    .. property:: rxd.node.Node1D.sec

        The section containing the node. Read-only.
    
    .. property:: rxd.node.Node1D.segment

        The segment containing the node. Read-only.
    
    .. property:: rxd.node.Node1D.x

        The normalized position of the center of the node. Read-only.

    .. property:: rxd.node.Node1D.surface_area

        The surface area of the compartment in square microns.

        This is the area (if any) of the compartment that lies on the plasma membrane
        and therefore is the area used to determine the contribution of currents (e.g. ina) from
        mod files or :class:`KSChan` to the compartment's concentration.

        Read only.
    

.. class:: rxd.node.Node3D

    A subclass of :class:`rxd.node.Node` used only for intracellular nodes being simulated in 3D.

    .. method:: rxd.node.Node3D.satisfies

        Supports the options of :meth:`rxd.node.Node.satisfies` and:

        If a tuple is provided of length 3, return ``True`` if the Node contains the ``(x, y, z)`` point; else ``False``.

    .. property:: rxd.node.Node3D.sec

        The section containing the node. Read-only.
    
    .. property:: rxd.node.Node3D.segment

        The segment containing the node. Read-only.

    .. property:: rxd.node.Node3D.surface_area

        The surface area of the compartment in square microns.

        This is the area (if any) of the compartment that lies on the plasma membrane
        and therefore is the area used to determine the contribution of currents (e.g. ina) from
        mod files or :class:`KSChan` to the compartment's concentration.

        Read only.


.. class:: rxd.node.NodeExtracellular

    A subclass of :class:`rxd.node.Node` used only for extracellular nodes being simulated in 3D.

    .. method:: rxd.node.NodeExtracellular.satisfies

        Supports the options of :meth:`rxd.node.Node.satisfies` and:

        If a tuple is provided of length 3, return ``True`` if the Node contains the ``(x, y, z)`` point; else ``False``.



Membrane potential
------------------

.. property:: rxd.v

    A special object representing the local membrane potential in a reaction-rate
    expression. This can be used with :class:`rxd.Rate` and 
    :class:`rxd.MultiCompartmentReaction` to build ion channel models as an alternative
    to using NMODL, NeuroML (and converting to NMODL via `jneuroml <https://github.com/NeuroML/jNeuroML>`_), the ChannelBuilder,
    or :class:`KSChan`.
    
    (If you want a numeric value for the current membrane potential at a
    segment ``seg`` use ``seg.v`` instead.) 

    Example (adapted from the `Hodgkin Huxley via rxd <https://neuron.yale.edu/neuron/docs/hodgkin-huxley-using-rxd>`_ tutorial)

        .. code::
            python

            from neuron.rxd.rxdmath import vtrap, exp, log
            from neuron import rxd

            alpha = 0.01 * vtrap(-(rxd.v + 55.0), 10.0)
            beta = 0.125 * exp(-(rxd.v + 65.0)/80.0)
            ntau = 1.0/(alpha + beta)
            ninf = alpha/(alpha + beta)

            # ... define cyt, mem, sections ...

            ngate = rxd.State([cyt, mem], name='ngate', initial=0.24458654944007166)
            n_gate = rxd.Rate(ngate, (ninf - ngate)/ntau)


Synchronization with segments
-----------------------------

Changes to :class:`rxd.Species` node concentrations are propagated to segment-level concentrations automatically no later
than the next time step. This is generally the right direction for information to flow, however NEURON also provides
a :func:`rxd.re_init` function to transfer data from segments to :class:`rxd.Species`.

.. function:: rxd.re_init

    Reinitialize all :class:`rxd.Species`, :class:`rxd.State`, and :class:`rxd.Parameter` from changes made
    to NEURON segment-level concentrations. This calls the corresponding :meth:`rxd.Species.re_init` methods.
    Note that reaction-diffusion models may contain concentration data at a finer-resolution than that of a
    :class:`nrn.Segment` (e.g. for models being simulated in 3D).

    Syntax:

        .. code::
            python

            rxd.re_init()

Numerical options
-----------------

.. function:: rxd.nthread

    Specify a number of threads to use for extracellular and 3D intracellular simulation. Currently has
    no effect on 1D reaction-diffusion models.

    Syntax:

        .. code::
            python

            rxd.nthread(num_threads)
    
    Example:

        To simulate using 4 threads:

        .. code::
            python

            rxd.nthread(4)

    Thread scaling performance is discussed in the NEURON
    `extracellular <https://doi.org/10.3389/fninf.2018.00041>`_ and
    `3D intracellular <https://doi.org/10.1101/2022.01.01.474683>`_ methods papers.

.. function:: rxd.set_solve_type

    Specify numerical discretization and solver options. Currently the main use is to indicate
    Sections where reaction-diffusion should be simulated in 3D.

    Syntax:

        .. code::
            python

            rxd.set_solve_type(domain=None, dimension=None, dx=None, nsubseg=None, method=None)

        where:

            - ``domain`` -- a :class:`Section` or Python iterable of sections. If the domain is ``None`` or omitted, the specification will apply to the entire model.
            - ``dimension`` -- 1 or 3
            - ``dx`` -- not implemented; specify dx for 3D models when creating the :class:`rxd.Region`
            - ``nsubseg`` -- not implemented
            - ``method`` -- not implemented
    
    This function may be called multiple times; the last setting for any given field will be used.
    Different sections may be simulated in different dimensions (a so-called hybrid model).
    
    .. warning::
    
        For 3D reaction-diffusion simulations, we recommend upgrading to at least NEURON 8.1.
        
        (Calculation of 3D concentration changes from MOD file activity
        in NEURON 7.8.x and 8.0.x was underestimated due to an inconsistency in surface voxel
        partial volume calculations.)

SBML Export
-----------

.. function:: rxd.export.sbml

    Export dynamics at a segment to an SBML file.

    Syntax:

        .. code:: 
            python

            rxd.export.sbml(segment, filename=None, model_name=None, pretty=True)

    This does not currently support :class:`rxd.MultiCompartmentReaction`; attempting to export dynamics that
    involve such reactions will raise an :class:`rxd.RxDException`.

    .. note::

        ``rxd.export`` is not available simply via ``from neuron import rxd``; you must also:

        .. code::
            python

            import neuron.rxd.export
    
Saving and restoring state
--------------------------

Some simulations require a lengthy initialization before exploring various possible stimuli.
In these situations, it is often convenient to run the initialization once, save the state,
do an experiment, and revert back to the saved state.

Beginning in NEURON 8.1, reaction-diffusion states are included automatically when using
:class:`SaveState` which additionally saves many other model states.

If one wants to save and restore *only* reaction-diffusion states, this can be done via the following
functions:

.. function: rxd.save_state

    Return a bytestring representation of the current rxd state.

    Note: this is dependent on the order items were created.

    Syntax:

        .. code::
            python

            state_data = rxd.save_state()
        
    .. versionadded: 8.1


.. function: rxd.restore_state

    Restore rxd state from a bytestring.

    Note: this is dependent on the order items were created.

    Syntax:

        .. code::
            python

            rxd.restore_state(state_data)
        
    .. versionadded: 8.1

The reaction-diffusion state data returned by :func:`rxd.save_state` and expected by :func:`rxd.restore_state`
consists of 16 bytes of metadata (8 bytes for a version identifier and 8 bytes for the length of the remaining portion)
followed by gzip-compresssed state values. In particular, not every binary string of a given length is a valid
state vector, nor is every state vector for a given model necessarily the same length (as the compressability may
be different).


Error handling
--------------

Most errors in the usage of the ``neuron.rxd`` module should
raise a :class:`rxd.RxDException`.

.. class:: rxd.RxDException

    An exception originating from the ``neuron.rxd`` module
    due to invalid usage. This allows distinguishing such
    exceptions from other errors.

    The text message of an :class:`rxd.RxDException` ``e`` may be read as ``str(e)``.
