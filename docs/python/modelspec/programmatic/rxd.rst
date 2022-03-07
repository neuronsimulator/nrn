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

In general, a reaction-diffusion model specification involves answering three conceptual questions:

1. :ref:`Where <rxd_where>` the dynamics are occurring (specified using an :class:`rxd.Region` or :class:`rxd.ExtracellularRegion`)
2. :ref:`Who <rxd_who>` is involved (specified using an :class:`rxd.Species` or :class:`rxd.State`)
3. :ref:`What <rxd_what>` the reactions are (specified using :class:`rxd.Reaction`, :class:`rxd.Rate`, or :class:`rxd.MultiCompartmentReaction`)

Another key class is :class:`rxd.Parameter` for defining spatially varying parameters.
Integration options may be specified using :func:`rxd.set_solve_type`.

Related resources
~~~~~~~~~~~~~~~~~

See also our `reaction-diffusion tutorials <../../../rxd-tutorials/index.html>`_,
the discussion about :ref:`ion accumulation <ion_channel_accumulation_bio_faq>` and
:ref:`ion diffusion <ion_diffusion_bio_faq>`_, and the 2021 NetPyNE course
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

            * xlo, ..., zhi -- define the bounding box of the domain; it should typically contain the entire morphology... by default NEURON assumes reflective (Neumann) boundary conditions, so you may want the box to extend well beyond the cell morphology depending on your use case
            * dx -- voxel edge size in µm
            * tortuosity -- increase factor in path length due to obstacles, effective diffusion coefficient d/tortuosity^2; either a single value for the whole region or a Vector giving a value for each voxel. Default is 1 (no change).
            * volume_fraction -- the free fraction of extracellular space; a volume_fraction of 1 assumes no cells; lower values are probably warranted for most simulations

----

.. _rxd_who:

Defining proteins, ions, etc
----------------------------

.. autoclass:: neuron.rxd.Species
   :members: __init__, indices, charge, name, nodes


.. _rxd_what:
Defining reactions
------------------


.. autoclass:: neuron.rxd.Reaction
    :members: __init__

----

.. autoclass:: neuron.rxd.Rate
    :members: __init__

----

.. autoclass:: neuron.rxd.MultiCompartmentReaction

----

Other
-----

.. currentmodule:: neuron.rxd.node
.. autoclass:: Node
    :members: __init__, _ref_concentration, satisfies, volume, surface_area, x, d, region, sec, species, concentration

----

.. currentmodule:: neuron.rxd.generalizedReaction
.. autoclass:: GeneralizedReaction

----

.. currentmodule:: neuron.rxd.species
.. autoclass:: SpeciesOnRegion
    :members: __init__, indices, nodes, concentration
    
----

.. currentmodule:: neuron.rxd.section1d
.. autoclass:: Section1D
    :members: __init__, indices, name, region, nseg, nrn_region, species, section_orientation, L

----

.. currentmodule:: neuron.rxd.nodelist
.. autoclass:: NodeList
    :members: __init__, __call__, concentration, diff, volume, surface_area, region, species, x
    
----
