.. _how_to_develop_and_debug_your_own_code:

How to develop and debug your own code
======================================

Rule I: Don't write a bunch of code (or create a bunch of GUI windows) all at once, and then expect everything to work the first time you try it.
-------------------------------------------------------------------------------------------------------------------------------------------------

Algorithm for developing code that works:

    1. 
        First, break the problem into a sequence of small tasks that can be implemented and tested, one at a time.
    
    2. 
        Pick the first task.

    3. 
        Write the code, or create the GUI tool, that accomplishes this task.
        If this involves anything that you aren't absolutely sure about,
        read the relevant entries in Programmer's Reference, 
        and/or use the GUI on a toy problem to make sure you know what you're doing.

    4. 
        Repeat. Test your program to see if it works. If it doesn't work, re-read the Programmer's Reference and revise the code (or work with the GUI tool) until your program works.

    5. 
        If no more tasks remain, your job is done.
        Otherwise, pick the next task and go to step 3.


Rule II: Learn how to debug your own code.
------------------------------------------

Algorithm for debugging code:

    1. 
        Try to break the code into chunks that accomplish a sequence of small tasks.

    2. 
        Comment out all of the code except for the little bit that accomplishes the first task.

    3. 
        Repeat.  
        Test the code to see if it works.
        If it doesn't work, re-read the Programmer's Reference, then revise the code
        until the code works.

    4. 
        If no more chunks of code remain, your job is done.
        Otherwise, uncomment the next chunk of code, and go to step 3.


The Python Debugger: pdb
-------------

NEURON models implemented in Python may also be examined using `Python's built-in debugger pdb <https://docs.python.org/3/library/pdb.html>`_.






