Dialog Boxes
------------

.. function:: boolean_dialog

    .. tab:: Python
    
        Syntax:
    
            .. code-block::
                python
            
                n.boolean_dialog("label", ["accept", "cancel"])

        Description:
            Pops up a dialog window at the center of the screen and blocks
            everything until dealt with.
            Returns 1 or 0.
        
        Example:
    
            .. code-block::
                python
            
                from neuron import n, gui

                if n.boolean_dialog('Do you prefer to code in Python or HOC?', 'Python', 'HOC'):
                    print('You prefer Python!')
                else:
                    print('You prefer HOC!')

        .. image:: ../../images/boolean_dialog.png
            :align: center

    .. tab:: HOC


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

    .. tab:: Python
    
        Syntax:
    
            .. code-block::
                python
            
                n.continue_dialog("message")

        Description:
            Provides information to the user.
            Like :func:`boolean_dialog`, blocks everything until dealt with.

        Example:
    
            .. code-block::
                python
            
                from neuron import n, gui
                n.continue_dialog("You are reading a message from a dialog box.")
        
            .. image:: ../../images/continue_dialog.png
                :align: center

    .. tab:: HOC


        Syntax:
            ``continue_dialog("label")``
        
        
        Description:
            Provides info to the user.
            Like :func:`boolean_dialog`, blocks everything until dealt with.
        
----

.. function:: string_dialog

    .. tab:: Python
    
        Syntax:
            .. code-block::
                python
            
                n.string_dialog("message", strref)
        
        Description:
            Prompts the user to enter a string. The initial value of strref is used
            as the default value.
            If canceled, returns 0 and *strref* remains unchanged.
            Otherwise, returns 1 and *strref* is replaced with the entered text.
            Like :func:`boolean_dialog`, blocks everything until dealt with.

        Example:
    
            .. code-block::
                python

                from neuron import n, gui

                my_str = n.ref('')
                if n.string_dialog('Type a string:', my_str):
                    print(f'You typed: {my_str[0]}')
                else:
                    print('You canceled')
                
            .. image:: ../../images/string_dialog.png
                :align: center

    .. tab:: HOC


        Syntax:
            ``string_dialog("label", strdef)``
        
        
        Description:
            Returns 0 if canceled and *strdef* remains unchanged.
            Like :func:`boolean_dialog`, blocks everything until dealt with.
        
