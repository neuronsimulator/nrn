.. _seclamp-iclamp-arbitrary-waveform:

SEClamp and IClamp just deliver rectangular step waveforms. How can I make them produce an arbitrary waveform, e.g. something that I calculated or recorded from a real cell?
-----------------------------------------------------------------------------------------------------------------------------

The Vector class's play method can be used to drive any variable with a sequence of values stored in a Vector. For example, you can play a Vector into an IClamp's amp, an SEClamp's amp1, an SEClamp's series resistance rs (e.g. if you have an experimentally measured synaptic conductance time course). To learn how to do this, get :download:`data/vecplay.hoc`, which contains an exercise from one of our 5-day hands-on NEURON courses. Unzip it in an empty directory. This creates a subdirectory called :file:`vectorplay`, where you will find a file called :file:`arbforc.html`

Open this file with your browser and start the exercise.

