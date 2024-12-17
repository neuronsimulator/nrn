NEURON{ SUFFIX Xmech}

COMMENT

Dummy mechanism for linking an "unknown" mechanism into neuron for
teaching and course examination purposes. 

This merely generates the hook for registering a mechanism.
The teacher can construct an unknown channel in another directory,
translate it via nocmodl, manually eliminate the registration of range
variables etc, compile it and place it in the nrnoc/SRC/$CPU directory
and create a new library.  The student's task is then to reverse
engineer the model in a circumstance in which no information specific to
the model is available from the interpreter. (except that the channel
has been inserted).

ENDCOMMENT

