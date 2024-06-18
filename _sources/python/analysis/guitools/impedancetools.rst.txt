.. _attshape:

.. seealso: :ref:`impratio`, :ref:`impedance_impx`, :ref:`impedance_logavsx`, 


Impedance Tools
---------------

These tools plot various functions of :class:`Impedance` vs 
location and frequency. :ref:`impshape` is described below, but see also:

.. toctree:: :maxdepth: 1

    impratio.rst
    impedanx.rst

 
Each tool has three panels: The top 
panel contains button and menu controls specific to the tool. The 
middle panel displays a picture of the cell and marks the measurement 
and injection sites. The bottom panel displays the results of the 
impedance measurements. 
 
The redraw button in the top panel forces a re-calculation of the results. 
This is useful if a parameter or state changes in the underlying model 
of the cell. The :menuselection:`Extras --> MovieMode` menu item recalculated the results 
for each step during a :ref:`runcontrol_initrun`. When selected, 
the :menuselection:`Extras --> AutoScale` rescales 
the results panel after each calculation. 
 
The :guilabel:`Plot` menu specifies whether to plot log attenuation (default), 
input impedance, transfer impedance, or ``V(measure)/V(inject)``
Note: log attenuation is defined as ``ln( V(inject)/V(measure) )``
Input impedance is plotted with 
respect to the measurement site and the injection site is unused. 
     

.. _impshape:

ImpShape
~~~~~~~~

Tool for generating the neuromorphic rendering, a picture of the 
neuron in which distance is scaled to represent the log of 
voltage attenuation. In this transform, 
long distance represents large attenuation. 
 
The tool consists of three panels. The middle panel shows the normal 
unscaled shape of the neuron on which the user can select a measurement/injection 
site (default location is location 0 of the currently accessed section 
when the ImpShape object was created). The lower panel displays 
a neuromorphic rendering along with a unit attenuation scale bar. 
The upper control panel allows selection of the frequency at which to 
calculate the impedance and the direction of current flow, ``Vin/Vout``, 
with respect 
to the measurement/injection site. 
 
When :guilabel:`Vin` is selected one can imagine that 
for every point on the cell, that point is voltage clamped to 1 mV and 
the voltage at the measurement site (0 end of the selected section in 
the lower panel) is recorded. The shape of the neuron 
is then plotted so that the distance between the 
voltage clamp site and the measurement site is the natural 
log of the attenuation 
factor, ``ln( V(inject)/V(measure) )``
 
When :guilabel:`Vout` is selected, one can imagine that the selected position 
becomes voltage clamped to 1mV and the voltage at every other position 
on the cell is measured. 
 

