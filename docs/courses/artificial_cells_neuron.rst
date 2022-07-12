.. _artificial_cells_neuron:

Artificial Cells in NEURON
==========================

In NEURON, artificial cells are point processes that serve as both a target and source object for a network connection object. 
That they are targets mean that they have a :class:`NetCon` ``NET_RECEIVE`` block which handles discrete event input streams through one or more NetCon objects. 
That they are also sources means that the :class:`NetCon` ``NET_RECEIVE`` block also generates discrete output events which are delivered through one or more NetCon objects. 
Generally, such cells are computationally very efficient (hundreds of times faster than cells we have simulated up to now whose voltage response is a consequence of membrane conductance) because their computation time does not depend on the integration step, dt, but only on the number of events. 
That is, handling 100000 spikes in one hour for 100 cells takes the same time as handling 100000 spikes in 1 second for 1 cell. 
The total computation time is proportional to the total number of spikes delivered during a run and is independent of the number of cells or number of connections or interval between spikes.

NEURON has four built-in point process classes which can be used to construct artificial cell types:

1.
    `NetStim <https://nrn.readthedocs.io/en/latest/python/modelspec/programmatic/mechanisms/mech.html?highlight=netstim#NetStim>`_ produces a user-specified train of one or more output events, and can also be triggered by input events

2.
    :hoc:class:`IntFire1`, which acts like a leaky integrator driven by delta function inputs. That is, the state variable m decays exponentially toward 0. 
    Arrival of an event with weight w causes an abrupt change in m. 
    If m exceeds 1, an output event is generated and the cell enters a refractory period during which it ignores further inputs. At the end of the refractory period, m is reset to 0 and the cell becomes responsive to new inputs.

3.
    :hoc:class:`IntFire2`, a leaky integrator with time constant taum driven by a total current that is the sum of

        { a user-settable constant "bias" current }
    
    plus

        { a net synaptic current }.

    Net synaptic current decays toward 0 with time constant taus, where taus > taum (synaptic current decays slowly compared to the rate at which "membrane potential" m equilibrates). When an input event with weight w arrives, the net synaptic current changes abruptly by the amount w.

4. :hoc:class:`IntFire4`, with fast excitation current (rises abruptly, decays exponentially) and slower alpha function like inhibition current that is integrated by even slower membrane.

NEURON requires that all point processes be located in a section. To meet this (in this context, conceptually irrelevant) requirement, the Network Builder tool locates each point process of its instantiated artificial cells in the dummy section called ``acell_home_``



