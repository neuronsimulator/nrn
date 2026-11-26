#pragma once
#include "nrnpy_capi.h"

namespace neuron::python {

/// Make the HOC class PythonObject, and install the functions with which HOC and the simulator call
/// Python. Python itself starts when one of them needs it.
void register_python_object();

// What the interface registers (docs/capi.rst).
// clang-format off
#define NRNPY_HOOKS(X) \
    X(coreneuron_arg) X(coreneuron_enable) X(coreneuron_file_mode) X(gui_callback)  \
    X(hoc_lock) X(model_epoch) X(object_class) X(reference) X(rvp_callable)         \
    X(savestate_restore) X(savestate_store) X(section_class) X(segment_class)
// clang-format on
enum class Hook {
#define X(name) name,
    NRNPY_HOOKS(X)
#undef X
        count
};
/// A hook, or null.
Ref optional(Hook hook);
/// A hook, which importing neuron registers; a HOC error if there is none.
Ref need(Hook hook);

/// The model may have changed: what the interface keeps about it is out of date.
void model_changed();

}  // namespace neuron::python
