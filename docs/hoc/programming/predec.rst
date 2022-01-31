
.. _hoc_predec:


.. _hoc_predeclared-variables:

Predeclared Variables
---------------------





.. hoc:data:: hoc_ac_

        A variable used by the graphical interface to communicate with the 
        interpreter. It is very volatile. It sometimes holds a value on a 
        function call. If this value is needed by the user it should be 
        copied to another variable prior to any other function call. 

----



.. hoc:data:: hoc_obj_


    Syntax:
        ``hoc_obj_[0]``

        ``hoc_obj_[1]``


    Description:
        When a line on a :hoc:class:`Graph` is picked with the :ref:`hoc_gui_pickvector` tool
        two new :hoc:class:`Vector`\ 's are created containing the y and x coordinates of the
        line. The y vector is referenced by hoc_obj_[0] and the x vector is 
        referenced by hoc_obj_[1]. 


----



.. hoc:data:: hoc_cross_x_


    Syntax:
        ``hoc_cross_x_``


    Description:
        X coordinate value of the last :ref:`hoc_graph_crosshair` manipulation.

    .. seealso::
        :ref:`hoc_graph_crosshair`


----



.. hoc:data:: hoc_cross_y_


    Description:
        Y coordinate value of the last :ref:`hoc_graph_crosshair` manipulation.

    .. seealso::
        :ref:`hoc_graph_crosshair`




