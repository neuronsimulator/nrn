.. _impratio:


ImpedanceRatio
--------------

Tool for plotting voltage attenuation or impedance at a specific measurement and 
injection site as a function of log frequency from 0.1 to 1000 Hz. 
 
The measurement site is selected by first selecting the :guilabel:`SelectMeasure` item 
of the :guilabel:`SelectLocation` menu and then selecting a location on the neuron 
in the middle panel. A red dot denotes the measurement location. 
The injection site is selected by via selection of the :guilabel:`SelectInject` menu item. 
Measurement and injection sites are indicated by red and blue dots respectively. 
The :guilabel:`SwapMeasure/Inject` menu item swaps the measurement and injection sites. 
 
The :guilabel:`Plot` menu specifies whether to plot log attenuation (default), 
input impedance, transfer impedance, or ``V(measure)/V(inject)``. 
Input impedance is plotted with 
respect to the measurement site and the injection site is unused. 
 
This tool uses the :class:`Impedance` 
class to calculate voltage attenuations. 
     

