
.. _hoc_funfit:


FunctionFitter
--------------

A widget for graphing a parameterized function while exploring parameter 
variations. Also can adjust the parameters automatically to fit data. 
Function, parameters, independent variable can be specified by the user. 
This widget may be saved in a session. 
 
The FunctionFitter starts out with entries for graphing a single exponential 
with two parameters.  When a parameter value is changed the graph is replotted. 

Plot
~~~~

Replots function with current arg values 

Steps
~~~~~

Number of values of independent variable used in plot 

IndependentVar
~~~~~~~~~~~~~~

Enter the name of independent variable in a 
string dialog.

Args
~~~~

Enter space separated names of parameters in a string dialog. 
Default value editors for these parameters will appear in the rightmost 
box. 

Yexpr
~~~~~

Enter an expression involving the independent variable and the 
parameters (args). Any valid top level hoc expression is acceptable. 

PraxisFit
~~~~~~~~~

Starts fitting the function to the data with respect to the DataWeights 
by adjusting the checked argument values. See :hoc:func:`fit_praxis` .

StopAtNextQuadForm
~~~~~~~~~~~~~~~~~~

Stop the praxis fitter when it finishes its current/next cycle. 
At this point it contains a computation of the quadratic form of the parameter 
phase space (printed in the terminal window). 

Running
~~~~~~~

Checked when the praxis fitter is executing. 

WatchTheFit
~~~~~~~~~~~

Plot the function on each call to the error function during fitting. 
Things are slower if this box is checked. 

RoughFit
~~~~~~~~

Instead of fitting all the data according to the data weights, use 
only 5 equally spaced points in each of the two central data regions. 
Things can be much faster if this box is checked. 

ArgValues
~~~~~~~~~

The values of the arguments are used in the plot. When a value 
is changed the function is replotted. When fit data is present, a 
checkbox is added to the left of each argument button. When the box is checked 
then the fitter adjusts the value for a best fit. When not checked the parameter 
is treated as a constant. 

CurrentValuesAsDefault
======================

Resets the default values of the parameter field editors to their current values. 
 

FittoData
~~~~~~~~~


ReadDataFile
============

Get data from a file. The format is the number of data points followed 
by pairs of x,y data. 

CommonFunctionalForms
=====================


FitCriterion
============

not implemented 

ParameterRangeLimits
====================

Pops up a panel of parameters with their range limits. When 
the fitter calls the error function and one of the parameters is 
outside its range the error function will return a value of 1e6. 

DataWeights
===========

Pops up a panel of data weight intervals and weights. The first interval 
ranges from the beginning of the data to the interval 1 endpoint. From 
the interval3 endpoint to the end of the data, the weight is 0. The entire 
interval is given the weight indicated. Intervals can be manipulated directly 
by the :guilabel:`AdjustWeightRegions` tool of the :guilabel:`Graph` menu. 

SaveRestoreFunction
===================

Arg values and the y-expression can be saved in a list and restored by selection 
with a browser. 
 

