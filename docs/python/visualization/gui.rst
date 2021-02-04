.. _gui:


GUI Look And Feel
*****************

.. seealso::
     :ref:`NEURONMainMenu`, :ref:`standardruntools`

     
The Print window manager gives special meaning to each mouse button 
as described in the PrintWindowManager section. 
 
This interface is profligate in its use of windows. In the course 
of a simulation it is expected that windows will be created and 
destroyed freely. In OpenLook one dismisses a window merely by 
pressing the right(menu) button in the window header area and 
selecting dismiss. If your window manager does not support easy 
dismissing of windows then you *MUST* invoke nrniv with the 
automatic dismiss button feature. The default widget style is 
MOTIF but openlook style can be selected. The relevant options are 
 
``nrniv -dismissbutton -openlook ...``
     

Scene
~~~~~

Graphs and Shape windows are instances of views into a scene.

.. code-block::
    python
    
    from neuron import h
    
    g = h.Graph() 
    g.label("here is some text") 
    g.beginline() 
    g.line(100, 50) 
    g.line(200, 100) 
    g.flush() 

    #pop up an example of a scene 

A scene is defined as the space represented by the model 
coordinate system. A view is defined as that portion of the scene 
which is drawn in the window. 
There can be many views into a scene and each scene has its own 
menu selections but they can be different for different scenes. 
Scenes use an openlook style in which the right(menu) mouse button 
pops up menus from which one either selects an action to be performed 
immediately, eg. Round or Whole Scene, or else selects a meaning for 
the left(select) mouse button which will be used when the left mouse 
button is pressed within any view of that particular scene. This can 
be thought of as selecting a tool with the right button and wielding 
that tool with the left button. 
These menu items are like radio buttons in that only one is active at 
a time. It is recommended that when you move into a view you quickly 
pop up the menu so you can be certain of the meaning of the left button. 
The middle button translates the view around the scene. All 
scenes have a common View menu as the first menu item 
with which one can create a new view, zoom in/out, round the view 
or make the view correspond to the natural size of the scene. 
     
.. _gui_view_equal_plot:

View = Plot
===========

View = plot: scale the view with respect to the variables being 
plotted. 


ZoomOut10
=========

10% Zoom out: View more of the scene keeping the center of the view 
fixed. 
 

ZoomIn10
========

10% Zoom in: View less of the scene keeping the center of the view 
fixed. 
 

NewView
=======

New View: Mode for creation of new view windows. 
Use the left mouse button to 
draw a rectangle in the view which will become the interior of 
a new view. Press the left mouse button at one corner of the desired 
view and drag it to the opposite corner. The new view window can then be 
resized and positioned using your window manager. 
 

ZoomInOut
=========

Zoom in/out: The location where the left mouse button is pressed is 
the fixed point of zooming, ie doesn't change its position. 
Dragging the mouse up and to the right zooms in. Dragging 
the mouse down and to the left zooms out. Graph views have 
independent scaling in the x and y directions. There is 
a bias toward changing only one dimension corresponding to 
the general drag direction.  Shape views 
have a constant aspect ratio so only the x direction is used 
for zooming. 
 

Translate
=========

Translate: Drag the scene around in the view. The middle mouse button 
is always attached to this tool. There is a drag bias which 
makes it easier to move in the horizontal or vertical direction 
without change in the other dimension. 
 

RoundView
=========

Round View: The header of each view window shows the size of the canvas 
in model coordinates. Pressing this button rounds the view 
so these numbers don't have so many decimal places. 
The algorithm for rounding needs improvement. 
 

WholeScene
~~~~~~~~~~

Whole Scene: Adjusts the zoom and translation so the view is of the 
entire scene with a  10% border. 
 

SetView
~~~~~~~

Set View: Successive dialogs for x and y view size each require user to 
enter two space separated numbers for the beginning and end 
of axis. Default values are left, right, bottom, top of view 
reduced by 10%. The view size is set to the entries and then 
the view zooms out by 10%. ie accepting the default values 
leaves the view unchanged. 
 

Scene = View
~~~~~~~~~~~~

Scene=View: Defines the size of the whole scene. 
Sets the scene size to the size of the view. Subsequent 
Whole Scene adjustments will return to this size. 
 

ObjectName
~~~~~~~~~~

Prints the name by which the interpreter knows this object. Within this 
session the user can use this name to manipulate the object via interpreter 
commands. 
 

Browser
~~~~~~~

Browsers are visible lists.

.. code-block::
    python
        
    from neuron import h, gui

    f = h.File()
    f.chooser('', 'Example file browser', '*', 'Type file name', 'Cancel')
    while f.chooser():
        print(f.getname())



