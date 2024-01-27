
.. _hoc_panel:

         
Widgets
-------

The following are implemented as hoc functions. They are used to create 
panels of buttons, menus, and field editors. 
 

----



.. hoc:function:: xpanel


    Syntax:
        ``xpanel("name")``

        ``xpanel("name", [0-1])``

        ``xpanel()``

        ``xpanel(x, y)``

        ``xpanel(scroll)``

        ``xpanel(scroll, x, y)``


    Description:
         


        ``xpanel("name")`` 

        ``xpanel("name", [0-1])`` 
            Title of a new panel. Every 
            button, menu, and value between this and a closing ``xpanel()`` command 
            with no arguments (or placement args) belongs to this panel. 
            If the form is used with a second argument equal to 1, then 
            the panel is laid out horizontally. Otherwise the default is vertically. 

        ``xpanel()`` 

        ``xpanel(x, y)`` 
            done constructing the panel. so map it to the screen with position 
            optionally specified. 

        ``xpanel(slider)`` 

        ``xpanel(slider, x, y)`` 
            as above but if the first arg is a number, then the value determines 
            whether the panel will be inside a scrollbox. Scroll = 0 means a scrollbox 
            will NOT be used. Scroll = 1 means the panel will be inside a scrollbox. 
            Scroll = -1 is the default value and whether or not a scrollbox is used 
            is determined by the number of panel items in comparison with the 
            value of the panel_scroll property in the nrn.defaults file. 


         
         

----



.. hoc:function:: xbutton


    Syntax:
        ``xbutton("command")``

        ``xbutton("prompt", "command")``


    Description:


        ``xbutton("command")`` 
            new button with command to execute when pressed. The label 
            on the button is "*command*". 

        ``xbutton("prompt", "command")`` 
            the label ont the button is "*prompt*", the action 
            to execute is "*command*". 


         

----



.. hoc:function:: xstatebutton


    Syntax:
        ``xstatebutton("prompt",&var [,"action"])``


    Description:
        like :hoc:func:`xbutton`, but when pressed var is set to 0 or 1 so that it matches the
        telltale state of the button. If the var is set by another way the 
        telltale state is updated to reflect the correct value. 

         

----



.. hoc:function:: xcheckbox


    Syntax:
        ``xcheckbox("prompt",&var [,"action"])``


    Description:
        like :hoc:func:`xstatebutton`, but checkbox appearance.

         

----



.. hoc:function:: xradiobutton


    Syntax:
        ``xradiobutton("name", "action")``

        ``xradiobutton("name", "action", 0or1)``


    Description:
        Like an ``xbutton`` but highlights the most recently selected 
        button of a contiguous group (like a car radio, mutually exclusive 
        selection). 
        If the third argument is 1, then the button will be selected when the 
        panel is mapped onto the screen. However, in 
        this case the action should also be explicitly executed by the programmer. 
        That is not done automatically since it is often the case that the action 
        is invalid when the radio button is created. 

    Example:

        .. code-block::
            none

            proc a() { 
                print $1 
            } 
             
            strdef label, cmd 
             
            xpanel("panel") 
                xmenu("menu") 
                for i =1, 10 { 
                    sprint(label, "item %d", i) 
                    sprint(cmd, "a(%d)", i) 
                    xradiobutton(label, cmd) 
                } 
                xmenu() 
            xpanel() 

         

         

----



.. hoc:function:: xmenu


    Syntax:
        ``xmenu("title")``

        ``xmenu()``

        ``xmenu("title", 1)``

        ``xmenu("title", "stmt")``

        ``xmenu("title", "stmt", 1)``


    Description:


        ``xmenu("title")`` 
            create a button in the panel with label "title" which, when 
            pressed, pops up a menu containing buttons and other menus. Every 
            ``xbutton`` and ``xmenu`` command between this and the closing ``xmenu()`` 
            command with no arguments becomes the menu. 
            Don't put values into menus. 

        ``xmenu()`` 
            done defining the menu. Menus can be nested as in 

            .. code-block::
                none

                	xmenu("one") 
                	  xmenu("two") 
                	  xmenu() 
                	xmenu() 


        ``xmenu("title", 1)`` 
            adds the menu to the menubar. Note that a top level menu with no 
            second argument starts a new menubar. Normally these menubars have only 
            one top level item. 

            .. code-block::
                none

                xpanel("menubar") 
                	xmenu("first") 
                		xbutton("one","print 1") 
                		xbutton("two","print 2") 
                	xmenu() 
                	xmenu("second", 1) 
                		xbutton("three","print 3") 
                		xbutton("four","print 4") 
                		xmenu("submenu") 
                			xbutton("PI", "print PI") 
                		xmenu() 
                	xmenu() 
                	xmenu("third", 1) 
                		xbutton("five","print 5") 
                		xbutton("six","print 6") 
                	xmenu() 
                	xmenu("nextline") 
                		xbutton("seven","print 7") 
                		xbutton("eight","print 8") 
                	xmenu() 
                xpanel() 


        ``xmenu("title", "stmt")`` and ``xmenu("title", "stmt", 1)`` 
            Dynamic menu added as item in panel or menu or (when third argument 
            is 1) to a menubar. An example of the first type is the 
            NEURONMainMenu/File/RecentDir and an example of the last type is the 
            NEURONMainMenu/Window 
             
            When the menu title button is selected, the stmt is executed in a context 
            like: 

            .. code-block::
                none

                	xmenu("title") 
                	stmt 
                	xmenu() 

            which should normally build a menu list and then this list is mapped to 
            the screen as a normal walking menu. 
             

            .. code-block::
                none

                load_file("nrngui.hoc") 
                xpanel("test") 
                xmenu("dynamic", "make()") 
                xpanel() 
                 
                strdef s1, s2 
                n = 0 
                 
                proc make() {local i 
                   n += 1 
                   for i=1, n { 
                      sprint(s1, "label %d", i) 
                      sprint(s2, "print %d", i) 
                      xbutton(s1, s2) 
                   } 
                } 
                 



         

