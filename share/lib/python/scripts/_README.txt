This directory should host script file to be automatically added to
the bin path of the loaded Python interpreter

In the case if binary files to which we just want a wrapper the _binwrapper.py 
file exists. You should only create a softlink with the target bin name
e.g. `ln -s _binwrapper.py nrniv` will create a wrapper script which launches
nrniv found in the distributed package resources .data/lib

