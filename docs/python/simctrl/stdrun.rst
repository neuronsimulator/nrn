.. _0stdrun:

.. _standardruntools:

Standard Run Tools
------------------


All standard tools are available from the NEURONMainMenu. The fastest 
way to load these tools is to execute:

.. code-block::
    python

    from neuron import gui

 

Brief summaries of the menu options are provided below, for more information on select functions see also:

.. toctree::
    :maxdepth: 1
    
    runctrl.rst
    family.rst

Implementations of the standard tools are in `$NEURONHOME/lib/hoc/*.hoc <https://github.com/neuronsimulator/nrn/blob/master/share/lib/hoc>`_ 
     
.. _NEURONMainMenu:

NEURON Main Menu
~~~~~~~~~~~~~~~~

 
Main menu for standard control, graphing, menu generation. 
 
To pop up the panel execute: 

.. code-block::
    python

    from neuron import gui

.. warning::

    In HOC code, you may see ``load_file('nrngui.hoc')`` instead, but that does not work for Python code
    as it does not start the thread that monitors for GUI events.
 
Serious users should peruse the init and run procedures. 
The run chain that eventually calls :func:`fadvance` is 

.. code-block::
    none

        h.run --> h.continuerun --> h.step --> h.advance --> h.fadvance 

There is often reason to substitute a new step or advance 
procedure to do intermediate calculations on the fly. 
Sometimes it is useful to replace the init() procedure. If so 
make sure you don't take away functionality which is already 
there. See `$NEURONHOME/lib/hoc/stdrun.hoc <https://github.com/neuronsimulator/nrn/blob/master/share/lib/hoc/stdrun.hoc>`_ for the 
implementations of these procedures.

A simple example of overriding init:

.. code-block::
    python

    h('proc init() {finitialize(v_init) nrnpython("myinit()")}')

    def myinit():
        # new code to happen after initialization here
        print('initializing...')
        # only need the following if states have been changed
        if h.cvode.active():
            h.cvode.re_init()
        else:
            h.fcurrent()
        h.frecord_init()

     
     

File
====

LoadSession
"""""""""""

Pop up a File chooser for loading a previously save graphical 
interface session (default extension .ses) Session files may be loaded 
several times. 

LoadHoc
"""""""

Pop up a File chooser for loading and executing a hoc file. 
Selected Hoc files are loaded only once. 

LoadDLL
"""""""

Pop up a File chooser for dynamically loading a dll containing 
compiled and linked model descriptions. This is available only under mac os 
and mswin. 

savesession
"""""""""""

Save all the windows, mapped and hidden, in a session file. 
Greater control over exactly which windows are saved is given by the 
:ref:`GroupManager` and the 
:ref:`PWM`.

.. _workingdir:

workingdir
""""""""""

Pops up a Directory chooser for 
changing to a specified working directory. 
If a dll file has not already been loaded 
and if a :file:`nrnmech.dll` (under mswin) or a :file:`nrnmac.dll`
(under macos) exists in the "changed to" 
directory then the dll file is loaded. 

recentdir
"""""""""

A list of the last 10 directories chosen from the :ref:`workingdir` menu 
item. On the mac and mswin, if a dll file has not been previously 
loaded and such a file exists in the directory, then it will be 
loaded. 

Quit
""""

Exits NEURON. 
 


Edit
====


Build
=====

singlecompartment
"""""""""""""""""


CellBuilder
"""""""""""

Pops up a new instance of a :ref:`celbild` 
for specifying the topology, 
shape, and biophysical properties of a neuron. 

.. _networkcell:

NetworkCell
"""""""""""


FromCellBuilder
...............

Pops up two tools used to specify 
synapse types and the locations of instances of these synapse types 
on a cell type defined by the :ref:`celbild` 
This makes a network ready cell type that can be used by the 
Note that the only Point Processes used to construct synapse types are those 
whose model description contains a NET_RECEIVE block. e.g. see 
:class:`ExpSyn` and :ref:`NetworkBuilder`.

ArtificialCell
..............

Pops up a tool for constructing artificial network ready cells from 
PointProcess types containing a NetReceive block 
that can also act as a :class:`NetCon` source. 
e.g. see :class:`IntFire1`.

.. _networkbuilder:

NetworkBuilder
""""""""""""""

Pops up a new instance of a NetBuild class 
for specifying cells and their :class:`NetCon` connections. 
Only network ready cells defined by the :ref:`NetworkCell` tools can be 
used with this class. 




Tools
=====

Fitting
"""""""


Parameterized Function
......................

Starts a :ref:`funfit` tool for plotting a parameterized function and 
easily exploring its behaviour while varying the parameters. 
Also can fit the function to data using either the simplex 
or principal axis methods. 
The more powerful :ref:`mulfit` is now recommended. 

.. _runfitter:

Run Fitter
..........

