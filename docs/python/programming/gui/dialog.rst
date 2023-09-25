Dialog Boxes
------------

.. function:: boolean_dialog

    Syntax:
    
        .. code-block::
            python
            
            h.boolean_dialog("label", ["accept", "cancel"])

    Description:
        Pops up a dialog window at the center of the screen and blocks
        everything until dealt with.
        Returns 1 or 0.
        
    Example:
    
        .. code-block::
            python
            
            from neuron import h, gui

            if h.boolean_dialog('Do you prefer to code in Python or HOC?', 'Python', 'HOC'):
                print('You prefer Python!')
            else:
                print('You prefer HOC!')

    .. image:: ../../images/boolean_dialog.png
        :align: center

.. seealso::
    :class:`SymChooser`, :meth:`VBox.dialog`

----

.. function:: continue_dialog

    Syntax:
    
        .. code-block::
            python
            
            h.continue_dialog("message")

    Description:
        Provides information to the user.
        Like :func:`boolean_dialog`, blocks everything until dealt with.

    Example:
    
        .. code-block::
            python
            
            from neuron import h, gui
            h.continue_dialog("You are reading a message from a dialog box.")
        
        .. image:: ../../images/continue_dialog.png
            :align: center

----

.. function:: string_dialog

    Syntax:
        .. code-block::
            python
            
            h.string_dialog("message", strref)
        
    Description:
        Prompts the user to enter a string. The initial value of strref is used
        as the default value.
        If canceled, returns 0 and *strref* remains unchanged.
        Otherwise, returns 1 and *strref* is replaced with the entered text.
        Like :func:`boolean_dialog`, blocks everything until dealt with.

    Example:
    
        .. code-block::
            python

            from neuron import h, gui

            my_str = h.ref('')
            if h.string_dialog('Type a string:', my_str):
                print(f'You typed: {my_str[0]}')
            else:
                print('You canceled')
                
        .. image:: ../../images/string_dialog.png
            :align: center
