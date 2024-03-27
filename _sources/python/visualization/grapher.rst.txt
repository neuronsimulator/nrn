.. _grapher:


Grapher
-------

A tool for graphing any set of expressions as a function of 
an independent variable. 
 
The Grapher is similar to a "for" statement. 
It iterates the independent variable over the range specified by the 
"Indep Begin" and "Indep End" field editors using "Steps" steps. 
At each step, it executes the statements 
specified in the Generator field (if any), and plots the values that are 
specified in the Graph scene from the :ref:`gui_PlotWhat` menu item at the x-axis 
location specified by the value of the "X-expr" (normally the same 
as the independent variable). 
 
These y values and X-expr can be variable names or function names. 
 
To pop up the widget say: 
 

.. code-block::
    python

    	h.load_file("grapher.hoc")  # reads this file 
    	h.makegrapher(1)            # pop up a new grapher 

 
Creating a grapher using 

.. code-block::
    python

    	h.makegrapher() 

pops up a short form of the Grapher without "Indep Begin", "Indep 
End", or "X-expr" buttons. The limits are defined by the x-axis length 
of the current view. 
 
Graphers may be saved in a .ses file. 
 
To use the grapher widget: 

    1)  enter the independent variable name (press the button) eg. x 
    2)  enter the begin, end, and steps values. 
    3)  specify the y variables: 
        in the graph panel press the left button and select "Plot What?" 
        and enter a HOC expression or variable to plot. eg. sin(x) 
        You can do this several times to plot several expressions. 
    4)  If the y variables are actual functions of the independent variable 
        press the Plot button to see the plot. If the y variables 
        are in fact just variables then you will need a generator 
        statement that tells how to compute the y variables given 
        a value of the independent variable. eg. Just for fun you can 
        try entering the generator statement: ``print x, sin(x)``

 

Example:

.. code-block::
    none

    	Example 1: plot sin(t) 
    	1) PlotWhat: sin(t) 
    	2) Plot 
     
    	Example 2: plot of steady state m process in nrniv 
    	1) Independent Var: x 
    	2) Generator: rates_hh(x) 
    	3) IndepBegin -100, Indep End 50 
    	4) Set View: x: -100 50  y: 0 1 
    	5) PlotWhat: minf_hh 
    	6) Plot 
    	 
    	Example 3: In context of Neuron Main Menu simulation 
    	0) pop up grapher by selecting appropriate "New Graph" submenu item. 
    	1) Independent Var: v_init 
    	2) Generator: init() 
    	3) IndepBegin -100, Indep End 50 
    	4) SetView: x: -100 50 y:cancel 
    	5) PlotWhat: <any set of variables in any section> 
    	6) Plot 
     
    	Example 4: peak inward current during voltage clamp 
    	Replace the standard: proc advance() {fadvance()} with a procedure 
    	that stores the peak inward current (and possibly sets stoprun=1 
    	when you are past the peak). Then in the Grapher set Steps to 20 
    	the independent variable to the voltage clamp amplitude and the 
    	generator to run(). 



Plot
~~~~

For each value of the independent variable the generator statement 
is executed (if it exists) and the PlotWhat expressions are plotted. 

EraseAll
~~~~~~~~

Removes all expressions from the graph. 

Steps
~~~~~

Number of independent variable values used to make the graph. 
For the small grapher, the range of the independent variable is 
the length of the x-axis. 

IndependentVar
~~~~~~~~~~~~~~

Dialog appears requesting the variable to be used as the independent 
variable (default t). If the variable is undefined it will 
be created. 

Generator
~~~~~~~~~

A statement to be executed after setting a value of the independent 
variable but before plotting the expressions. This allows plotting 
of variables that depend implicitly on the independent variable. 

IndepBegin
~~~~~~~~~~

For a grapher made with ``h.makegrapher(1)``, specifies initial value 
of the independent variable. 

IndepEnd
~~~~~~~~

Specifies final value of independent variable. 

Xexpr
~~~~~

A grapher made with ``h.makegrapher(1)`` allows separate specification of 
independent variable and the x axis plot functions. This allows 
phase plane plots. The Xexpr may be any function of the independent 
variable or an implicit function if a generator statement exists. 
 