.. image:: ../images/filechooser.png
    :align: center
            

The list can be scrolled with a scroll bar but 
I think it is most convenient to drag the list up and down with the middle 
mouse button. Rate scrolling is controlled with the right mouse button. 
The left button highlights a selection. Double clicking generally executes 
the selection. Browsers are used to select files for printing, 
variables for plotting, etc. Sometimes, a browser has a field editor in which 
one can directly type an entry. Usually after an item has been selected you 
have to press an :guilabel:`Accept` or :guilabel:`Cancel` button to actually execute the selection. 
Browsers can be scrolled with :kbd:`d`, :kbd:`u`, :kbd:`j`, :kbd:`k`, :kbd:`n`, :kbd:`p` and others. 
 

FieldEditor
~~~~~~~~~~~

See also :ref:`ValueEditor`, a FieldEditor for floating point numbers. 
Field editors accept a string entered by the user.  The allowed strings 
are determined by the context.  In not all cases does typing the return 
key signal the execution of a selection (if not, press the :guilabel:`accept`
button).  Field editors have an emacs-like syntax and typing characters 
inserts them at the cursor.  The left mouse button specifies the cursor 
location and dragging selects a portion of the string.  After selecting 
a portion of the string, typing a character will replace that portion 
with the character. 

.. code-block::
    none

    	^A beginning of line 
    	^E end of line 
    	^F forward one character 
    	^B backward one character 
    	^U select whole string 
    	^W select from cursor to beginning of string 
    	^D delete next character 
    	^H delete previous character 
    	return (normally accept) 
    	escape, ^G (normally cancel) 
    	and others 

 

Panel
~~~~~

Panels: windows containing buttons, menus, and value editors. All mouse buttons 
mean the same thing. 
 
If the number of items in a vertically arranged single panel is greater 
than the number in the ``*panel_scroll:`` resource in the 
`$(NEURONHOME)/lib/nrn.defaults https://github.com/neuronsimulator/nrn/blob/master/share/lib/nrn.defaults.in>`_ file (default 12) then the panel items 
are shown in a scroll box so that they do not take up so much screen 
space. 
 
See :func:`xpanel` for hoc functions to generate panels 

.. code-block::
    python
    
    from neuron import h
    
    # pop up example panel 
    strdef tempstr 
    tempstr = "slider................." 
    x=.1 
    xx = 0 
    y=0 
    z=0 
    h.xpanel("Example Panel") 
    h.xbutton("PushButton", "print \"released button\"") 
    h.xlabel("Following two are for variable x") 
    h.xvalue("Value Editor", "x", 0, "print x") 
    h.xvalue("Default Value Editor for variable x", "x", 1, "print x") 
    h.xcheckbox("Checkbox", &y, "print \"state y is \", y") 
    h.xstatebutton("StateButton", &z, "print \"state z is \", z") 
    h.xmenu("Example Menu") 
    h.xbutton("Item 1", "print \"selected item 1\"") 
    h.xbutton("Item 2", "print \"selected item 2\"") 
    h.xcheckbox("Checkbox", &y, "print \"state y is \", y") 
    h.xradiobutton("Radio 1", "print 1") 
    h.xradiobutton("Radio 2", "print 2") 
    h.xradiobutton("Radio 3", "print 3") 
    h.xmenu() 
    h.xlabel("Following 3 are mutually exclusive") 
    h.xradiobutton("Radio 1", "print 1") 
    h.xradiobutton("Radio 2", "print 2") 
    h.xradiobutton("Radio 3", "print 3") 
    h.xvarlabel(tempstr) 
    h.xslider(&xx, 0, 100, "sprint(tempstr, \"slider for xx = %g\", xx)") 
    h.xpanel() 


     

Button
======

Buttons: execute an action when the mouse button is pressed and released over 
the button widget. 

Menu
====

Menus: Drag the mouse to the desired item. If the menu fails to go away you 
can press one item and then move the mouse away and release. This 
should cause the menu to unmap without executing the item. 

.. _valueeditor:

ValueEditor
===========

Value editor: A combination button with label and a field editor. 
If a value is being entered the label is colored yellow 
and there is 
a cursor in the field editor. You might have a desired value in the 
editor but if the label is yellow the computer will not know it. Make 
sure values are accepted by pressing return or by pressing the button. 
Arbitrary expressions may be entered into value editors. They will 
be replaced by their value upon acceptance. 
Pressing the middle/right mouse button over a digit will 
increase/decrease the digit by 1. Dragging 
will increase/decrease repeatedly. 
(but don't forget to release the label button to accept 
the value). 

