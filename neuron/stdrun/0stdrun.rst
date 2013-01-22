.. _0stdrun:

.. _standardruntools:

StandardRunTools
----------------

    All standard tools are available from the NEURONMainMenu. The fastest 
    way to load these tools is to execute 

    .. code-block::
        none

        load_file("nrngui.hoc") 

    which avoids inefficiencies of the :func:`load_proc` command on 
    mswindows machines. 
     
    Implementations of the standard tools are in :file:`$NEURONHOME/lib/hoc/*.hoc` 
     
.. _NEURONMainMenu:

NEURON Main Menu
~~~~~~~~~~~~~~~~

     
    Main menu for standard control, graphing, menu generation. 
     
    To pop up the panel execute: 

    .. code-block::
        none

        	load_file("nrngui.hoc") 

     
    Serious users should peruse the init and run procedures. 
    The run chain that eventually calls fadvance is 

    .. code-block::
        none

        		run continuerun step advance :func:`fadvance` 

    There is often reason to substitute a new step or advance 
    procedure to do intermediate calculations on the fly. 
    Sometimes it is useful to replace the init() procedure. If so 
    make sure you don't take away functionality which is already 
    there. See $NEURONHOME/lib/hoc/stdrun.hoc for the 
    implementations of these procedures. 
     
     

File
~~~~


Edit
~~~~


Build
~~~~~


Tools
~~~~~


Graph
~~~~~

    For creating common kinds of graphs of functions of time. 
    These graphs are connected to the standard run procedure such 
    that at every step (see :meth:`StandardRunTools.RunControl`) the value of the functions 
    are plotted. 

Vector
~~~~~~


Window
~~~~~~

    A list of all the ungrouped windows (except the NEURONMainMenu) 
    and window groups. 
    Windows mapped to the screen are indicated by a checkmark; others are 
    hidden. Windows may be hidden by selecting the "Hide" item on the 
    menubar under the window title. Windows may be hidden or mapped by selecting 
    the item in the ungrouped window list. Selecting a window group will hide 
    or map all the windows in that group. 

Ungrouped
~~~~~~~~~

    A window appears in this list if it is not a member of a window group. 
    All ungrouped windows may be mapped or hidden by selection of the show 
    or hide menu item. 

GroupManager
~~~~~~~~~~~~

    Window group names appear in this list. Selecting this item pops up 
    a window group manager used for creating, renaming, inserting/deleting 
    windows into the group, and saving a selected group to a session file. 
     
    The window group manager is a dialog box so it must be closed after use. 
     
    When a window group name is defined it may be selected in the WindowGroups 
    list. The windows of the group are indicated in the middle list. 
    Selecting items in this list and the ungrouped windows list removes or 
    inserts the window into the group. 
     

LoadSession
~~~~~~~~~~~

    Pop up a File chooser for loading a previously save graphical 
    interface session (default extension .ses) Session files may be loaded 
    several times. 

LoadHoc
~~~~~~~

    Pop up a File chooser for loading and executing a hoc file. 
    Selected Hoc files are loaded only once. 

LoadDLL
~~~~~~~

    Pop up a File chooser for dynamically loading a dll containing 
    compiled and linked model descriptions. This is available only under mac os 
    and mswin. 

savesession
~~~~~~~~~~~

    Save all the windows, mapped and hidden, in a session file. 
    Greater control over exactly which windows are saved is given by the 
    :meth:`Window.GroupManager`#NEURONMainMenu and the 
    :meth:`LookAndFeel.PWM` 

workingdir
~~~~~~~~~~

    Pops up a Directory chooser for 
    changing to a specified working directory. 
    If a dll file has not already been loaded 
    and if a nrnmech.dll (under mswin) or a nrnmac.dll 
    (under macos) exists in the "changed to" 
    directory then the dll file is loaded. 

recentdir
~~~~~~~~~

    A list of the last 10 directories chosen from the :func:`workingdir` menu 
    item. On the mac and mswin, if a dll file has not been previously 
    loaded and such a file exists in the directory, then it will be 
    loaded. 

Quit
~~~~

    Exits NEURON. 
     

singlecompartment
~~~~~~~~~~~~~~~~~


CellBuilder
~~~~~~~~~~~

    Pops up a new instance of a :meth:`StandardRunTools.CellBuilder` 
    for specifying the topology, 
    shape, and biophysical properties of a neuron. 

NetworkCell
~~~~~~~~~~~


FromCellBuilder
~~~~~~~~~~~~~~~

    Pops up two tools used to specify 
    synapse types and the locations of instances of these synapse types 
    on a cell type defined by the :meth:`StandardRunTools.CellBuilder` 
    This makes a network ready cell type that can be used by the 
    Note that the only Point Processes used to construct synapse types are those 
    whose model description contains a NET_RECEIVE block. e.g. see 
    :meth:`pointprocesses.ExpSyn`#neuron 
    :func:`NetworkBuilder` 