Starts a :ref:`runfit` tool for 
optimizing simulation parameters to best fit data. 
The more powerful :ref:`MulFit` is now recommended. 

Multiple Run Fitter
...................

Starts a :ref:`mulfit` tool for 
general optimization problems. This combines and extends 
dramatically the features of the :ref:`funfit` and 

.. _0stdrun_runcontrol:

Run Control
"""""""""""

Pops up a :ref:`runctrl` panel for controlling simulation runs. 

.. _variablestepcontrol:

Variable Step Control
"""""""""""""""""""""

Pops up a VariableTimeStep panel for controlling the :class:`CVode` 
variable time step, variable order method. 

Usevariabledt
.............

CVode is the integration method. See :meth:`CVode.active`.

Localvariabledt
...............

CVode is the integration method and there is a separate dt for 
every cell. 

AbsoluteTolerance
.................

The absolute tolerance used by CVode when it is 
active is given by this value times the specific state scale factor. 
This latter is normally 1, eg, for voltage, 
but if the state is normally found in a range <<1 or >>1 the scale 
factor may be explicitly specified in a model description or in 
the interpreter. 
See :meth:`CVode.atol` and :meth:`CVode.atolscale`

PointProcesses
""""""""""""""

Several useful tools for managing PointProcesses 
See :ref:`mech` for details about built-in point 
processes. The corresponding :file:`.mod` file must in general be 
examined in order to understand the particulars about a given 
point process type. 

Distributed Mechanisms
""""""""""""""""""""""

Several useful tools for managing density mechanisms such 
as distributed channels. 
See :ref:`mech_mechanisms` for details about built-in density 
mechanisms. The corresponding :file:`.mod` file must in general be 
examined in order to understand the particulars about a given 
mechanism type.




Managers
........


Inserter
,,,,,,,,

Starts an :ref:`Inserter` for the currently accessed section that 
allows one to insert and uninsert density membrane mechanisms. 
Currently this is most useful for single compartment simulations. 

Homogeneous Spec
,,,,,,,,,,,,,,,,

Starts a :ref:`ShowMech` tool that is useful for specifying constant parameters for 
membrane mechanisms in all sections of a simulation. 

Viewers
.......

.. _shapename:

ShapeName
,,,,,,,,,

Starts a :ref:`shapebox` tool that 
allows one to figure out the correspondence between the physical 
location of a section and a section name. Also allows one to 
get a parameter menu for the selected section. 

NameValues
,,,,,,,,,,

Pops up a panel for displaying values associated with Sections. 
 
Almost completely superseded by the more complete :ref:`ShapeName` except that 
this tool can make a panel of a single mechanism type. 
 

Mechanisms Globals
,,,,,,,,,,,,,,,,,,

Menu of possible membrane Mechanism's. Selecting an item pops up 
a panel showing the global parameters for this type of Mechanism. 
 

celsius
.......

Pops up a panel for viewing/setting the global temperature 
variable :data:`celsius` . 

globalRa
........

Pops up a panel for assigning a 
uniform value of :data:`Ra` (ohm-cm) to all sections. 
Ra used to be a global variable but is now a Section variable that 
can be different in different sections. This sets Ra forall sections 
equal to the value displayed in the fieldeditor. It used to 
be displayed in the NEURONMainMenu but that location is now 
administratively incorrect and error prone for models which manage 
Ra through the :ref:`CelBild`. 
 
 





Impedance
"""""""""

Menu of tools which use the :class:`Impedance` class to calculate 
voltage attenuation as a function of position and frequency 
 

Frequency
.........

Pops up an :ref:`ImpRatio` template tool for plotting the 
log of voltage attenuation (and other functions of impedance) 
between a selected injection and 
measurement site as a function of frequency. 
 

Path
....

Pops up a :ref:`impedance_impx` template tool for plotting the 
log of voltage attenuation (and other functions of impedance) 
at a specific measurement/injection site 
as a function of a selected path along the neuron in which current is 
injected/measured. 
 

LogAvsX
.......

Pops up a :ref:`impedance_logavsx` 
template tool for plotting the log of voltage attenuation 
(and other functions of impedance) between a specific measurement/injection 
site as a function of distance to every point on the cell. 
 

.. _stdrun_shape:

Shape
.....

Pops up an :ref:`ImpShape` template tool for displaying the morphoelectronic transform 
of neuron shape in which distance is represented as the negative log of attenuation. 
 


.. _ArchiveAndHardcopy:

Archive And Hardcopy
""""""""""""""""""""

Checkin this simulation to RCS and print all windows on the printer. 

See :ref:`project` 

Saves all (saveable) windows in this session to the file start.ses 
(:func:`save_session`). 
Prints the entire session to the filter :ref:`prjnrnpr` (:func:`print_session`). 

This menu item exists only when nrnmainmenu is executed after the file 
RCS/nrnversion exists. Ie when the files in the current working directory 
have been placed under NEURON :ref:`project` control. 




