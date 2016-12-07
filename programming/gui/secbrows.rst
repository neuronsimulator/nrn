.. _secbrows:

SectionBrowser
--------------



.. class:: SectionBrowser


    Syntax:
        ``sb = h.SectionBrowser()``

        ``sb = h.SectionBrowser(SectionList)``


    Description:
        Class that makes a visible list of all section names. 
        If the optional SectionList arg is present then only those 
        section names are shown in the browser. 

    Example:

        .. code-block::
            python

            from neuron import h, gui
            soma = h.Section(name='soma')
            sl = h.SectionList()
            sl.append()
            sb = h.SectionBrowser(sl)
            
    
    .. image:: ../../images/secbrows-sectionList.png
        :align: center

    .. seealso::
        :class:`SectionList`, :ref:`stdrun_shape`

         

----



.. method:: SectionBrowser.select


    Syntax:
        ``.select()``


    Description:
        currently accessed section is highlighted. 

         

----



.. method:: SectionBrowser.select_action


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
            python

            from neuron import h, gui

            class Cell:
                count = 0
                def __init__(self):
                    self.soma = h.Section(name='soma', cell=self)
                    self.dend = [h.Section(name='dend%d' % i, cell=self) for i in range(3)]
                    self.axon = h.Section(name='axon', cell=self)
                    self._id = Cell.count
                    Cell.count += 1
                def __repr__(self):
                    return 'Cell%d' % self._id

            cell = Cell()
            sb = h.SectionBrowser()


         

----



.. method:: SectionBrowser.accept_action


    Syntax:
        ``sb.accept_action("command")``


    Description:
        Command is executed when an item is accepted (double click) by 
        the mouse. Before execution, the selected section 
        is pushed. (and	popped after the command completes.) 
        Command is executed in the objet context in which the  \ ``accept_action`` 
        registered it. 

             
    #add example