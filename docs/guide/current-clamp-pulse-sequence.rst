.. _current-clamp-pulse-sequence:

I just want a current clamp that will deliver a sequence of current pulses at regular intervals. Vector play seems like overkill for this.
-------------------------------------------------------------------------------------------------------------

Right you are. Pick up `pulsedistrib.zip <http://www.neuron.yale.edu/neuron/static/docs/repstim/pulsedistrib.zip>`_, and unzip it into an empty directory. This creates a subdirectory called pulsedistrib, which contains :download:`data/Ipulse1.mod`, :download:`data/Ipulse2.mod`, :download:`data/readme.txt`, and :download:`data/test_1_and_2.hoc`. Read :download:`data/readme.txt`, compile the mod files, and then use NEURON to load :download:`data/test_1_and_2.hoc`, which is a simple demo of these two current pulse generators.

pulsedistrib also contains :download:`data/ipulse3.mod`, :download:`data/ipulse3rig.ses`, and :download:`data/test_3.hoc`, which address the next question in this list.

