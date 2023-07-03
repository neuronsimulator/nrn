
.. _hoc_electrod:

Electrode
---------



.. hoc:class:: Electrode

         
    A current injection electrode inserted in the middle of the 
    current section which can be switched between current and voltage 
    clamp modes and can do simple voltage clamp families. 
     
    usage: :samp:`{section} {e} = new Electrode([{xplacement}, {yplacement}])` 
    e.stim and e.vc can used to set parameters programmatically. 
     
    Electrode can be saved in a .session file and is best used 
    anonymously so that it is dismissed and point processes deleted 
    when the graphic is dismissed. 
         

----



.. hoc:method:: Electrode.IClamp

    Switches the Electrode to single pulse current injection. Uses IClamp 
    point process. 
         

----



.. hoc:method:: Electrode.del

    Time (ms) of the onset of the current stimulus relative to t = 0. 

----



.. hoc:method:: Electrode.dur

    Duration (ms) of the current stimulus 

----



.. hoc:method:: Electrode.amp

    Amplitude (nA) of the current stimulus 
         

----



.. hoc:method:: Electrode.VClamp

    Switches the Electrode to two electrode voltage clamp. Uses :hoc:class:`VClamp` point
    process that allows up to three level changes. The clamp is set to be ideal. 
     

----



.. hoc:method:: Electrode.dur0

    Duration in milliseconds of each level change starting at t=0. Each level 
    is concatenated. At t = dur0+dur1+dur2 the clamp is switched off and 
    no longer injects current. 
         

----



.. hoc:method:: Electrode.amp0

    Amplitude in millivolts of each level. 
         

----



.. hoc:method:: Electrode.VClampigraph

    Creates a currentgraph displaying the voltage clamp current. This button 
    exists because, with the present implementation, it is generally not 
    possible to reference the Electrode object from hoc because the only reference 
    is held by a vbox which in turn is only referenced by this Electrode. In 
    this way, when the Electrode window is dismissed, the Electrode is 
    destroyed and the point processes are removed from the neuron. 
         

----



.. hoc:method:: Electrode.VClampFamily

    Several common families for voltage clamp experiments. One should bring 
    up a current graph (VClampigraph button in VClamp card) and select KeepLines 
    in the graph popup menu. Only one clamp parameter is changed and the other 
    duration and amplitude levels are given by the values set in the VClamp panel 
    See User HocCode Electrode varyamp for the how the levels are varied. 
         

----



.. hoc:method:: Electrode.Testlevel

    varies amp1 in 10 steps 

----



.. hoc:method:: Electrode.Holding

    varies amp0 in 10 steps. Initialization is carried out at the value of amp0 
    so it is equivalent to the holding potential. 
         

----



.. hoc:method:: Electrode.Returnlevel

    varies amp2 in 10 steps. 
         
         

----



.. hoc:method:: Electrode.Location

    Shows a Shape scene of the neuron with the Electrode location marked as 
    a blue dot. The electrode location can be changed by making sure the 
    Section item in the selection menu is selected (right mouse button) and 
    pressing the left mouse button at any point on the picture of the neuron. 
    The position of the electrode is also reflected in the varlabel in the panel 
    just above the Shape. 
         
         

