.. _tedit:

         
TextEditor
----------



.. class:: TextEditor


    Syntax:
        :code:`e = new TextEditor()`

        :code:`e = new TextEditor(string)`

        :code:`e = new TextEditor(string, rows, columns)`


    Description:
        For editing or displaying multiline text. Default is 5 rows, 30 columns. 

    .. warning::
        At this time no scroll bars or even much functionality. Mouse editing 
        and emacs style works. 

         

----



.. method:: TextEditor.text


    Syntax:
        :code:`string = e.text()`

        :code:`string = e.text(string)`


    Description:
        Returns the text of the TextEditor in a strdef. If arg exists, replaces 
        the text by the string and returns the new text (string). 

         

----



.. method:: TextEditor.readonly


    Syntax:
        :code:`boolean = e.readonly()`

        :code:`boolean = e.readonly(boolean)`


    Description:
        Returns 1 if the TextEditor in read only mode. 
        Returns 0 if text entry by the user is allowed. 
        Change the mode with the argument form using 0 or 1. 

         

----



.. method:: TextEditor.map


    Syntax:
        :code:`e.map()`

        :code:`e.map(title)`

        :code:`e.map(title, left, bottom, width, height)`


    Description:
        Map the text editor onto the screen at indicated coordinates with 
        indicated title bar 


