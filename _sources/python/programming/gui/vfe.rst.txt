.. _vfe:

ValueFieldEditor
----------------



.. class:: ValueFieldEditor


    Syntax:
        ``vfe = h.ValueFieldEditor(...)``


    Description:
        Takes exactly the same args as :func:`xvalue` or :func:`xpvalue` but is an object 
        which can change some features (default) under program control. 
        Note that when this object is created, there must be an open :func:`xpanel`. 


----



.. method:: ValueFieldEditor.default


    Syntax:
        ``vfe.default()``


    Description:
        If the field editor is a default field editor then the current value 
        becomes the default value. ie. the red checkmark is turned off. 

         

