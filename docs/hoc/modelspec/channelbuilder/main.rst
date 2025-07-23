Using NEURON's Channel Builder
==============================

This collection of tutorials shows how to use NEURON's Channel Builder.
The Channel Builder is a GUI tool for
creating voltage- and ligand-gated channels whose state
transitions are described by kinetic schemes and/or
HH-style differential equations.
Channel gating can be deterministic or stochastic.

    **Deterministic gating** models are based on the (often tacit) assumption 
    of a very large number of channels, each having a very small conductance.
    This is also called the 
    "continuous system approximation to a large population of channels with discrete states."
    The result is a simulation 
    in which gating variables, ionic conductances and ionic currents 
    are continuous functions of time.
    The classical Hodgkin-Huxley model is a typical example of a deterministic gating model.

    **Stochastic gating** models assume random, independent state transitions of 
    countably many channels.
    Simulations show abrupt changes of gating variables, 
    ionic conductances, and ionic currents.

The look and feel of this native hoc implementation are very similar to
Robert Cannon's Java-based Catacomb Channel Builder, 
which has been available in NEURON for the past few years. 
Difficulties of installing the large Java runtime system 
have so far prevented the general use of Java for
critical tools that require 100% availability on all machines.

The present
implementation of the Channel Builder is not a complete
replacement for NMODL for several reasons (e.g. the
Channel Builder is restricted to channels and cannot
describe pumps or diffusion).
However, it is a very convenient way to add new active 
currents to NEURON.
A configured Channel Builder can export
a human-readable, 
plain text specification of the mechanism.
Channels constructed with this tool are available immediately 
to hoc and NEURON's GUI tools, 
like any other distributed mechanism or point processes.
However, they do not need to be compiled, 
and they execute slightly faster 
than equivalent mechanisms created with NMODL.

The Tutorials
-------------

`Creating a channel from an HH-style specification <hhstyle/outline.html>`_

Given an HH-style channel specification, we add a new voltage-gated 
current to NEURON.
This introduces material that will be helpful in the "kinetic scheme" tutorial.

`Creating a channel from a kinetic scheme specification <kinetic/outline.html>`_

Given a kinetic scheme channel specification, we add a new voltage-gated 
current to NEURON.

`Creating a channel with stochastic gating <stochastic/outline.html>`_

Given a Channel Builder that implements a deterministic channel 
specified by a kinetic scheme, 
we create a new one that implements stochastic gating.

----

*Copyright © 2008 by N.T. Carnevale and M.L. Hines, All Rights Reserved.*
