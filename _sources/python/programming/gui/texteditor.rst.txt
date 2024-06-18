.. _tedit:

         
TextEditor
----------



.. class:: TextEditor


    Syntax:
        ``e = h.TextEditor()``

        ``e = h.TextEditor(string)``

        ``e = h.TextEditor(string, rows, columns)``


    Description:
        For editing or displaying multiline text. Default is 5 rows, 30 columns. 

    .. warning::
        At this time no scroll bars or even much functionality. Mouse editing 
        and emacs style works. 

         

----



.. method:: TextEditor.text


    Syntax:
        ``string = e.text()``

        ``string = e.text(string)``


    Description:
        Returns the text of the TextEditor in a strdef. If arg exists, replaces 
        the text by the string and returns the new text (string). 

         

----



.. method:: TextEditor.readonly


    Syntax:
        ``boolean = e.readonly()``

        ``boolean = e.readonly(boolean)``


    Description:
        Returns True if the TextEditor in read only mode. 
        Returns False if text entry by the user is allowed. 
        Change the mode with the argument form using False (or 0) or True (or 1). 
        
        Prior to NEURON 7.6, this method returned 0 or 1 instead of False or True.

         

----



.. method:: TextEditor.map


    Syntax:
        ``e.map()``

        ``e.map(title)``

        ``e.map(title, left, bottom, width, height)``


    Description:
        Map the text editor onto the screen at indicated coordinates with 
        indicated title bar. 

        Note: title is a string. 

    .. image:: ../../images/texteditor-map.png
       :align: center
        


