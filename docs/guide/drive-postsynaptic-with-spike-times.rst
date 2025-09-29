.. _drive-postsynaptic-with-spike-times:

I have a set of recorded or calculated spike times. How can I use these to drive a postsynaptic mechanism?
-----------------------------------------------------------------------------------------------

Assuming that your synaptic mechanism has a ``NET_RECEIVE`` block, so that it is driven by events delivered by a :hoc:class:`NetCon`, I can think of two ways this might be done. Which one to use depends on how many calculated spike times you are dealing with.

If you only have a "few" spikes (up to a few dozen), you could just dump them into the spike queue at the onset of the simulation. Here's how: 

1.
    Create a Vector and load it with the times at which you want to activate the synaptic mechanism.

2.
    Then use an :ref:`finitialize_handler` that stuffs the spike times into the NetCon's event queue by calling the `NetCon class's event() method <https://nrn.readthedocs.io/en/latest/hoc/modelspec/programmatic/network/netcon.html>`_ during initialization.

    For example, if the Vector that holds the event times is syntimes, and the NetCon that drives the synaptic point process is nc, this would work:

For example, if the Vector that holds the event times is syntimes, and the NetCon that drives the synaptic point process is nc, this would work:

.. code::
    c++

    objref fih
    fih = new FInitializeHandler("loadqueue()")
    proc loadqueue() { local ii
        for ii=0,syntimes.size()-1 nc.event(syntimes.x[ii])
    }

Don't forget that these are treated as *delivery* times, i.e. the NetCon's delay will have no effect on the times of synaptic activation. If additional conduction latency is needed, you will have to incorporate it by adding the extra time to the elements of syntimes before the FInitializeHandler is called. 

If you have a lot of spikes then it's best to use an NMODL-defined artificial spiking cell that generates spike events at times that are stored in a Vector (which you fill with data before the simulation). For more information see `Driving a synapse with recorded or precomputed spike events <https://www.neuron.yale.edu/phpBB/viewtopic.php?f=28&t=2117>`_ in the "Hot tips" area of the `NEURON Forum <https://www.neuron.yale.edu/phpBB/>`_.

