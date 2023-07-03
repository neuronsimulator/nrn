
.. _hoc_pointman:


Managers
--------


PointManager
~~~~~~~~~~~~

Starts a general purpose :ref:`hoc_pointprocessmanager` for specifying a
location and defining what kind of point process should exist 
there. Any number of these managers can exist simultaneously. 
It is more general than the Electrode below in the sense that 
you can choose any point process and has a much cleaner 
implementation. 

PointGroup
~~~~~~~~~~

Starts a :ref:`hoc_PointProcessGroupManager` for managing a collection of
related point processes. If all the members of the collection are 
of the same type, then the values of their variables can be changed 
"globally" and all the values of a single parameter can be displayed. 

Electrode
~~~~~~~~~

Starts a general purpose :hoc:class:`Electrode`
voltage/current clamp (with some 
voltage clamp families) that can be moved to any position in any 
section. Any number of these may be present simultaneously. When 
one is dismissed from the screen, it is also removed from the neuron 
and the point processes it manages are destroyed. This widget is 
pretty much superseded by the :ref:`hoc_pointprocessmanager`.

Viewers
-------


PointProcesses
~~~~~~~~~~~~~~

Menu of possible pointprocess's. Selecting an item pops up a panel 
that contains a browser of the locations of a particular type of 
point process, and, if global parameters exist, a button for popping 
up a panel showing the global parameters for this type of point 
process. Double clicking a location on the browser pops up a panel 
showing the values for a particular point process instance. 
See :ref:`hoc_mech` for details about built-in point
processes. The corresponding .mod file must in general be 
examined in order to understand the particulars about a given 
point process type. 
     

.. _hoc_pointprocessmanager:

PointProcessManager
-------------------

     
Create a Point Process of a particular type at a particular location. 
Each instance of a PointProcessManager manages a single point process. 
 
The items in the :guilabel:`SelectPointProcess` menu are used to specify the 
type of the point process. After the selection, the :guilabel:`Show` menu 
is used to make the lower portion of the panel to display either the "Shape" 
(to indicate the location with a blue dot) 
or "Parameter" values of the point process. The type and location of 
the point process are also displayed in the upper portion of the panel. 
The location is changed by selecting the "Section" tool in the shape 
scene (right mouse button) and then clicking on a location (left mouse 
button). 
 
Note that when one point process is replaced by another 
the parameters are saved in a :hoc:class:`MechanismStandard`. When
the point process is re-installed, those parameters are restored. 
 
If the panel is saved in a session, the MechanismStandard's are 
saved as well. 
 
hoc usage:

.. code-block::
    none
    
    section p = new PointProcessManager([xplacement, yplacement]) 
    
p.pp is the point process currently installed in the cell. 
     

.. seealso::
    :ref:`hoc_mech`


.. _hoc_pointprocessgroupmanager:

PointProcessGroupManager
------------------------

     
Specify point process types, locations, and values for a set of 
point processes. Although the set may consist of different types, if 
all the types are the same then variable values can be changed "globally" 
for all pointprocesses in the set, and all values of a single parameter 
can be displayed at once. 
 
The panel consists of a control area at the top and a horizontally 
arrangement of three subpanels on the bottom. 
 
The middle subpanel (list browser) 
shows the names of the point processes in the managed set. Select 
a name by clicking on it. 
 
The left subpanel (shape scene) shows the locations of 
all the point processes in the list marked as blue dots. The selected 
name is marked as a red dot. When the "Section" tool of the shape 
scene menu (right mouse button) is selected, clicking on a location 
on the neuron will move the selected point process (red dot, highlighted 
name) to that location. The label in the control area shows the name 
and location of the selected point process. 
 
The right subpanel shows parameters in one of three styles determined 
by the :guilabel:`PanelStyle` menu. :guilabel:`ViewSelection` shows all the parameters for 
the selected (red dot, highlighted name) point process. The name of 
the selected point process is also shown at the top of this subpanel. 
 
The remaining two "PanelStyle" items work only if all the point processes 
in the list are of the same type. 
 
:guilabel:`GlobalSpec` is similar to :guilabel:`ViewSelection` but any change to a parameter 
(or clicking on a value button) causes that value to be assigned to 
all the point process of the list. Note that NO assignments are made 
when the :guilabel:`GlobalSpec` panel is constructed. The user must press 
a value button or enter a new value into the field editor. This helps 
prevent accidental changing of values in the individual point processes. 
The default values in the global spec panel are those values in the 
currently selected name. In this mode, the top of the right subpanel 
shows the label: "All PP's set to these values". 
 
When the :guilabel:`ViewOneName` item of the :guilabel:`PanelStyle` menu is selected 
the right submenu shows a list of names of parameters. Selecting 
one of these names shows this parameter for all the managed point 
processes (each button label indicates which point process is 
referenced). 
 
In the control portion of PointProcessGroupManager, the :guilabel:`New` menu 
is used to add a point process of the indicated type to the 
list of managed point processes. The :guilabel:`Remove` button destroys the 
selected point process. The :guilabel:`Copy` button clones the selected 
point process 
 