ArtificialCell
~~~~~~~~~~~~~~

    Pops up a tool for constructing artificial network ready cells from 
    PointProcess types containing a NetReceive block 
    that can also act as a :class:`NetCon` source. 
    e.g. see :meth:`pointprocesses.IntFire1`#neuron 

NetworkBuilder
~~~~~~~~~~~~~~

    Pops up a new instance of a NetBuild class 
    for specifying cells and their :class:`NetCon` connections. 
    Only network ready cells defined by the :meth:`Build.NetworkCell` tools can be 
    used with this class. 
     

RunControl
~~~~~~~~~~

    Pops up a :meth:`StandardRunTools.RunControl` panel for controlling simulation runs. 

.. _variablestepcontrol:

Variable Step Control
~~~~~~~~~~~~~~~~~~~~~

    Pops up a VariableTimeStep panel for controlling the :class:`CVode` 
    variable time step, variable order method. 

Usevariabledt
~~~~~~~~~~~~~

    CVode is the integration method. See :meth:`CVode.active`#classes 

Localvariabledt
~~~~~~~~~~~~~~~

    CVode is the integration method and there is a separate dt for 
    every cell. 

AbsoluteTolerance
~~~~~~~~~~~~~~~~~

    The absolute tolerance used by CVode when it is 
    active is given by this value times the specific state scale factor. 
    This latter is normally 1, eg, for voltage, 
    but if the state is normally found in a range <<1 or >>1 the scale 
    factor may be explicitly specified in a model description or in 
    the interpreter. 
    See :meth:`CVode.atol`#classes and :meth:`CVode.atolscale`#classes 

PointProcesses
~~~~~~~~~~~~~~

    Several useful tools for managing PointProcesses 
    See :meth:`neuron.pointprocesses` for details about built-in point 
    processes. The corresponding .mod file must in general be 
    examined in order to understand the particulars about a given 
    point process type. 

DistributedMechanisms
~~~~~~~~~~~~~~~~~~~~~

    Several useful tools for managing density mechanisms such 
    as distributed channels. 
    See :meth:`neuron.mechanisms` for details about built-in density 
    mechanisms. The corresponding .mod file must in general be 
    examined in order to understand the particulars about a given 
    mechanism type. 

Miscellaneous
~~~~~~~~~~~~~

     

Managers
~~~~~~~~


Inserter
~~~~~~~~

    Starts an :meth:`StandardRunTools.Inserter` for the currently accessed section that 
    allows one to insert and uninsert density membrane mechanisms. 
    Currently this is most useful for single compartment simulations. 

HomogeneousSpec
~~~~~~~~~~~~~~~

    Starts a :func:`ShowMechanism` tool that is useful for specifying constant parameters for 
    membrane mechanisms in all sections of a simulation. 

Viewers
~~~~~~~


ShapeName
~~~~~~~~~

    Starts a :func:`MenuExplore` tool that 
    allows one to figure out the correspondence between the physical 
    location of a section and a section name. Also allows one to 
    get a parameter menu for the selected section. 

NameValues
~~~~~~~~~~

    Pops up a panel for displaying values associated with Sections. 
     
    Almost completely superseded by the more complete :func:`ShapeName` except that 
    this tool can make a panel of a single mechanism type. 
     

MechanismsGlobals
~~~~~~~~~~~~~~~~~

    Menu of possible membrane Mechanism's. Selecting an item pops up 
    a panel showing the global parameters for this type of Mechanism. 
     

celsius
~~~~~~~

    Pops up a panel for viewing/setting the global temperature 
    variable :meth:`globals.celsius` . 

globalRa
~~~~~~~~

    Pops up a panel for assigning a 
    uniform value of :func:`Ra` (ohm-cm) to all sections. 
    Ra used to be a global variable but is now a Section variable that 
    can be different in different sections. This sets Ra forall sections 
    equal to the value displayed in the fieldeditor. It used to 
    be displayed in the NEURONMainMenu but that location is now 
    administratively incorrect and error prone for models which manage 
    Ra through the :func:`CellBuilder` . 
     
     

Voltageaxis
~~~~~~~~~~~

    Plots values vs t. 
    Suitable for plotting voltage and concentrations, especially when 
    calculations are secondorder correct. 
    v(.5) of currently selected section is always plotted but can 
    be explicitly removed with the Delete command in the Graph menu. 

Currentaxis
~~~~~~~~~~~

    Plots values vs t-.5dt 
    Suitable for plotting ionic currents (when calculations are :func:`secondorder` 
    correct). 

Stateaxis
~~~~~~~~~

    Plots values vs t+.5dt 
    Suitable for plotting states such as m_hh, n_hh, etc. These 
    plots may be very accurate when :func:`secondorder` = 2. 

Shapeplot
~~~~~~~~~

    Starts a :func:`PlotShape` . A picture of a neuron suitable for specifying 
    time, space, and shape plots. 

