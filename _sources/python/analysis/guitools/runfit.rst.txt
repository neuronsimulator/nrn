.. _runfit:


RunFitter
---------

A tool for fitting the output of a simulation to data. 
Simulation output variable(s), and parameters can be specified by the user. 
This tool may be saved in a session. This tool uses the :func:`fit_praxis` 
method and the least squares error function calls run(). 
 
The minimization function used by the fitter calls the hoc "run" 
procedure (see :ref:`runcontrol_initrun`). 
The output simulation variable data is stored using the 
:meth:`Vector.record` function, ie values are copied from the variable to the 
vector at the end of :func:`finitialize` and at the end of :func:`fadvance` whenever 
t passes the x values of the data.  Fitting parameters are varied using 
one element vectors registered with :meth:`Vector.play`, ie the value is 
copied from the vector to the parameter at the beginning of 
:func:`finitialize`. The notion of a fitting parameter has been extended so 
that one can call an arbitrary statement so that the fit value can 
be used to assign values to a collection of hoc variables. Eg. 
globally setting a range variable. 
 
Any number of data weight regions can be specified in order to ignore 
artifacts weight critical regions more heavily. 
 
Fitting parameters and output parameters are 
registered with the play/record lists only when the "Running" checkbox shows 
that the widget is executing. 
     

.. warning::
    Multiple instances of the RunFitter widget can be present. But make sure 
    you are not Running more than one at a time. When saving a session involving 
    it is necessary that on retrieval the necessary variables exist that 
    are used by the fitter. In the case of extra fit variables this means 
    that the master fitter should be selected prior to the slave fitters 
    when using the :ref:`PWM`.
     
    When a parameter is very close to 0, its limited resolution in 
    a field editor may cause problems. In this case define 
    the parameter to be a scaled version of the actual desired value, eg 

    .. code-block::
        none

        g_pas = .0001*$1 

     
    Only change morphology parameters such as diam and L using a statement 
    involving $1. Otherwise the system will not be notified that diameter 
    is changing. 

     

ReadData
~~~~~~~~

Pops up a filechooser for reading the data file. The first number in the file is 
the number of points. Subsequent pairs of numbers are x and y values of 
the data. Alternativly the Graph menu can be used to invoke the 
:guilabel:`DataFromClipboard` item (see :ref:`stdrun_retrievefromfile`).
 
When data is read from the clipboard, that data is saved when the 
tool is saved in a session. However, if the :guilabel:`ReadData` button is used 
the filename is saved. 
     

CurrentValuesAsDefault
~~~~~~~~~~~~~~~~~~~~~~

Any checkmarks are removed from the default field editors in the 
parameter list. 

Variabletofit
~~~~~~~~~~~~~

Pops up symbol chooser. The syntax of the variable must be in a form which 
is a valid argument to a Vector.record(var) function. Practically speaking, 
this means that if the variable happens to be a density range variable then the 
entry string must contain an explicit section arc length parameter. eg. 
soma.v(.5) . Point process variables can use either an objref prefix or the 
internal object name, eg SEClamp[0].i . Navigating to a variable name 
with the chooser generally yields a valid name. If more than one variable 
is to be fit to separate data curves, invoke a slave RunFitter with 
from the Extras/AnotherVariableToFit menu item. 
     

Parameterstovary
~~~~~~~~~~~~~~~~

Every time this button is pressed it pops up a symbol chooser for appending 
a variable to the list of parameters to be varied in order to least 
squares fit the "Variabletofit" to the data. DefaultFieldEditor's for these 
parameters appear in the top right box of the widget. These parameters must 
be in a form acceptable to the Vector.play(var) function. ie density 
range variables must contain an explicit arc length parameter. 
 
In the case of a simulation consisting of more than one compartment, it 
is often necessary to identify a parameter with a set of values. In that 
case one can enter an arbitrary statement involving the parameter "$1", eg 

.. code-block::
    none

    forall g_pas = $1 

 
.. note::

    This field requires HOC not Python expressions.

