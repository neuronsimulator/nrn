.. _current-clamp-event-pulse:

I want a current clamp that will generate a pulse when I send it an event, or that I can use to produce pulses at precalculated times.
-------------------------------------------------------------------------------------------------------------

Then get `pulsedistrib.zip <http://www.neuron.yale.edu/neuron/static/docs/repstim/pulsedistrib.zip>`_, and unzip it. Inside the pulsedistrib subdirectory you'll find :download:`data/ipulse3.mod`, :download:`data/ipulse3rig.ses`, and :download:`data/test_3.hoc` (and some other files that pertain to the previous question). :download:`data/ipulse3.mod` contains the NMODL code for a current clamp that produces a current pulse when it receives an input event. :download:`data/test_3.hoc` is a simple demo of the Ipulse3 mechanism, and :download:`data/ipulse3rig.ses` is used by :download:`data/test_3.hoc` to create the GUI for a demo of Ipulse3. It uses a :hoc:class:`NetStim` to generate the events that drive the Ipulse3. If you want to drive an Ipulse3 with recorded or precomputed event times, use the VecStim class as described under the topic `Driving a synapse with recorded or precomputed spike events <https://www.neuron.yale.edu/phpBB/viewtopic.php?f=28&t=2117>`_ in the "Hot tips" area of the `NEURON Forum <https://www.neuron.yale.edu/phpBB/>`_.

