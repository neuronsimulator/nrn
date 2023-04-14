#ifndef gui_redirect_h
#define gui_redirect_h

#include "hocdec.h"
#include "nrnpy.h"


extern Object* nrn_get_gui_redirect_obj();
extern double (*nrnpy_object_to_double_)(Object*);


#define TRY_GUI_REDIRECT_OBJ(name, obj)                                                \
    if (auto* const ngh_result =                                                       \
            neuron::python::methods.try_gui_helper(name, static_cast<Object*>(obj))) { \
        return static_cast<void*>(*ngh_result);                                        \
    }

#define TRY_GUI_REDIRECT_METHOD_ACTUAL_DOUBLE(name, sym, v)                     \
    {                                                                           \
        Object** guiredirect_result = NULL;                                     \
        if (neuron::python::methods.gui_helper) {                               \
            Object* obj = nrn_get_gui_redirect_obj();                           \
            guiredirect_result = neuron::python::methods.gui_helper(name, obj); \
            if (guiredirect_result) {                                           \
                return (nrnpy_object_to_double_(*guiredirect_result));          \
            }                                                                   \
        }                                                                       \
    }

#define TRY_GUI_REDIRECT_METHOD_ACTUAL_OBJ(name, sym, v)                        \
    {                                                                           \
        Object** guiredirect_result = NULL;                                     \
        if (neuron::python::methods.gui_helper) {                               \
            Object* obj = nrn_get_gui_redirect_obj();                           \
            guiredirect_result = neuron::python::methods.gui_helper(name, obj); \
            if (guiredirect_result) {                                           \
                return (guiredirect_result);                                    \
            }                                                                   \
        }                                                                       \
    }

#define TRY_GUI_REDIRECT_NO_RETURN(name, obj)                                        \
    if (auto* const ngh_result =                                                     \
            neuron::python::methods.try_gui_helper(name, static_cast<Object*>(obj)); \
        ngh_result) {                                                                \
        return;                                                                      \
    }

#define TRY_GUI_REDIRECT_DOUBLE(name, obj)                                             \
    if (auto* const ngh_result =                                                       \
            neuron::python::methods.try_gui_helper(name, static_cast<Object*>(obj))) { \
        hoc_ret();                                                                     \
        hoc_pushx(nrnpy_object_to_double_(*ngh_result));                               \
        return;                                                                        \
    }

#define TRY_GUI_REDIRECT_ACTUAL_DOUBLE(name, obj)                                      \
    if (auto* const ngh_result =                                                       \
            neuron::python::methods.try_gui_helper(name, static_cast<Object*>(obj))) { \
        return nrnpy_object_to_double_(*ngh_result);                                   \
    }

#define TRY_GUI_REDIRECT_ACTUAL_STR(name, obj)                                       \
    {                                                                                \
        char** ngh_result;                                                           \
        if (nrnpy_gui_helper3_str_) {                                                \
            ngh_result = nrnpy_gui_helper3_str_(name, static_cast<Object*>(obj), 0); \
            if (ngh_result) {                                                        \
                return ((const char**) ngh_result);                                  \
            }                                                                        \
        }                                                                            \
    }

#define TRY_GUI_REDIRECT_ACTUAL_OBJ(name, obj)                                         \
    if (auto* const ngh_result =                                                       \
            neuron::python::methods.try_gui_helper(name, static_cast<Object*>(obj))) { \
        return ngh_result;                                                             \
    }

#define TRY_GUI_REDIRECT_DOUBLE_SEND_STRREF(name, obj)                           \
    {                                                                            \
        Object** ngh_result;                                                     \
        if (nrnpy_gui_helper3_) {                                                \
            ngh_result = nrnpy_gui_helper3_(name, static_cast<Object*>(obj), 1); \
            if (ngh_result) {                                                    \
                hoc_ret();                                                       \
                hoc_pushx(nrnpy_object_to_double_(*ngh_result));                 \
                return;                                                          \
            }                                                                    \
        }                                                                        \
    }

#endif