Each parameter has a checkbox to the left of its name. When checked, 
the value will be adjusted during a fit to optimize the model to the 
data. If not checked the parameter will be held constant during the 
fit. 
 

Extras
~~~~~~


DataWeights
===========

Pops up a panel showing the boundaries and weight values for each 
data region. The boundaries can also be manipulated by selecting the 
AdjustWeightRegions tool from the :guilabel:`Graph` menu (right button) and then 
dragging the boundary lines. Weights are defined so that data points 
a small region will have a total weight equal to the data points in 
a large region when the interval weight values are the same. 

ParameterRanges
===============

Allows specification of the allowable parameter range for a fitting parameter. 
If praxis uses a parameter outside this range, the least squares error function 
will return 1e6 without calling the run procedure. Default parameter ranges 
for all fitting parameters are initialized to 1e-6 to 1e6 
 

Changeparmfromlist
==================

Pops up a browser with all parameter names. Double clicking on a name 
will pop up a string dialog which can be used to change the parameter 
name or statement. 

Removeparmfromlist
==================

Pops up a browser with all parameter names. Double clicking on a name 
will remove that parameter from the list. 

SaveRestoreFitParams
====================

The SaveFitParms menu item 
saves the current values of parameters, parameter range limits, and 
and whether the parameter is to be held constant during a fit. 
 
The SaveFitBrowser menu item pops up a list browser. Double clicking 
on these items will copy the saved parameters etc, back into the current 
parameter panel. 
 

NumberOfDataRegions
===================

Select the number of data regions to use in weighting the data. 
 

AnotherVariableToFit
====================

Pop up a slave RunFitter to allow simultaneous fitting of several 
sets of data to several fit variables. A RunFitter Slave does not 
have a parameter panel but has independent selection of data, 
variable to fit, and data weight regions. 
 
When saving a RunFitter Slave to a session, it must be placed on the 
paper icon of the PrintWindowManager AFTER its master. 
 

Dofit
~~~~~

Calls praxis to do the fit. During a fit, intermediate results are 
occasionally printed to the xterm window showing the progress of the 
fit. While the widget is working the :guilabel:`Running` checkbox is checked. 
If the StopatnextQuadForm button is pressed while the fit is running, 
the fit will stop at at the end of its current main loop returning its 
current best fit along with a print of the principal axes and principal 
values. Left alone, praxis will return when it is within 1e-5 of the local 
minimum. If :guilabel:`Dofit` is pressed while the :guilabel:`StopatnextQuadForm` is checked 
praxis will stop after one main loop (calculate principal axes and values) 
 

SingleRunFit
~~~~~~~~~~~~

Call the least squares error function once. This results in a single 
simulation run with the parameter values displayed in the panel. The 
"Error of fit" field editor shows the square norm of the data - outputvariable. 
 

StopatnextQuadForm
~~~~~~~~~~~~~~~~~~

Cause praxis to stop after it finishes its current principal axis/value 
computation. Be patient, it may be necessary to wait for several runs 
before the computation completes. To immediately quit, press 
the :ref:`runctrl_stop` button on the RunControl. This will stop the fit immediately 
and set parameters to the best fit found so far. 
Only do a :kbd:`^C` if necessity demands and then 
remove the check by pressing :guilabel:`SingleRunFit`. 
 

Running
~~~~~~~

Checked when in the process of doing a Dofit or SingleRunFit. When checked 
one should not try to change the widget by changing parameters or doing 
a recursive run. The check may not be accurate if the previous run generated 
a runtime error since the check is removed only if the call to praxis 
returns normally. In this case one may press the SingleRunFit button and 
follow the instructions to remove the check. 
 

Roughfit
~~~~~~~~

Uses just 4 points per weight region to fit the data. This can 
allow the variable time step method to run much faster since there are 
many fewer recording events requested. 
 

BeQuiet
~~~~~~~

Turns off printing by the praxis function and does not flush 
the graphs after :func:`run` is called. 
 

