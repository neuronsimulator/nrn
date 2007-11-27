// The problem this hack overcomes is that extending Python requires
// ivocmain.cpp in the shared library (compiled with PIC) and that is
// fine for normal neuron as well. But there are link problems when
// built statically and hence ivocmain.cpp is listed in the
// ivoc_SOURCES so that it is sure to be linked early in the process.

#include "ivocmain.cpp"
