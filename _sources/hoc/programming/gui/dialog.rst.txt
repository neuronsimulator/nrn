Dialog Boxes
------------

.. hoc:function:: boolean_dialog

    Syntax:
        ``boolean_dialog("label", ["accept", "cancel"])``

    Description:
        Pops up a dialog window at the center of the screen and blocks
        everything until dealt with.
        Returns 1 or 0.

.. seealso::
    :hoc:class:`SymChooser`, :hoc:meth:`VBox.dialog`

----

.. hoc:function:: continue_dialog

    Syntax:
        ``continue_dialog("label")``

    Description:
        Provides info to the user.
        Like :hoc:func:`boolean_dialog`, blocks everything until dealt with.


----

.. hoc:function:: string_dialog

    Syntax:
        ``string_dialog("label", strdef)``

    Description:
        Returns 0 if canceled and *strdef* remains unchanged.
        Like :hoc:func:`boolean_dialog`, blocks everything until dealt with.

