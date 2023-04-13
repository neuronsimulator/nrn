#pragma once
/**
 * Declarations of global symbols in NEURON that have to be populated with python-version-specific values when dynamic Python is enabled. These are set by the nrnpython_reg_real function, and defined in nrnpy.cpp.
 */
struct Object;
// PyObject is a typedef, so we can't forward-declare it as a type. This pattern is common enough in the wild that we hope Python won't dare change it.
struct _object;
typedef _object PyObject;
namespace neuron::python {
struct impl_ptrs {
    void (*hoc_nrnpython)(){};
    PyObject* (*ho2po)(Object*){};
    Object* (*po2ho)(PyObject*){};
    int (*interpreter_start)(int){};
};
/**
 * @brief Collection of pointers to functions with python-version-specific implementations.
 * 
 * This is defined in nrnpy.cpp.
 */
extern impl_ptrs methods;
}
