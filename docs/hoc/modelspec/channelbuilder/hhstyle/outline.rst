Creating a channel from an HH-style specification
===================================================

Our goal is to implement a new voltage-gated macroscopic current 
whose properties are described by HH-style equations.

For this example, we will borrow the equations for the HH sodium current
because they are familiar and because we can easily check our new current against 
NEURON's built-in HH mechanism.

**The HH sodium current is given by**
    i\ :sub:`Na` = g\ :sub:`Na` (V - E\ :sub:`Na`)

**where**
    g\ :sub:`Na` = gbar\ :sub:`Na` m\ :sup:`3` h
    
    gbar\ :sub:`Na` = 0.12 S/cm\ :sup:`2`

**and the gating variables m and h are governed by first order differential equations of the form**
    x' = a\ :sub:`x` (1 - x) - b\ :sub:`x` x

**Specifically**
    a\ :sub:`m` = 0.1 (V + 40) / [1 - e\ :sup:`- (V + 40) / 10`]
    
    b\ :sub:`m` = 4 e\ :sup:`- (V + 65) / 18`
    
    a\ :sub:`h` = 0.07 e\ :sup:`- (V + 65) / 20`
    
    b\ :sub:`h` = 1 / [e\ :sup:`- (V + 35) / 10` + 1]
    
    with time in ms and all voltages in mV.

Here's the outline of how we'll proceed :

`Step 1. Bring up a Channel Builder <startchnlbld.html>`_

`Step 2. Specify the channel's basic properties <basicprop.html>`_

Step 3. Specify "channel gating," i.e. :

    `specify the states that control channel gating <hhstates.html>`_
    
    `specify transitions between kinetic scheme states <hhstates.html#transitions>`_
    
    `specify how voltage and ligands affect states and transitions <vdepend.html>`_

`Step 4. Test the channel <testhh.html>`_

----

`Go back to the main page ("Using the Channel Builder") <../main.html>`_
to work on a different tutorial.

----

*Copyright © 2004 by N.T. Carnevale and M.L. Hines, All Rights Reserved.*
