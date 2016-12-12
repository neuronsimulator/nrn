.. _predec:

.. _predeclared-variables:

Predeclared Variables
---------------------





.. data:: hoc_ac_

    Syntax:
        ``h.hoc_ac_``

        A variable used by the graphical interface to communicate with the 
        interpreter. It is very volatile. It sometimes holds a value on a 
        function call. If this value is needed by the user it should be 
        copied to another variable prior to any other function call. 

----



.. data:: hoc_obj_


    Syntax:
        ``h.hoc_obj_[0]``

        ``h.hoc_obj_[1]``


    Description:
        When a line on a :class:`Graph` is picked with the :ref:`gui_pickvector` tool 
        two new :class:`Vector`\ 's are created containing the y and x coordinates of the 
        line. The y vector is referenced by hoc_obj_[0] and the x vector is 
        referenced by hoc_obj_[1]. 


----



.. data:: hoc_cross_x_


    Syntax:
        ``h.hoc_cross_x_``


    Description:
        X coordinate value of the last :ref:`graph_crosshair` manipulation. 

    .. seealso::
        :ref:`graph_crosshair`


----



.. data:: hoc_cross_y_

    Syntax:
        ``h.hoc_cross_y_``

    Description:
        Y coordinate value of the last :ref:`graph_crosshair` manipulation. 

    .. seealso::
        :ref:`graph_crosshair`




