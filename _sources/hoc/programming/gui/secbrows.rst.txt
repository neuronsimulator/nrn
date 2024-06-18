
.. _hoc_secbrows:

SectionBrowser
--------------



.. hoc:class:: SectionBrowser


    Syntax:
        ``sb = new SectionBrowser()``

        ``sb = new SectionBrowser(SectionList)``


    Description:
        Class that makes a visible list of all section names. 
        If the optional SectionList arg is present then only those 
        section names are shown in the browser. 

    .. seealso::
        :hoc:class:`SectionList`, :ref:`hoc_stdrun_shape`

         

----



.. hoc:method:: SectionBrowser.select


    Syntax:
        ``.select()``


    Description:
        currently accessed section is highlighted. 

         

----



.. hoc:method:: SectionBrowser.select_action


    Syntax:
        ``sb.select_action("command")``


    Description:
        Command is executed when an item is selected (single click or 
        dragging) by the mouse. Before execution, the selected section 
        is pushed. (and	popped after the command completes.) 
        Command is executed in the object context in which \ ``select_action`` 
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



.. hoc:method:: SectionBrowser.accept_action


    Syntax:
        ``sb.accept_action("command")``


    Description:
        Command is executed when an item is accepted (double click) by 
        the mouse. Before execution, the selected section 
        is pushed. (and	popped after the command completes.) 
        Command is executed in the objet context in which the  \ ``accept_action`` 
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