Default Value Editor
====================

Default Value editor: 
These value editors have an extra check box to the left of the value 
field  which is marked when 
the value is different from its creation value. One may toggle 
between the default and most recent value by pressing the check box 
with the left or middle mouse button. 
The default value may be permanently changed by pressing the check 
box with the right button. 
On the right of the value field is a stepper 
(little button with the	up arrow) that is used to change values in 
lieu of typing a number. 
The stepper works as follows: 
left mouse button: increase by the increment 
middle mouse button: decrease by the increment 
right mouse button: select the increment. Res stands for resolution 
and means the increment is the least significant digit in the value 
field. The only other increments are the decades between .001 
and 1000. When holding down the left or right mouse button, after 
a short time the stepper will repeatedly increment the value 
field. Every 20 steps, the increment will increase by a factor of 
10 but will return to its first step value on release. The repetition 
mode will not cross 0. To cross 0 release and re-press. 
Only on release of the mouse button will the action (if any) 
be executed and finally all value editors will be updated. 
The default increment starts at the least significant digit in the 
value field. Stepper delays use the resources: 

*   autorepeatStart: .05    //seconds 
*   autorepeatDelay: .02 
 
 
.. _pwm:

Print & File Window Manager
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Its primary purpose is to organize the windows onto a page for printing. 
The manager contains two scenes representing the screen and a piece of 
paper.  The location and relative size of each hoc window appears on the 
screen scene. 
 
See :func:`pwman_place`. 

ScreenItem
==========

To specify which subset of windows is to be printed you click on the 
relevant rectangles in the screen scene.  A rectangle representing the 
relative location and size on the page will appear in the page scene. 
 

PaperItem
=========

Windows selected for printing may be manipulated in the page scene. 
Place the mouse cursor over the desired window rectangle in the page 
scene and: 
 
Right button
    remove the window from the page.  If one clicks again 
    on that window in the screen scene then the window will return to the 
    same location and relative size on the page as when it was removed. 
 
Middle button
    resize the window.  This resizes not the window on the 
    screen but how large the window will appear on the page.  The window 
    always maintains the same aspect ratio as the window on the console 
    screen.  To resize, drag the mouse with the button down til the desired 
    size is reached. 
 
Left button
    move the window.  Drag the mouse to the desired position 
    and release the button. 
 
When the manager is iconified, all the windows disappear.  When the 
manager is redisplayed all the windows come back where they left off. 
(This is the case for openlook. Many window managers do not allow easy 
dismissing, moving, and resizing of transient windows and therefore 
require the use of top level windows which do not iconify as a group) 
 
 

Help
====

Help: Toggles the interface into help mode. In help mode 
the cursor changes to a "?" and a help message will be 
displayed for any button or menu item that is pressed while 
the question mark cursor is present. No actions are executed 
in help mode but sometimes dialog boxes may pop up which 
should be canceled in order for them not to do anything. 
Pressing the help button in help mode will return to 
the normal interface with an arrow cursor. 
 
The help system requires a running Netscape process. If the system 
is not working properly on your machine, the help 
button can be removed by specifying ``*pwm_help: off`` in the 
:file:`nrn/lib/nrn.defaults` file. 

.. _pwm_print:

Print
=====

Print: Sends the postscript images of the windows to a printer 
selected by the Other menu item, :ref:`SelectPrinter`. 
If no printer has been selected a printer 
dialog pops up. See :ref:`WindowTitlesPrinted` . 
 

PrintToFile
===========

Print to File: Menu for saving windows to a printable file in the formats 
 
.. seealso::
    :func:`print_session`, :ref:`WindowTitlesPrinted` 

PostScript
""""""""""

PostScript: Pops up dialogue requesting Filename for saving the postscript 
images of the windows appearing	in the page icon. A :file:`.ps` suffix is recommended. 

Idraw
"""""

Idraw: Filename for saving an idraw format of graph windows appearing 
in the page icon. Each graph is an idraw group. Idraw is an excellent 
program for polishing graphs to publication quality. 
A .id suffix is recommended. 

.. _printtofile_ascii:

