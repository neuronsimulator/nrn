.. _using_session_files_for_saving_and_retrieving_windows:

Using Session Files for Saving and Retrieving Windows
=================

Saving the windows of an interface for later re-use is an essential operation. The position, size, and contents of all the windows you have constructed often represent a great deal of labor that you don't want to do over again. Losing a CellBuild or a MulRunFitter window can be a disaster.

This article explains how to save windows, how to save them in groups in separate files in order to keep separate the ideas of specification, control, graphing, optimization, etc., and how to recover as much work as possible in the event that a saved window generates an error when it is read from a file.

What is a session?
-------------

A "session" is the set of all of NEURON's graphical windows, including hidden windows.

How (and when) to save all windows to a ses file, and how to retrieve them
--------------

The simplest way to save NEURON's graphical windows is to save all of them at once with :menuselection:`NEURONMainMenu --> File --> save session`. This creates a session file which NEURON can use to recreate the windows that were saved. Session files, which are generally given the suffix ".ses" to distinguish them from user-written hoc files, are discussed below in more detail.

It's an excellent practice to save the entire session whenever a crash would cause distress because of the loss of all work since the previous save, so save early and save often. Be sure to verify that the new session file can be retrieved (:menuselection:`NEURONMainMenu --> File --> load session`) before you overwrite an earlier session file that works!

It is most useful to retrieve such a session file right after launching NEURON, when no other windows are present on the screen. It is especially useful if one of the windows is a CellBuild or NetGUI, because most windows depend on the existence of information declared by them. Conflicts can arise if there are multiple CellBuild or NetGui windows that could interfere with one another, especially if they create sections with the same names.

How (and why) to save selected windows
------------

The main problem with saving all windows in a single session file is that it mixes specification, control, parameter, and graphing windows. Experience has shown that it is best to segregate these categories in order to have more conceptual control over a modeling project. This allows you to easily start a simulation by retrieving the desired variant of a CellBuilder window, separately retrieving one of several possible stimulus protocols and parameter sets, and lastly retrieving groups of graph windows.

If your custom interface is simple, the Print & File Window Manager (PFWM) offers an easy way to save a few windows This tool is discussed elsewhere--see the FAQ list entry "Q: How do I print a hard copy of a NEURON window?" under the heading "Using NEURON--general questions". Select the windows you want to save, just as if you were going to print them. After you have chosen the windows to be saved, click on the Session button instead of the Print button, and scroll down to the item "Save selected".

A more powerful tool for managing windows is the Window Group Manager.

The Window Group Manager
++++++++++

The :menuselection:`NEURONMainMenu --> Window --> GroupManager`` enables the separate creation of such session files with only a little more effort than the bulk :menuselection:`NEURONMainMenu --> File --> save session`. With the "NewGroup" and "ChangeName" buttons one can define several named groups, e.g. "Cell", "Stimulus", "Control", and "Voltage Graphs".

The names of window groups appear in the left panel of the window group manager. If you click on one of these names, the titles of the windows that belong to it will appear in the center panel. Clicking on one of these titles removes it from the group, and the title will now appear in the right panel, which lists the ungrouped windows. To add a window to the group, just click on the window title in the right panel. You can move a window from one window group to another by following this sequence :

1.
    Select the group it is currently in.

2.
    Select its window title. (Makes it ungrouped)

3.
    Select the group you want it to be in.

4.
    Select its window title in the ungrouped list.

To save any window group to its own session file, click on the SaveGroup button.

The Window Group Manager is a dialog box and so must be closed before other windows will accept further input. It may be opened at any time via the :menuselection:`NEURONMainMenu --> Window --> GroupManager` menu item and groups re-saved, new windows added to existing groups, etc.

What's in a ses file
--------

A session file is actually just a sequence of hoc instructions for reconstructing the windows that have been saved to it. In a session file, the instructions for each window are identified by comments. It is often a simple matter to use a text editor to modify those instructions, e.g. change the value of a parameter, or remove all the instructions for a window if it is preventing the file from being loaded. Below we discuss how the latter can happen. There are circumstances in which a window in the current session may not easily be made to start again when NEURON is exited and relaunched.

What can go wrong, and how to fix it
------------

The most common cause of errors during retrieval of a session file is that one or more variables used by a window have not yet been defined. Thus, retrieving a point process manager window before the prerequisite cable section has been created will result in a hoc error. Retrieving a Graph of SEClamp[0].i will not succeed if SEClamp[0] does not exist. In most cases, loading the prerequisite sessions first will fix the error. To make sure that session files are loaded in the proper sequence, it can be helpful to create a file called init.hoc that contains a series of load_file statements, e.g.

.. code::
    c++

    load_file("nrngui.hoc")
    load_file("cell.ses")
    load_file("stim.ses")
    load_file("ctrl.ses")
    load_file("graf.ses")

Errors due to mismatched object IDs are easy to correct by editing the session file. Mismatched object IDs can occur from particular sequences of creation and destruction of windows by the user. For example, suppose you

1.
    Start a PointProcessManager and create the first instance of an :class:`IClamp`. This will be IClamp[0]

2.
    Start another PointProcessManager and create a second instance of an IClamp. This will be IClamp[1]

3.
    Close the first PointProcessManager. That destroys IClamp[0].

4.
    Start a graph and plot IClamp[1].i

5.
    Save the session.

If you now exit and re-launch NEURON and retrieve the session, the old IClamp[1] will be re-created as IClamp[0], and the creation of the Graph window will fail due to the invalid variable name it is attempting to define. The fix is just to edit the session file and change the IClamp[1].i string to IClamp[0].i

