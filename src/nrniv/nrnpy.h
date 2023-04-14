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
struct Section;
struct Symbol;
namespace neuron::python {
/**
 * @brief Collection of pointers to functions with python-version-specific implementations.
 *
 * When dynamic Python is enabled, these are filled in from a python-version-specific
 * libnrnpythonX.Y library and then called from python-version-agnostic code inside NEURON.
 */
struct impl_ptrs {
    Object* (*callable_with_args)(Object*, int narg){};
    double (*call_func)(Object*, int, int*){};
    char* (*call_picklef)(char*, std::size_t size, int narg, std::size_t* retsize){};
    void (*call_python_with_section)(Object*, Section*){};
    void (*cmdtool)(Object*, int type, double x, double y, int kd){};
    int (*guigetstr)(Object*, char**){};
    double (*guigetval)(Object*){};
    Object** (*gui_helper)(const char* name, Object* obj){};
    Object** (*gui_helper3)(const char* name, Object* obj, int handle_strptr){};
    char** (*gui_helper3_str)(const char*, Object*, int){};
    void (*guisetval)(Object*, double){};
    int (*hoccommand_exec)(Object*){};
    int (*hoccommand_exec_strret)(Object*, char*, int){};
    void (*hoc_nrnpython)(){};
    PyObject* (*ho2po)(Object*){};
    void (*hpoasgn)(Object*, int){};
    void (*interpreter_set_path)(std::string_view){};
    int (*interpreter_start)(int){};
    Object* (*mpi_alltoall_type)(int, int){};
    double (*object_to_double)(Object*){};
    void* (*opaque_obj2pyobj)(Object*){};
    Object* (*pickle2po)(char*, std::size_t size){};
    Object* (*po2ho)(PyObject*){};
    char* (*po2pickle)(Object*, std::size_t* size){};
    double (*praxis_efun)(Object* pycallable, Object* hvec){};
    int (*pysame)(Object* o1, Object* o2){};
    void (*py2n_component)(Object*, Symbol*, int, int){};
    void (*restore_thread)(PyThreadState*){};
    PyThreadState* (*save_thread)(){};
    // Such a common pattern it gets a wrapper
    Object** try_gui_helper(const char* name, Object* obj) const {
        if (gui_helper) {
            return gui_helper(name, obj);
        } else {
            return nullptr;
        }
    }
};
/**
 * @brief Collection of pointers to functions with python-version-specific implementations.
 *
 * This is defined in nrnpy.cpp.
 */
extern impl_ptrs methods;
}  // namespace neuron::python
extern Symbol* nrnpy_pyobj_sym_;
