#pragma once
#include "nrndlldef.h"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
/**
 * Declarations of global symbols in NEURON that have to be populated with python-version-specific
 * values when dynamic Python is enabled. These are set by the nrnpython_reg_real function, and
 * defined in nrnpy.cpp.
 */
struct Object;
// Back-compat pointer filled from methods.hoccommand_exec. MOD VERBATIM
// (e.g. test/rxd/beforestep_py.mod) imports this from nrniv.dll.
extern NRN_DLLSYM int (*nrnpy_hoccommand_exec)(Object*);
// PyObject is a typedef, so we can't forward-declare it as a type. This pattern is common enough in
// the wild that we hope Python won't dare change it.
struct _object;
typedef _object PyObject;
struct _ts;
typedef _ts PyThreadState;
struct Section;
struct Symbol;
class IvocVect;
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
    std::vector<char> (*call_picklef)(const std::vector<char>&, int narg){};
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
    Object* (*pickle2po)(const std::vector<char>&){};
    Object* (*po2ho)(PyObject*){};
    std::vector<char> (*po2pickle)(Object*){};
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

// DYNAMIC Python: these live in nrniv.dll and libnrnpythonX.Y.dll assigns them.
extern NRN_DLLSYM Symbol* nrnpy_pyobj_sym_;
extern NRN_DLLSYM std::vector<const char*> py_exposed_classes;
extern NRN_DLLSYM std::string nrnpy_pyexe;
extern NRN_DLLSYM int nrnpy_nositeflag;
extern NRN_DLLSYM int hoc_max_builtin_class_id;
extern NRN_DLLSYM int (*p_nrnpy_pyrun)(const char*);
extern NRN_DLLSYM int (*nrnpy_pr_stdoe_callback)(int, char*);
extern NRN_DLLSYM void (*nrnpy_sectionlist_helper_)(void*, Object*);
extern NRN_DLLSYM void* (*nrnpy_get_pyobj)(Object*);
extern NRN_DLLSYM void (*nrnpy_restore_savestate)(int64_t, char*);
extern NRN_DLLSYM void (*nrnpy_store_savestate)(char**, uint64_t*);
extern NRN_DLLSYM void (*nrnpy_decref)(void*);
extern NRN_DLLSYM double (*nrnpy_call_func)(Object*, double);
extern NRN_DLLSYM int (*nrnpy_call_obj_method)(Object*, const char*, Object*);
extern NRN_DLLSYM int (*nrnpy_call_obj_method_double)(Object*, const char*, double);
extern NRN_DLLSYM IvocVect* (*nrnpy_vec_from_python_p_)(void*);
extern NRN_DLLSYM Object** (*nrnpy_vec_to_python_p_)(void*);
extern NRN_DLLSYM Object** (*nrnpy_vec_as_numpy_helper_)(int, double*);
extern NRN_DLLSYM Object* (*nrnpy_rvp_rxd_to_callable)(Object*);
extern NRN_DLLSYM int (*nrnpy_nrncore_enable_value_p_)();
extern NRN_DLLSYM int (*nrnpy_nrncore_file_mode_value_p_)();
extern NRN_DLLSYM char* (*nrnpy_nrncore_arg_p_)(double);
extern NRN_DLLSYM void (*nrnpy_reg_mech_p_)(int);
extern NRN_DLLSYM int (*nrnpy_ob_is_seg)(Object*);
extern NRN_DLLSYM Object* (*nrnpy_seg_from_sec_x)(Section*, double);
extern NRN_DLLSYM Section* (*nrnpy_o2sec_p_)(Object*);
extern NRN_DLLSYM void (*nrnpy_o2loc_p_)(Object*, Section**, double*);
extern NRN_DLLSYM void (*nrnpy_o2loc2_p_)(Object*, Section**, double*);
extern NRN_DLLSYM char* (*nrnpy_pysec_name_p_)(Section*);
extern NRN_DLLSYM Object* (*nrnpy_pysec_cell_p_)(Section*);
extern NRN_DLLSYM int (*nrnpy_pysec_cell_equals_p_)(Section*, Object*);