----



.. hoc:function:: xlabel


    Syntax:
        ``xlabel("string")``


    Description:
        Show the string as a fixed label. 

         

----



.. hoc:function:: xvarlabel


    Syntax:
        ``xvarlabel(strdef)``


    Description:
        Show the string as its current value. 

         

----



.. hoc:function:: xvalue


    Syntax:
        ``xvalue("variable")``

        ``xvalue("prompt", "variable" [, boolean_deflt, "action" [, boolean_canrun, boolean_usepointer]])``

        ``xvalue("prompt", "variable", 2)``


    Description:


        ``xvalue("variable")`` 
            create field editor for variable 

        ``xvalue("prompt", "variable" [, boolean_deflt, "action" [, boolean_canrun, boolean_usepointer]])`` 
            create field editor for variable with the button labeled with "*prompt*". 
            If *boolean_deflt* == 1 then add a checkbox which is checked when the 
            value of the field editor is different that when the editor was 
            created. Execute "action" when user enters a new value. If 
            *boolean_canrun* == 1 then use a default_button widget kit appearance 
            instead	of a push_button widget kit appearance. 
            If *boolean_usepointer* is true then (for efficiency sake) try to 
            use the address of variable instead of interpreting it all the time. 
            At this time you must use the address form if the button is created 
            within an object, otherwise when the button is pressed, the symbol 
            name won't be parsed within the context of the object but at the 
            top-level context. 

        ``xvalue("prompt", "variable", 2)`` 
            a field editor that keeps getting updated every 10th ``doNotify()``. 

        The domain of values that can be entered by the user into a field editor 
        may be limited to the domain specified by the 
        :hoc:func:`variable_domain` function , the domain specified for the variable in
        a model description file, or a default domain that exists 
        for some special NEURON variables such as diam, Ra, L, etc. 
        For a field editor to check the domain, domain limits must be in effect 
        prior to creation of the field editor. 

         

----



.. hoc:function:: xpvalue


    Syntax:
        ``xpvalue("variable")``

        ``xpvalue("prompt", &variable, ...)``


    Description:
        like :hoc:func:`xvalue` but definitely uses address of the variable.

    .. seealso::
    
        :hoc:func:`units`
         

----



.. hoc:function:: xfixedvalue


    Syntax:
        ``xfixedvalue("variable")``

        ``xfixedvalue("prompt", "variable", boolean_deflt, boolean_usepointer)``


    Description:
        like xvalue but cannot be changed by the user except under 
        program control and there can be no action associated with it. 
        Note: this is not implemented. For now, try to do the same thing 
        with ``xvarlabel()``. 

         

----



.. hoc:function:: xslider


    Syntax:
        ``xslider(&var, [low, high], ["send_cmd"], [vert], [slow])``


    Description:
        Slider which is attached to the variable var. Whenever the slider 
        is moved, the optional *send_cmd* is executed. The default range is 
        0 to 100. Steppers increase or decrease the value by 1/10 of the range. 
        Resolution is .01 of the range. vert=1 makes a vertical slider and 
        if there is no *send_cmd* may be the 4th arg. slow=1 removes the "repeat 
        key" functionality from the slider(and arrow steppers) and also 
        prevents recursive calls to the *send_cmd*. This is necessary if 
        a slider action is longer than the timeout delay. Otherwise the 
        slider can get in a state that appears to be an infinite loop. 
        The downside of slow=1 is that the var may not get the last value 
        of the slider if one releases the button during an action. 

----


.. hoc:function:: units

    Syntax:
        ``current_units = units(&variable)``

        ``current_units = units(&variable, "units string")``

        ``"on or off" = units(1 or 0)``

        ``current_units = units("varname", ["units string"])``

    Description:
        When units are on (default on) value editor buttons display the units 
        string (if it exists) along with the normal prompt string. Units for 
        L, diam, Ra, t, etc are built-in and units for membrane mechanism variables 
        are declared in the model description file. See modlunit . 
        Note that units are NOT saved in a session. Therefore, any user defined 
        variables must be given units before retrieving a session that shows them 
        in a panel. 
         
        The units display may be turned off with \ ``units(0)`` or by setting the 
        \ ``*units_on_flag: off`` in the nrn/lib/nrn.defaults file. 
         
        \ ``units(&variable)`` returns the units string for any 
        variable for which an address can be taken. 
         
        \ ``units(&variable, "units string")`` sets the units for the indicated 
        variable. 
         
        If the first arg is a string, it is treated as the name of the variable. 
        This is restricted to hoc variable names of the style, "name", or "classname.name". 
        Apart from the circumstance that the string arg style must be used when 
        executed from Python, a benefit is that it can be used when an instance 
        does not exist (no pointer to a variable of that type). 
        If there are no units specified for the variable name, or the variable 
        name is not defined, the return value is the empty string. 

    Example:

        .. code-block::
            none

            units(&t) // built in as "ms" 
            units("t") 
            units("ExpSyn.g") // built in as "uS" 
            x = 1 
            {units(&x, "mA/cm2")}	// declare units for variable x 
            units(&x)		// prints mA/cm2 
            proc p () { 
            	xpanel("Panel") 
            	xvalue("t") 
            	xvalue("prompt for x", "x", 1) 
            	xpanel() 
            } 
            p()		//shows units in panel 
            units(0) 	// turn off units 
            p()		// does not show units in panel 

    .. warning::
        In the Python world, the first arg must be a string as the pointer style will 
        raise an error. 