Miscellaneous
"""""""""""""



Family
......


Family1
,,,,,,,

Starts a :ref:`Family` tool for controlling a family of simulations. 
One defines a variable and set of values for looping over an 
action. 

Command
,,,,,,,

Starts an :ref:`ExecCommand` tool for specifying and 
executing a hoc command. 

Builders
........


Kinetic Scheme Builder
,,,,,,,,,,,,,,,,,,,,,,

Starts a :ref:`KineticBuild` tool for simulating a 
single channel kinetic scheme






Graph
=====

For creating common kinds of graphs of functions of time. 
These graphs are connected to the standard run procedure such 
that at every step (see :ref:`0stdrun_runcontrol`) the value of the functions 
are plotted.



Currentaxis
"""""""""""

Plots values vs t-.5dt 
Suitable for plotting ionic currents (when calculations are :data:`secondorder` 
correct). 

Stateaxis
"""""""""

Plots values vs t+.5dt 
Suitable for plotting states such as m_hh, n_hh, etc. These 
plots may be very accurate when :data:`secondorder` = 2. 

.. _shape_plot: 

Shapeplot
"""""""""

Starts a :class:`PlotShape` . A picture of a neuron suitable for specifying 
time, space, and shape plots. 

VectorMovie
"""""""""""

Starts a :class:`Graph` that is flushed when above plots are flushed. 
This is suitable for selecting vectors from the PlotWhat menu 
and seeing them change every time step. 

PhasePlane
""""""""""

Starts a :class:`Graph` for plotting f(t) vs g(t). When started a dialog 
box pops up requesting the expression for g(t). As in the PlotWhat 
browser for graphs you may enter any variable or function, but it 
should change when the RunControl's InitRun button is pressed. 

Grapher
"""""""

Starts a :ref:`Grapher` tool for plotting any expression vs a specified 
independent variable. Lines are not drawn on this graph in 
response to a run. However it can be made to control a family 
of runs. 

.. _voltage_axis:

Voltageaxis
"""""""""""

Plots values vs t. 
Suitable for plotting voltage and concentrations, especially when 
calculations are secondorder correct. 
v(.5) of currently selected section is always plotted but can 
be explicitly removed with the Delete command in the Graph menu. 





Vector
======



.. _vector_savetofile:

Save to File
""""""""""""

Menu for saving/retrieving the last Vector selection to a file. eg. 
from a :ref:`gui_PickVector` as well as other Vector tools. 
 
The format of the file is:

1)  optional first line with the format 

    .. code-block::
        none

        label:anystring 

2)  optional line with one number which is the count of points. 
3)  a tab separated pair of x, y coordinates 
    each line. If there is no "count" line, there must be 
    no empty lines at the end of the file and the last character must 
    be a newline. 
 
When the file is saved with this menu item, 
the label and count are always present in the file. 
For long files retrieval is much more efficient if the count is present. 
 
The implementation of these operations is in 
`$NEURONHOME/lib/hoc/stdlib.hoc <https://github.com/neuronsimulator/nrn/blob/master/share/lib/hoc/stdlib.hoc>`_
vectors and performing simple manipulations on them. 

.. seealso::
    :data:`hoc_obj_`


.. _stdrun_retrievefromfile:

RetrievefromFile
""""""""""""""""

See :ref:`vector_savetofile`
 

GatherValues
""""""""""""

Starts a :ref:`GatherVec` tool collecting x,y values 
where x and y come from variables. 

Play
""""

Starts an :ref:`VectorPlay` tool for playing a vector into 
a variable. 

Display
"""""""

Starts an :ref:`VecWrap` tool for displaying several 
vectors and performing simple manipulations on them. 

Draw
""""

Starts a tool for drawing a curve. 


Window
======

A list of all the ungrouped windows (except the NEURONMainMenu) 
and window groups. 
Windows mapped to the screen are indicated by a checkmark; others are 
hidden. Windows may be hidden by selecting the :guilabel:`Hide` item on the 
menubar under the window title. Windows may be hidden or mapped by selecting 
the item in the ungrouped window list. Selecting a window group will hide 
or map all the windows in that group. 

Ungrouped
"""""""""

A window appears in this list if it is not a member of a window group. 
All ungrouped windows may be mapped or hidden by selection of the show 
or hide menu item. 

.. _groupmanager:

GroupManager
""""""""""""

Window group names appear in this list. Selecting this item pops up 
a window group manager used for creating, renaming, inserting/deleting 
windows into the group, and saving a selected group to a session file. 
 
The window group manager is a dialog box so it must be closed after use. 
 
When a window group name is defined it may be selected in the WindowGroups 
list. The windows of the group are indicated in the middle list. 
Selecting items in this list and the ungrouped windows list removes or 
inserts the window into the group. 


