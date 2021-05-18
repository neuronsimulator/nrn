.. _shapebox:


MenuExplore
-----------

Shape and browser for creating a section panel 

.. index:: MenuExpore (HOC class)

usage:
    ``ob = h.MenuExplore()``
 
Pressing the left mouse button on a section in the Shape scene (make 
sure the :guilabel:`Section` popup menu item is selected) colors the section red and 
highlights the corresponding section name in the browser. 
 
Pressing the left mouse button once on a name in the browser colors 
the corresponding section in the Shape Scene (you can drag the mouse as well). 
 
Double clicking the left mouse button on a name in the browser pops up 
a panel showing the parameters associated with that section. 
 
This assumes homogeneous sections by default. So Parameters which are not 
initialized as constants along the section get a label to that effect instead 
of a field editor.  If this is not desired specify a position, 0 < x < 1, 
for the location. 
 
If panels besides PARAMETER are desired select from the :guilabel:`Type` menu. 
     

PointProcessLocator
~~~~~~~~~~~~~~~~~~~

.. index:: PointProcessLocator (HOC class)

shape, browser, and menu for a point process 

usage:
    :samp:`ob = h.PointProcessLocator({pointprocess})` 
 
In the shape scene there is a blue dot showing the location of the 
pointprocess.  Press the left mouse button at any point on the neuron to 
relocate the point process. Parameters of the point process are 
displayed in the lower part of the window. 
 

ShapeBrowser
~~~~~~~~~~~~

Pressing the left mouse button on a section in the Shape scene (make 
sure the :guilabel:`Section` popup menu item is selected) colors the section red and 
highlights the corresponding section name in the browser. 
 
Pressing the left mouse button once on a name in the browser colors 
the corresponding section in the Shape Scene (you can drag the mouse as well). 
 
This class is useful as a part of a larger class and as a dialog for 
selecting a section. 
 

