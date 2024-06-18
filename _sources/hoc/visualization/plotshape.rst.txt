
.. _hoc_pltshape:


PlotShape Window
----------------

A picture of all the sections in a scene. This is designed for use 
with 3-d reconstructions in which the 3-d shape is not usually tampered with. 
However it can also be used with stylized neurons with the following caveat: 
If you do invoke a shape scene with a stylized neuron, the system will impose 
its own 3-d information on those sections consistent with the current 
connections, diam, and L values of the sections. 
 
.. note::

    When :hoc:func:`define_shape` is called, 3d info for sections that were not
    cleared is translated in order to appear connected to its parent. 
 
 

Section
~~~~~~~

It is a vestigial menu item for PlotShape's and does nothing. 
In :hoc:class:`Shape` it selects the tool for calling various
actions when clicking on a part of the shape. 
 

Rotate3D
~~~~~~~~

Dragging rotates about an axis in the view plane that is 
perpendicular to the direction of mouse movement. During rotation 
the schematic style is used to give decent performance. During 
a rotation a 3-d axis appears indicating the direction of view. 
At this time rotations are about the origin at (0,0,0). 
Because rotations are non commutative, returning to the original 
position will not return you to the original orientation. However, 
WHILE THE MOUSE IS PRESSED, you can use various keys to impose 
fixed rotations. 
 
:kbd:`z` or :kbd:`space`:
    returns to the original xy-plane view with model z-axis 
    out of the screen. 
 
:kbd:`x`:
    model x-axis into screen. 
 
:kbd:`y` or :kbd:`a`:
    model y-axis into screen (the a is near the shift and control 
    keys and its a difficult reach to use y sometimes). 
 
:kbd:`X`, :kbd:`Y`, :kbd:`Z`, :kbd:`A`:
    rotate about the indicated (view) axis by 10 degrees. 
 
:kbd:`^X`, :kbd:`^Y`, :kbd:`^Z`, :kbd:`^A`:
    rotate in the opposite direction by 10 degrees. 
    The repeat key property during a key press causes the 
    appearance of a rotational velocity. 
 

ShapeStyle
~~~~~~~~~~

Show Diam:
    Shape is drawn using trapezoids between 3-d points. If the 
    view is large and the diameters small not all of them may 
    appear. (slowest) 
 
Centroid:
    Shape is drawn as lines between 3-d points. 
 
Schematic:
    Shape is drawn as line between first and last 3-d point 
    of each section. (fastest) 
 

PlotWhat
~~~~~~~~

The default plot variable is v. 
A browser pops up with all the range variables that can be plotted 
as a function of position on the shape. At this time only range 
variables can be plotted, not arbitrary functions. The PlotShape 
scene has a label indicating the chosen variable. A small bug prevents 
label from being updated since it is occluded by the dialog box. 
Refreshing (or any other drawing action) will update the label. 

VariableScale
~~~~~~~~~~~~~

A field editor dialog pops up. Enter the scale as two space separated 
numbers (first less than the second). 
This range scale is used for the color scale. Purple is the 
first number, yellow is the second number, and 20 colors in between are 
intermediate values. This scale is also used to construct the default 
y axis for time and space plots of the variable. 

TimePlot
~~~~~~~~

Press a location on the shape and a graph will be created in which 
the range variable at that location will be plotted. You can select 
several locations to add more plots to the graph. The sections 
selected are highlighted in different colors. A new graph will be 
created if you select another tool and then come back to this one. 

SpacePlot
~~~~~~~~~

Press the left mouse button at one location and drag the mouse to 
another location on the cell. A graph will be created which contains 
a range variable plot between the two selected locations. (Actually 
the nearest ends (arc position 0 or 1) of the sections of the 
selected locations. The path is highlighted). A range variable plot 
is like a movie. 
The variable plotted does not have to exist at each section in the path. 
It is plotted as 0 where it does not exist. (Range variables, 
except v, do not exist at arc positions 0 and 1. At these points the 
value plotted is that just interior to the section.) 

ShapePlot
~~~~~~~~~

A color scale is used to show the value of the variable on the 
shape. Purple is low, yellow is high. The colorbar appears in the top 
left part of the view when you are in this mode. To change its location, 
back to the top left, manipulate the view then toggle to another 
plot style and back to shape or else change the scale. 
Performance could be better. 

MoveText
~~~~~~~~

Drag the variable label to another location. 
 

