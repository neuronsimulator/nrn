.. _mulfit:


MulRunFitter
------------

.. index:: MulRunFitter

A tool for fitting to data 
the output of a complex simulation involving possibly 
many runs with different stimulus protocols each with several recordings. 

When saved in a session, e.g. :file:`fit.ses`, two companion files are 
created called :file:`fit.ses.ft1` and :file:`fit.ses.fd1`. The :file:`*.ft1` file contains 
a readable (and modifiable) definition of the protocols and parameters. 
The :file:`*.fd1` file contains a copy of the data and internal parameters of 
the panels. When the :file:`fit.ses` is retrieved, these two files are also 
read to rebuild the state of the MulRunFitter. 

Because it generally takes a long time and a lot of effort to 
enter the information necessary to specify a complete multiple run 
fit strategy, it is a good idea to often save the partially built 
fitter in a session so that if something goes wrong you can always 
exit and start up again at a safe point. 

The usage of the MulRunFitter is expected to undergo considerable 
refinement in the future. 

Pressing the :guilabel:`ErrorValue` button will execute the "Generator" protocols 
and show the total error between simulation and data at this particular 
point in parameter space. 
 
Parameters Menu
~~~~~~~~~~~~~~~
 
The :guilabel:`Parameters` menu is used primarily to add, remove, and change 
parameter names or expressions. Parameter names can be any variable 
name or any statement involving $1. Statements involving $1 are 
extremely useful in causing a single parameter to modify many 
variables. e.g. 
``forall Ra = $1`` or ``some_procedure($1)`` 
Even if the parameter is a single variable name, it is most often 
more useful to express it as a "normalized expression" so that all 
"optimized" values are close to unity. e.g. ``cai0_ca_ion = 1e-4*$1`` 
The Praxis fitter does not work efficiently (or at all) when parameters 
have very different scales. (but see the log scale factor discussion 
below) 
 
It also creates panels for: 
 
ParameterPanel
    Specifies the current value of the parameter. 
    If the checkbox to the left of the parameter name is active, the 
    parameter will be varied by the fitting algorithm. Otherwise it 
    will be held constant. This panel should be Closed if a parameter 
    name is added, changed, or deleted in order that the list can be 
    updated. 
 
DomainPanel
    Specifies the minimum and maximum values of a parameter. 
    Group attributes can be used to set some common domains en masse. 
    If the parameter is set to a value outside this range, the error 
    function will return a very large value (1e9) and not execute 
    the multiple run protocols. If there is an X in the Log column then 
    log scaling is used during optimization. For positive (or negative) 
    definite parameters this can result in very great efficiency improvements 
    during parameter search and certainly obviates scaling difficulties. 
    To change an individual domain property, double click on a name 
    in the domain list. A string editor will pop up and you can 
    enter three space separate numbers. The first is a flag (1 or 0) 
    which specifies whether log scaling is to be used for this 
    parameter. If selected and the range does not include 0, an "X" will 
    appear in the "Log" column. The remaining two numbers are the 
    low and high values of the range. This panel should be Closed if 
    a parameter name is added, changed, or deleted in order to update 
    its list. 
 
OptimizerPanel
    This controls the fitting algorithm. This menu item 
    does not really belong here since it has nothing to do with parameters 
    but it had to go somewhere. Currently, the only fitting algorithm 
    that can be used by the MulRunFitter tool is :func:`fit_praxis` . 
    "Real time", "#multiple runs", and "Minimum so far" 
    show some statistics for the current fitting process. 
    "# quad forms before return" is the argument to a :func:`stop_praxis` function 
    call. A value of 0 means that praxis returns when the local space 
    around the minimum it has found meets the criterion chosen by 
    :func:`attr_praxis` . In this tool, the default values are 
    tolerance=1e-4 
    maxstepsize=.5 
    printmode=0. A non-zero value indicates the number of complete 
    principal axis search steps to be performed before returning. 
    If the error function is a quadratic form, praxis will theoretically 
    find the minimum in one of these steps. After a quad form step, praxis 
    contains a quadratic form approximation to the local search space. 
    The eigenvalues and eigenvectors of this quadratic form are obtainable 
    from :func:`pval_praxis` 

 
When the :guilabel:`Append the path to savepath.fit` checkbox is checked, 
the elapsed time, fitting error value, and parameters are appended to 
a file called :file:`savepath.fit` every time the error value is reduced 
by a call to the error function.. 

