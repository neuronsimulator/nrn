Dialog Boxes
------------

.. function:: boolean_dialog

    Syntax:
        ``boolean_dialog("label", ["accept", "cancel"])``

    Description:
        Pops up a dialog window at the center of the screen and blocks
        everything until dealt with.
        Returns 1 or 0.

.. seealso::
    :class:`SymChooser`, :meth:`VBox.dialog`

----

.. function:: continue_dialog

    Syntax:
        ``continue_dialog("label")``

    Description:
        Provides info to the user.
        Like :func:`boolean_dialog`, blocks everything until dealt with.


----

.. function:: string_dialog

    Syntax:
        ``string_dialog("label", strdef)``

    Description:
        Returns 0 if canceled and *strdef* remains unchanged.
        Like :func:`boolean_dialog`, blocks everything until dealt with.

