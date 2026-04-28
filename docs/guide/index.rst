Guides
======

These guides cover the core concepts you need to build and simulate models
with NEURON. Start with :doc:`what_is_neuron` for an overview, or jump to
:doc:`how_to_get_started` if you are ready to dive in.

.. grid:: 1 1 2 2
   :gutter: 3

   .. grid-item-card:: :octicon:`info` What is NEURON?
      :link: what_is_neuron
      :link-type: doc

      An overview of NEURON's capabilities, design philosophy, and
      advantages for computational neuroscience.

   .. grid-item-card:: :octicon:`rocket` How to Get Started
      :link: how_to_get_started
      :link-type: doc

      Installation, first steps, key resources, and pointers to
      tutorials and the discussion board.

   .. grid-item-card:: :octicon:`meter` Units in NEURON
      :link: units
      :link-type: doc

      Default units for time, voltage, current, length, and
      conductance --- plus how to use ``neuron.units``.

   .. grid-item-card:: :octicon:`question` FAQ
      :link: faq
      :link-type: doc

      Answers to frequently asked questions about running,
      troubleshooting, and configuring NEURON.

.. toctree::
   :maxdepth: 2
   :caption: Getting Started

   what_is_neuron
   how_to_get_started
   bio_faq
   units
   faq

.. toctree::
   :maxdepth: 2
   :caption: GUI Tools & Model Building

   saveses
   cellbuilder
   import3d
   network_builder_tutorials
   optimization

.. toctree::
   :maxdepth: 1
   :caption: Advanced Topics

   modelview_compact_display
   randomness
   porting_mechanisms_to_cpp