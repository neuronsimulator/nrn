.. _compiling_new_mechanisms_under_macos:

Compiling new mechanisms under MacOS
================

This is based on the text file `ftp://hines.med.yale.edu/neuron/mac/mknrndll/README`.

Test mknrndll
--------

The mknrndll script, model description translator (nmodl68k), and units checker (modlunit68k), are in the same folder as the neuron application. They were created when you installed NEURON.

To see if everything is installed properly, double click on mknrndll. This will make a window appear that describes mknrndll. Click on this window's OK button and it will go away. If anything is missing, another window will pop up that tells you what is needed.

If something is missing
------------

Follow the steps in the `Using NMODL files <https://nrn.readthedocs.io/en/latest/courses/using_nmodl_files.html?highlight=mknrndll>`_ page.

mknrndll.hqx is a self-extracting stuffit archive is independent of NEURON and can be extracted into any folder name.

Among other things, mknrndll.hqx contains several items that can also be obtained from http://www.apple.com/developer/, including the MrC compiler, PPCLink, ToolServer, CIncludes, SharedLibraries stubs, and the "Scriptable Text Editor."

If you already have the "Scriptable Text Editor" just throw this one into the trash.

Now test mknrndll again.
++++++++++++++++

An aside:

If you already have the ToolServer and you want to get rid of this one, things are a bit more complicated since the mknrndll applescript that creates nrnmac.dll files from your .mod files thinks MrC, PPCLink, CIncludes, and SharedLibraries are all in the folder of the folder containing ToolServer. So just make sure these are in the relation

.. code::
    Python

    folder
            folder
                ToolServer
            MrC
            PPCLink
            CIncludes
            SharedLibraries

Then check things out by running the ToolServer application by itself.

Using mknrndll to create a nrnmac.dll file
---------------

To create a nrnmac.dll file suitable for loading into NEURON, drag a set of .mod files or a folder containing .mod files onto the mknrndll script icon. After a minute or so the dll file should appear in the folder containing the first .mod file found. If you double click on that nrnmac.dll file or drag it onto the neuron application, neuron should run and load the mechanisms in that dll file.

Note: .mod files have to be type, TEXT.

How this all works
------------

Of course a lot can go wrong between dragging the .mod files and getting a working nrnmac.dll file. To deal with the common problems it is important to know what the mknrndll script is doing.

1.
    create "mknrndll data" folder in the folder containing the first .mod file. This is where the translated .c files, compiled .o files, a toolserver script, and the error message files are kept.

2.
    translate all the .mod files (modified after existing .c files) into .c files and store them in the "mknrndll data" folder. This will fail if the .mod files are not of type 'TEXT'. Dragging a directory onto the binary2text program in the nrn installation directory recursively converts all files to type 'TEXT'

3.
    create a toolserver script to compile and link the .c files into a nrnmac.dll shared library and put the script in the "mknrndll data" folder.

4.
    run the script. You know it fails if the toolserver doesn't go away by itself. If it doesn't, kill it and look at the messages in the error file in the "mknrndll data" folder. (I look at them with SimpleText).

Good luck. And ask questions if you have problems. michael.hines@yale.edu

FAQ
===

**Q:** When I double click on my nrnmac.dll file, NEURON hangs. It seems to work properly if I drag it onto the neuron application.

**A:** The double click is launching an older version of neuron. Get rid of all previous versions on your machine.

**Q:** I have MrC, CIncludes, PPCLink, and SharedLibraries in the folder containing "Toolserver Folder" but it still can't find one of those.

**A:** There must be another ToolServer application on your machine and that is the one being launched by the mknrndll script. Use the finder to find it and put the above in proper relation to it. Then throw my "ToolServer Folder" into the trash.

**Q:** The "mknrndll data" folder is created but no files get into it.

**A:** The .mod files must be of type TEXT. Can you edit them?

**Q:** The mknrndll script is not launching ToolServer.

**A:** Try running the ToolServer application by itself. Perhaps it will complain that it can't find StdCLib. If so, and you are running MPW on a Power Macintosh with a version of system software prior to MacOS 7.6, then copy the StdCLibInit file from the "Required for PreMac OS 7.6" folder to the Extensions folder of your System Folder. Then reboot your system.