Ascii
"""""

Ascii: Filename for saving an ascii format of the lines in graph windows 
appearing in the page icon. :meth:`Graph.addvar` and :meth:`Graph.addexpr` 
lines in a Graph window are saved if there are some and there 
is no :meth:`Graph.family` label. If there are no addvar/addexpr lines 
or if there is a family label then all lines on the graph with more 
than two points are printed (along with their labels, if any). 
If all the lines have the same number of points and they are all 
labeled then the file is printed in matrix form (with the first column 
being the x values). Header information is also printed that gives 
a manifest of the lines and their sizes. 
 
Unlabeled lines are printed at the end of the file with the format 

.. code-block::
    none

    	number unlabeled 
    	number points in first unlabeled line 
    	x y pairs of point 
    	number points in second unlabeled line 
    	... 


 
.. seealso:: :ref:`FamilyLabel` 
 

Session
=======

Session: Menu for savings windows for recall 

Retrieve
""""""""

Retrieve: Retrieves a saved session. Note that the saved values in the value 
editors become the default values when retrieved. 

SaveSelected
""""""""""""

SaveSelected: Saves size, location, and values of the panels, graphs, 
and shapes (but not browsers) appearing on the paper icon in the 
indicated file.	A .ses suffix is recommended. This is usually more 
useful than saving all items on the screen since it is normally 
the case that most of the user effort goes into specifying the 
graphs and most of the other windows are generated by the interpreter. 
The model coordinate size of all scenes is given by the view size 
of the primary view window. Therefore after a retrieve, the 
"whole scene" menu operation will restore the view size when saved. 

.. _session_saveall:

SaveAll
"""""""

SaveAll: Saves all windows in the specified file. 
 
See :func:`save_session` 

Other
=====

Other: Menu of other options 

.. _selectprinter:

SelectPrinter
"""""""""""""

SelectPrinter: Enter your normal system command for printing. The Print button will 
send post script to this command. for example: 
``lpr -Plp``
 
Unix and Mswindows versions construct a print line of the form 

.. code-block::
    none

    pwm_postscript_filter < temp_filename | PRINT_CMD ; rm temp_filename 
    pwm_postscript_filter temp_filename printer_command 

respectively. 
In the mswindows version pwm_postscript_filter and printer_command may 
be set in the nrn.def(aults) file. The default printer command is 
" > prn" 
 
In the unix version the printer command is found from the 
"PRINT_CMD" environment variable. 
 
.. _windowtitlesprinted:

WindowTitlesPrinted
"""""""""""""""""""

If checked, then window titles are printed when the windows are printed. 
Titles are always printed when :func:`print_session` is executed. 
 

VirtualScreen
"""""""""""""

VirtualScreen: Useful for mswindows version when using low-resolution monitor. 
Also invoked under mswindows when the :kbd:`F1` key is pressed when focus in 
any InterViews window. 
 
Pops up a view of the print window manager's screen icon to allow moving 
of windows (select and drag with left mouse) and changing the center of the 
screen (click with middle button). 
 
What makes this useful under mswindows is 1) the fact that it can 
be raised to the top of the window hierarchy (The print window manager 
can't since it is the parent to all InterViews windows) and 2) there is 
a scale button which can scale the window size so all windows will fit 
on the screen. 
 

LandPort
""""""""

LandPort 
Land/Port: The page will be printed in landscape or portrait mode.  The 
mode is indicated by the orientation of the page icon. 
 

Tray
""""

Tray: The windows on the page icon are collected into a single larger 
window consisting of a row of columns. The algorithm for doing this 
isn't too smart but you can get good trays by arranging the page window icons 
in a row of columns. When you dismiss a tray a dialog box pops up which asks 
if you want to dismiss the window or dissolve it into its original windows. 
Trays can be saved and retrieved but they cannot be subsequently dissolved. 
 

