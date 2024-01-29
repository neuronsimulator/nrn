#pragma once
#include "neuron/container/data_handle.hpp"
#include "nrnpython.h"

struct Object;
struct Symbol;
struct PyHocObject {
    PyObject_HEAD
    Object* ho_;
    union {
        double x_;
        char* s_;
        char** pstr_;
        Object* ho_;
        neuron::container::data_handle<double> px_;
        PyHoc::IteratorState its_;
    } u;
    Symbol* sym_;             // for functions and arrays
    void* iteritem_;          // enough info to carry out Iterator protocol
    int nindex_;              // number indices seen so far (or narg)
    int* indices_;            // one fewer than nindex_
    PyHoc::ObjectType type_;  // 0 HocTopLevelInterpreter, 1 HocObject
                              // 2 function (or TEMPLATE)
                              // 3 array
                              // 4 reference to number
                              // 5 reference to string
                              // 6 reference to hoc object
                              // 7 forall section iterator
                              // 8 pointer to a hoc scalar
                              // 9 incomplete pointer to a hoc array (similar to 3)
};
