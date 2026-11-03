This directory should host script files to be added to the bin path of the
loaded Python interpreter.
In the case of NEURON binary executables, we just want a wrapper which sets the
correct environmental variables, which is why the binwrapper.py file exists.
See the CMakeLists.txt file in the same directory for a list of scripts which
are installed when a NEURON Python wheel is installed.
