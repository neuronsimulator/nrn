.. _exit-neuron-no-gui:

How do I exit NEURON? I'm not using the GUI, and when I enter ^D at the oc> prompt it doesn't do anything?
-----------------------------------------------------------------------------------------------

You seem to be using an older MSWin or MacOS version of NEURON (why not get the most recent version?). Typing the command

quit()

at the oc> prompt works for all versions, new or old, under all OSes. Don't forget the parentheses, because quit() is a function. Oh, and you need to press the Enter or Return key too.


