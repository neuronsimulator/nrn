neuron.rxd
==========

.. currentmodule:: neuron.rxd
.. autoclass:: Region
    :members: nrn_region, secs

----

.. autoclass:: Species
   :members: __init__, indices, charge, name, nodes


----


.. autoclass:: Reaction
    :members: __init__

----

.. autoclass:: Rate

----

.. autoclass:: MultiCompartmentReaction

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


