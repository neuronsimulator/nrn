"""The core's CoreNEURON hooks: neuron.coreneuron's enable, file_mode, nrncore_arg.

pc.psolve asks libnrniv's nrncore_is_enabled() whether to run CoreNEURON, and
nrncore_psolve() for its argument string. Both go through function pointers
that NEURON's Python module sets (nrnpy_hoc.cpp); while they are NULL,
coreneuron.enable = True is silently ignored and NEURON simulates the model
itself. As NEURON's, the hooks read the "neuron.coreneuron" module only if it
has been imported, and print a Python error instead of raising it (the core
just sees CoreNEURON as disabled). A pointer someone else set is left alone.
"""

import ctypes
import sys
import traceback

_MODULE = "neuron.coreneuron"

# Keep the callbacks alive for as long as the core can call them.
_mn_nrncore_enable_cb = None
_mn_nrncore_file_mode_cb = None
_mn_nrncore_arg_cb = None


def _opt_value(option):
    """int(neuron.coreneuron.<option>); 0 without the module, -1 on error."""
    module = sys.modules.get(_MODULE)
    if module is None:
        return 0
    try:
        return int(getattr(module, option))
    except BaseException:
        traceback.print_exc()
        return -1


def _enable_value():
    return _opt_value("enable")


def _file_mode_value():
    return _opt_value("file_mode")


def _nrncore_arg(tstop):
    """strdup() of neuron.coreneuron.nrncore_arg(tstop); NULL if empty or on error.

    The core frees the string, so it comes from the allocator libnrniv uses.
    """
    from .nrnref import _get_libc

    module = sys.modules.get(_MODULE)
    if module is None:
        return None
    try:
        arg = module.nrncore_arg(tstop)
        if not isinstance(arg, str) or not arg.isascii():
            raise TypeError(
                "neuron.coreneuron.nrncore_arg() must return an ascii string"
            )
    except BaseException:
        traceback.print_exc()
        return None
    if not arg:
        return None
    return _get_libc().strdup(arg.encode("ascii"))


def register(libnrniv):
    global _mn_nrncore_enable_cb, _mn_nrncore_file_mode_cb, _mn_nrncore_arg_cb
    try:
        enable_slot = ctypes.c_void_p.in_dll(libnrniv, "nrnpy_nrncore_enable_value_p_")
        file_mode_slot = ctypes.c_void_p.in_dll(
            libnrniv, "nrnpy_nrncore_file_mode_value_p_"
        )
        arg_slot = ctypes.c_void_p.in_dll(libnrniv, "nrnpy_nrncore_arg_p_")
    except ValueError:
        return  # a core built without dlfcn support has no CoreNEURON launcher
    int_hook = ctypes.CFUNCTYPE(ctypes.c_int)
    if not enable_slot.value:
        _mn_nrncore_enable_cb = int_hook(_enable_value)
        enable_slot.value = ctypes.cast(_mn_nrncore_enable_cb, ctypes.c_void_p).value
    if not file_mode_slot.value:
        _mn_nrncore_file_mode_cb = int_hook(_file_mode_value)
        file_mode_slot.value = ctypes.cast(
            _mn_nrncore_file_mode_cb, ctypes.c_void_p
        ).value
    if not arg_slot.value:
        _mn_nrncore_arg_cb = ctypes.CFUNCTYPE(ctypes.c_void_p, ctypes.c_double)(
            _nrncore_arg
        )
        arg_slot.value = ctypes.cast(_mn_nrncore_arg_cb, ctypes.c_void_p).value
