#pragma once
#include <string_view>
/**
 * Declarations of global symbols in NEURON that have to be populated with python-version-specific
 * values when dynamic Python is enabled. These are set by the nrnpython_reg_real function, and
 * defined in nrnpy.cpp.
 */
struct Object;
// PyObject is a typedef, so we can't forward-declare it as a type. This pattern is common enough in
// the wild that we hope Python won't dare change it.
struct _object;
typedef _object PyObject;
struct _ts;
typedef _ts PyThreadState;
namespace neuron::python {
struct impl_ptrs {
    Object* (*callable_with_args)(Object*, int narg){};
    char* (*call_picklef)(char*, std::size_t size, int narg, std::size_t* retsize){};
    int (*guigetstr)(Object*, char**){};
    double (*guigetval)(Object*){};
    void (*guisetval)(Object*, double){};
    void (*hoc_nrnpython)(){};
    PyObject* (*ho2po)(Object*){};
    void (*interpreter_set_path)(std::string_view){};
    int (*interpreter_start)(int){};
    Object* (*mpi_alltoall_type)(int, int){};
    void* (*opaque_obj2pyobj)(Object*){};
    Object* (*pickle2po)(char*, std::size_t size){};
    Object* (*po2ho)(PyObject*){};
    char* (*po2pickle)(Object*, std::size_t* size);
    int (*pysame)(Object* o1, Object* o2){};
    void (*restore_thread)(PyThreadState*){};
    PyThreadState* (*save_thread)(){};
};
/**
 * @brief Collection of pointers to functions with python-version-specific implementations.
 *
 * This is defined in nrnpy.cpp.
 */
extern impl_ptrs methods;
}  // namespace neuron::python