Quit
""""

Quit: Pops up dialog to allow Exit from NEURON. 
On exit will ask if you want to save open editor buffers. 
 
.. _gui_graph:

Graph
~~~~~

.. _graph_crosshair:

Crosshair
=========

Crosshair: shows coordinates and enables access to all line data. 
Press the left mouse button (LMB) near a line and drag the 
mouse left or right. A cross hairs will appear with the x,y value in 
model coordinates. The behaviour of the cross hairs makes it 
convenient to find local maxima. On creation the crosshairs starts 
at the nearest point on the line. On dragging it searches from the 
last point for the nearest point but will stop searching if any point 
becomes farther away. This makes it possible to easily follow 
phase plane plots. Crosshairs may call a hoc function on a keypress. 
See :meth:`Graph.crosshair_action`.
 
If no crosshair action has been installed, any keypress will print 
the x,y coordinates of the crosshair in the terminal window. 
 
Note that a crosshair_action can obtain all the x,y coordinate data 
for a line. Also the global variables :data:`hoc_cross_x_` and 
:data:`hoc_cross_y_` contain the last value of the crosshair coordinates. 
 

.. _gui_plotwhat:

PlotWhat
========

Plot What?: Pops up a browser with which one can navigate to any 
variable (double clicking) to enter it into the field 
editor. Double clicking on object names or section names 
will cause more names to appear in the adjacent browser and allows 
one to quickly build a complete symbol name. Alternatively one 
can directly type or edit the name in the field editor. When you 
are satisfied with the name in the field editor type return or 
press the accept button. The program will check if the name (or 
arbitrary expression) is interpretable and, if so, will be added to 
the list of expressions to be plotted in this graph whenever 
Graph.plot(xvalue) is executed. Warning: some names in the 
browsers are not interpretable or make no sense being plotted. 
If there are inconveniently many names in the first browser, 
you can use the Show menu to reduce the selection to only 
variables, objectvars, sections, or objects. Note that the objects 
allow plotting of variables which may otherwise not be accessible 
to the interpreter because there is no objectvar that references 
them. However, unfortunately, such graph lines cannot be saved in 
a session. 
 
If a variable in the browser contains the word [all] in place of 
an explicit index then the Graph will plot it as a function of 
its index. See :meth:`Graph.vector` . 
 

.. _gui_pickvector:

PickVector
==========

When this tool is chosen, clicking the left mouse button near 
a graphed line will copy the y and x coordinates of the line 
into two new :class:`Vector`'s which are referenced by :data:`hoc_obj_`\ [0] and 
:data:`hoc_obj_`\ [1] respectively. The vectors may be saved to a file by selecting 
the :ref:`Vector_SavetoFile` item from the Vector menu of the 
NEURONMainMenu 
 

PlotRange
=========

If the graph is doing a space plot with a RangeVarPlot then 
the PlotWhat item changes its style to request entry of another 
range variable to plot using the same path. Also one can enter 
an expression involving $1. The expression will be executed for 
each section in the path for each arc position set to $1. 
 

.. _gui_changecolor_brush:

ChangeColor-Brush
=================

Change Color: Pops up a color and brush palette to select the 
default color and brush style for the graph. 
Clicking on text or lines will change the line/text to that style. 
After the palette is dismissed it can be retrieved by clicking 
on another radiomenu item and then clicking on this one again. 
Note: Lines associated with labels always have the same color. 
Kept lines are not associated with labels. 
The number of selectable colors and brushes may be set by 
changing the values in your :file:`~/.nrn.defaults` file (see CBWidget in 
`$(NEURONHOME)/lib/nrn.defaults <https://github.com/neuronsimulator/nrn/blob/master/share/lib/nrn.defaults.in>`_) 
 

AxisType
========

Axis Type: 
Menu of View Axis, New Axis, View Box, and Erase Axis. 

ViewAxis
""""""""

View Axis: 
Erases the old axis and draws a set of axes in the background. 
The axes are sized dynamically with respect to the view coordinates. 

NewAxis
"""""""

New Axis: 
Erases the old axis and draws a new axis in a rounded view. 
The new axis depends on the size of the view and is the same in 
every view of the scene. 

ViewBox
"""""""

View Box: 
Erases the old axis and draws an axis box as a background 
with clipping. The box is sized dynamically with respect to the 
view coordinates. 
 
.. _keeplines:

KeepLines
=========

Keep Lines: While checked, lines are saved. When not checked 
the previous line is discarded every time 
:meth:`Graph.begin` is executed in preparation for plotting new lines. 
A useful idiom to save a reference line is to toggle the Keep Lines 
item on and then off. 

.. seealso::
    :meth:`Graph.family`, :ref:`FamilyLabel`

.. _familylabel:

FamilyLabel
===========

Pops up a global (same for all Graph windows) symbol chooser 
which is used to select a label for :ref:`KeepLines`. Function is 
identical to :meth:`Graph.family`. Ie. the label is used as a variable name 
and the value of the variable is used to actually label the kept lines. 
To get a compatible label (instead of an :meth:`Graph.addexpr` label) 
for the last line, the KeepLines menu item should be toggled off. 
 
If all lines are labeled and have the same size then :ref:`PrintToFile_Ascii` 
has a matrix format. 
 

Erase
=====

Lines are erased but not text or axes. 
 

MoveText
========

One can drag text to another location in the scene. 
 

ChangeText
==========

Clicking on existing text allows one to change it. 
Clicking on an empty spot creates a label at that location. 
One can change a plot expression. eg. to add a scale factor. 
Labels for plot variables that use the more efficient 
pointers cannot be changed. Labels can be marked as either 
fixed with respect to scene/model coordinates or fixed 
with respect to view/screen coordinates 
 

Delete
======

Delete: One can delete text or lines by pressing the left key while 
the mouse cursor is over the object.  If the text is associated with a 
line the line is deleted as well as its label. 
     

