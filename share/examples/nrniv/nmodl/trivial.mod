NEURON{ SUFFIX nothing}

COMMENT

This is the most trivial mechanism possible. It will translate into
boilerplate C code with empty structures and mechanism calls. In principle
one can use this code by adding your own code to fill in the structures and
write functions to communicate with the NEURON user level.

The first statement used to be required by the model language
definition.  In a NEURON context it doesn't do anything but MODL
requires it in order to specify the plot range for the independent
variable and the number of points to plot.  One can leave it out if the
model will only be used with neuron (linking with nrnocmodl or
nrnivmodl) and will not be used as a standalone model linked only with
the interpreter (ocmodl, ivmodl). 

The second statement turns off automatic suffixes which are normally added to
every variable and function name at the user lever of NEURON.  Without it
all variables and functions declared in the model (except special variables
known to NEURON such as v, celsius, dt, etc.) would get the suffix, _trivial,
from the base name of the file.  By the way, this model generates an empty
mechanism called "trivial" which can be "insert"ed into a section.

The reason, trivial mechanisms like this can be useful is to write
functions callable from NEURON which have nothing to do with membrane
mechanisms per se. For example, one can link in a fast fourier transform
function.

ENDCOMMENT