The :guilabel:`Running` checkbox is on when the optimizer is running. You should 
not do much gui interaction at this time such as opening new 
windows or pressing buttons that could potentially interfere with 
the protocol runs. First press the :guilabel:`Stop` button (may need to press 
it several times) to stop the optimizer. The :guilabel:`Optimize` button 
starts the fitting process. 
 
Generators Menu
~~~~~~~~~~~~~~~
 
The :guilabel:`Generators` menu is used to add and view stimulus protocols 
(which include error functions for calculating the difference between 
simulation results and data). Each generator is a generalization of 
a :ref:`RunFitter`. 
 
:guilabel:`Add Generator`
    creates an empty "Unnamed single run protocol" which 
    is turned off (the "-" in front of its name). A generator which 
    is turned off is not used (does not result in a run and does not 
    contribute to the total error value) during optimization or when 
    the :guilabel:`ErrorValue` button is pressed. 
 
:guilabel:`Remove Generator`
    destroys a generator when a user double clicks on 
    its name. 
 
:guilabel:`Change Name`
    When you double click on a generator name a 
    string editor pops up in which you can enter a (hopefully meaningful) 
    name for the protocol. 
 
:guilabel:`Use Generator`
    Double click on a generator name to toggle whether 
    it is used by the total error function. When a generator is used, 
    a "+" appears in front of its name. 
 
:guilabel:`Multiple Protocol Name`
    pops up a string editor which allows you 
    to enter a name for this instance of the entire multiple run fit. 
 
:guilabel:`Display Generator`
    Single clicking, or even dragging the mouse 
    over the generator list items, displays the current selection in 
    a separate generator panel. When all the generators are empty this 
    panel is very small. So first select a Fitness/VariableToFit 
    to specify a dependent variable, e.g ``soma.v(.5)`` or 
    ``SEClamp[0].i`` to fit to data. Then close the panel and reopen 
    it (requires double clicking if the item is already selected in 
    the generator list) in order to see the Graph portion of the display. 
    The generator panel is a :class:`Deck` and the top card is the one 
    selected in the generator list. 
 
Fitness menu of a Generator instance in the generator Panel
=========================================================== 
 
:guilabel:`Variable to fit`
    pops up a symbol chooser for selecting a dependent 
    variable which you wish to fit to data for this protocol. 
    For example, possible dependent variable for which you might have 
    data are  ``soma.v(.5)`` or 
    ``SEClamp[0].i``. Any number of dependent variables can be fit for 
    one protocol (run with particular stimulus values set, see below). 
    For a voltage clamp family, there would only be one current (dependent 
    variable) per generator and different generators for each voltage level. 
    The only case in which there would be more than one dependent variable 
    for a single generator is when there are multiple electrode recordings. 
    The list of dependent variables for this generator is indicated 
    in a sequence of radio button. The large panel below this list 
    contains information about the error function for this variable 
    as well as graph for the data and simulation result for the selected 
    dependent variable. 
 
:guilabel:`Remove a Fit Variable` and :guilabel:`Change a Fit Variable`
    pop up 
    lists in which you double click an item to perform the 
    removal action or pop up a string editor to change a dependent 
    variable name. 
 
:guilabel:`Change Method to`
    a menu which specifies the fitness (error) 
    functions available for comparing the currently selected (radio list) 
    dependent variable. "RegionFitness" is the default error function 
    and is very similar to the error function for the :ref:`RunFitter` . 
    In every case, the Graph menu is used to get the data from the 
    clipboard. 
 
:guilabel:`RegionFitness`
    error value is the square norm between data and 
    dependent variable treated as continuous curves. 
 
:guilabel:`YFitness`
    error value is the sum of the square difference of 
    the selected data points (indicated by short vertical blue lines) 
    and the dependent variable at those times. Several Graph menu items 
    are available to add, remove, and move these points. 
 
:guilabel:`Protocol Constant`
    Pops up a symbol chooser in which you can 
    select a stimulus variable. This will be added as a default field 
    editor to the protocol portion of the generator panel. There can 
    be any number of protocol constants. During a run of this 
    generator, the values of the protocol constants are set to the 
    indicated values. After the run, the constants are returned to 
    their original values. 
 
:guilabel:`Remove Protocol Constant`
    Pops ups a list of the existing protocol 
    constants. Double click on an item to throw one away. 

