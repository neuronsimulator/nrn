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

        In NEURON 7.4+, secs is optional at initial region declaration, but it
        must be specified before the reaction-diffusion model is instantiated.
        
        Here:

        * ``secs`` is any Python iterable of sections (e.g. ``soma.wholetree()`` or ``[soma, apical, basal]`` or ``h.allsec()``)
        * ``nrn_region`` specifies the classic NEURON region associated with this object and must be either ``"i"`` for the region just inside the plasma membrane, ``"o"`` for the region just outside the plasma membrane or ``None`` for none of the above.
        * ``name`` is the name of the region (e.g. ``cyt`` or ``er``); this has no effect on the simulation results but it is helpful for debugging
        * ``dx`` deprecated; do not use
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
        
        The sections may be expressed as a NEURON SectionList or as any Python
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

        An :class:`rxd.NodeList` of all the nodes corresponding to the species.

        This can then be further restricted using the callable property of NodeList objects.

        Example: All nodes from ``sp`` on the Section ``dend``:

            .. code::
                python

                nodes_on_dend = sp.nodes(dend)

        Example: All nodes from ``sp`` on the Segment ``dend(0.5)``:

            .. code::
                python

                nodes_on_dend_center = sp.nodes(dend(0.5))
            
            For 1D simulation with a species defined on 1 region, this will be a :class:`rxd.NodeList` of length 1 (or 0 if the species is not defined on the segment); for 3D simulation or with multiple regions, the list may be longer and further filtering may be required.
        
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
            * ``forward_rate`` and ``backward_rate`` represent the reaction rates; reactions are assumed to be governed by mass-action kinetics with these as the rate constants unless ``custom_dynamics`` is true, in which case these are expressions fully defining the rate of change.
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

A :class:`rxd.Node` represents a particular state value or :class:`rxd.Parameter` in a particular location. Individual :class:`rxd.Node` objects are typically obtained either from being passed to an initialization function or by filtering or selecting from an :class:`rxd.NodeList` returned by :attr:`rxd.Species.nodes`. Node objects are often used for recording concentration using :attr:`rxd.Node._ref_concentration`.

.. currentmodule:: neuron.rxd.node
.. autoclass:: Node
    :members: __init__, _ref_concentration, satisfies, volume, surface_area, x, d, region, sec, species, concentration


.. currentmodule:: neuron.rxd.nodelist
.. autoclass:: NodeList
    :members: __init__, __call__, concentration, diff, volume, surface_area, region, species, x
    
Error handling
--------------

Most errors in the usage of the ``neuron.rxd`` module should
raise a :class:`rxd.RxDException`.

.. class:: rxd.RxDException

    An exception originating from the ``neuron.rxd`` module
    due to invalid usage. This allows distinguishing such
    exceptions from other errors.

    The text message of an :class:`rxd.RxDException` ``e`` may be read as ``str(e)``.