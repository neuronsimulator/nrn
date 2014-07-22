COMMENT
/**
 * @file HDF5reader.mod
 * @brief 
 * @author king
 * @date 2009-10-23
 * @remark Copyright © BBP/EPFL 2005-2011; All rights reserved. Do not distribute without further notice.
 */
ENDCOMMENT

COMMENT
This is intended to serve two purposes.  One as a general purpose reader for HDF5 files with a basic set of
accessor functions.  In addition, it will have special handling for our synapse data files such that they can be handled
more efficiently in a  massively parallel manner.  I feel inclined to spin this functionality out into a c++ class where
I can access more advanced coding structures, especially STL.
ENDCOMMENT

NEURON {
    THREADSAFE : have no evidence it is, but want to get bluron to accept
    ARTIFICIAL_CELL HDF5Reader
    BBCOREPOINTER ptr
}

VERBATIM
/* not quite sure what to do for bbcore. For now, nothing. */
static void bbcore_write(double* x, int* d, int* xx, int* dd, _threadargsproto_) {
  return;
}
static void bbcore_read(double* x, int* d, int* xx, int* dd, _threadargsproto_) {
  return;
}
ENDVERBATIM

PARAMETER {
}

ASSIGNED {
        ptr
}

INITIAL {
}

NET_RECEIVE(w) {
}