VectorMovie
~~~~~~~~~~~

    Starts a :func:`Graph` that is flushed when above plots are flushed. 
    This is suitable for selecting vectors from the PlotWhat menu 
    and seeing them change every time step. 

PhasePlane
~~~~~~~~~~

    Starts a :func:`Graph` for plotting f(t) vs g(t). When started a dialog 
    box pops up requesting the expression for g(t). As in the PlotWhat 
    browser for graphs you may enter any variable or function, but it 
    should change when the RunControl's InitRun button is pressed. 

Grapher
~~~~~~~

    Starts a :meth:`StandardRunTools.Grapher` tool for plotting any expression vs a specified 
    independent variable. Lines are not drawn on this graph in 
    response to a run. However it can be made to control a family 
    of runs. 

.. _vector_savetofile:

Save to File
~~~~~~~~~~~~

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
    :file:`$NEURONHOME/lib/hoc/stdlib.hoc`
    vectors and performing simple manipulations on them. 

.. seealso::
    :data:`hoc_obj_`


RetrievefromFile
~~~~~~~~~~~~~~~~

    See :ref:`vector_savetofile`
     

GatherValues
~~~~~~~~~~~~

    Starts a :meth:`StandardRunTools.GatherVec` tool collecting x,y values 
    where x and y come from variables. 

Play
~~~~

    Starts an :meth:`StandardRunTools.VectorPlay` tool for playing a vector into 
    a variable. 

Display
~~~~~~~

    Starts an :meth:`StandardRunTools.VecWrap` tool for displaying several 
    vectors and performing simple manipulations on them. 

Draw
~~~~

    Starts a tool for drawing a curve. 


Family
~~~~~~


Family1
~~~~~~~

    Starts a :meth:`StandardRunTools.Family` tool for controlling a family of simulations. 
    One defines a variable and set of values for looping over an 
    action. 

Command
~~~~~~~

    Starts an :meth:`StandardRunTools.ExecCommand` tool for specifying and 
    executing a hoc command. 

Builders
~~~~~~~~


KineticSchemeBuilder
~~~~~~~~~~~~~~~~~~~~

    Starts a :meth:`StandardRunTools.KineticBuild` tool for simulating a 
    single channel kinetic scheme. 

Fitting
~~~~~~~


ParameterizedFunction
~~~~~~~~~~~~~~~~~~~~~

    Starts a :func:`FunctionFitter` tool for plotting a parameterized function and 
    easily exploring its behaviour while varying the parameters. 
    Also can fit the function to data using either the simplex 
    or principal axis methods. 
    The more powerful :func:`MultipleRunFitter` is now recommended. 

.. _runfitter:

RunFitter
~~~~~~~~~

    Starts a :meth:`StandardRunTools.RunFitter` tool for 
    optimizing simulation parameters to best fit data. 
    The more powerful :func:`MultipleRunFitter` is now recommended. 

MultipleRunFitter
~~~~~~~~~~~~~~~~~

    Starts a :meth:`StandardRunTools.MulRunFitter` tool for 
    general optimization problems. This combines and extends 
    dramatically the features of the :func:`FunctionFitter` and 

Impedance
~~~~~~~~~

    Menu of tools which use the :class:`Impedance` class to calculate 
    voltage attenuation as a function of position and frequency 
     

Frequency
~~~~~~~~~

    Pops up an :func:`ImpedanceRatio` template tool for plotting the 
    log of voltage attenuation (and other functions of impedance) 
    between a selected injection and 
    measurement site as a function of frequency. 
     

Path
~~~~

Pops up a :func:`Impx` template tool for plotting the 
log of voltage attenuation (and other functions of impedance) 
at a specific measurement/injection site 
as a function of a selected path along the neuron in which current is 
injected/measured. 
     

LogAvsX
~~~~~~~

Pops up a :meth:`ImpedanceTools.LogAvsX` 
template tool for plotting the log of voltage attenuation 
(and other functions of impedance) between a specific measurement/injection 
site as a function of distance to every point on the cell. 
     

.. _stdrun_shape:

Shape
~~~~~

Pops up an :func:`ImpShape` template tool for displaying the morphoelectronic transform 
of neuron shape in which distance is represented as the negative log of attenuation. 
     
.. _ArchiveAndHardcopy:

Archive And Hardcopy
~~~~~~~~~~~~~~~~~~~~

Checkin this simulation to RCS and print all windows on the printer. 
 
See :func:`ProjectManagement` 
 
Saves all (saveable) windows in this session to the file start.ses 
( :func:`save_session` ). 
Prints the entire session to the filter :func:`prjnrnpr` ( :func:`print_session` ). 
 
This menu item exists only when nrnmainmenu is executed after the file 
RCS/nrnversion exists. Ie when the files in the current working directory 
have been placed under NEURON :func:`ProjectManagement` control. 

