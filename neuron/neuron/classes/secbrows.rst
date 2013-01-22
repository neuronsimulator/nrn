.. _secbrows:

SectionBrowser
--------------



.. class:: SectionBrowser


    Syntax:
        :code:`sb = new SectionBrowser()`

        :code:`sb = new SectionBrowser(SectionList)`


    Description:
        Class that makes a visible list of all section names. 
        If the optional SectionList arg is present then only those 
        section names are shown in the browser. 

    .. seealso::
        :class:`SectionList`, :ref:`stdrun_shape`

         

----



.. method:: SectionBrowser.select


    Syntax:
        :code:`.select()`


    Description:
        currently accessed section is highlighted. 

         

----



.. method:: SectionBrowser.select_action


    Syntax:
        :code:`sb.select_action("command")`


    Description:
        Command is executed when an item is selected (single click or 
        dragging) by the mouse. Before execution, the selected section 
        is pushed. (and	popped after the command completes.) 
        Command is executed in the object context in which \ :code:`select_action` 
        registered it. 

    Example:

        .. code-block::
            none

            begintemplate Cell 
            public soma, dend, axon 
            create soma, dend[3], axon 
            endtemplate Cell  
              
            objref sb, cell[3] 
            for i=0,2 cell[i] = new Cell() 
            sb = new SectionBrowser() 
            sb.select_action("act()") 
              
            proc act() { 
                    printf("currently accessed section is %s\n", secname()) 
            } 


         

----



.. method:: SectionBrowser.accept_action


    Syntax:
        :code:`sb.accept_action("command")`


    Description:
        Command is executed when an item is accepted (double click) by 
        the mouse. Before execution, the selected section 
        is pushed. (and	popped after the command completes.) 
        Command is executed in the objet context in which the  \ :code:`accept_action` 
        registered it. 

    Example:

        .. code-block::
            none

            create soma, dend[3], axon 
            objref sb 
            sb = new SectionBrowser() 
            sb.accept_action("act()") 
             
            proc act() { 
            	printf("currently accessed section is %s\n", secname()) 
            } 



