
.. _hoc_neuron_rxd:

Basic Reaction-Diffusion
========================

.. toctree::
    :maxdepth: 2
    
    geometry3d.rst


.. warning::

    These functions are experimental.
    Please contact the NEURON developers with issues and be sure to validate your results.

.. currentmodule:: neuron.rxd
.. autoclass:: Region
    :members: __init__, nrn_region, secs

----

.. autoclass:: Species
   :members: __init__, indices, charge, name, nodes


----


.. autoclass:: Reaction
    :members: __init__

----

.. autoclass:: Rate
    :members: __init__

----

.. autoclass:: MultiCompartmentReaction

----

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

.. currentmodule:: neuron.rxd.geometry
.. autoclass:: FixedCrossSection
    :members: __init__, volumes1d, surface_areas1d, neighbor_areas1d, is_volume, is_area, __call__
    
----
    
.. autoclass:: FractionalVolume
    :members: __init__, volumes1d, surface_areas1d, neighbor_areas1d, is_volume, is_area, __call__

----

.. autoclass:: FixedPerimeter
    :members: __init__, volumes1d, surface_areas1d, neighbor_areas1d, is_volume, is_area, __call__

----

.. autoclass:: Shell
    :members: __init__, volumes1d, surface_areas1d, neighbor_areas1d, is_volume, is_area, __call__
