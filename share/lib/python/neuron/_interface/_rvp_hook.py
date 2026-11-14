"""The core's RangeVarPlot hook: turn an rxd species into a callable.

h.RangeVarPlot(species, seg0, seg1) plots a Python object. The core passes it
through libnrniv's nrnpy_rvp_rxd_to_callable (spaceplt.cpp), which NEURON's
Python module sets; the neuron package registers the converter, which maps an
rxd species to a function of x and returns anything else unchanged. While the
pointer is NULL the core calls the species itself, and the plot is wrong.

As NEURON's, the converter's result is wrapped as a new PythonObject. The
core's HocCommand takes the only reference, so this returns it unowned. An
error in the converter is printed, and the object is plotted unconverted.
"""

import ctypes
import traceback

# Set by the neuron package (rxd's _nrnpy_rvp_pyobj_callback).
_converter = None

# Keep the callback alive for as long as the core can call it.
_mn_rvp_rxd_to_callable_cb = None


def set_pyobj_callback(converter):
    """Register the function that turns a RangeVarPlot's Python object into a callable."""
    global _converter
    _converter = converter


def _rvp_rxd_to_callable(obj):
    from .api import _py_pyobj_to_hoc_cfunctype, _unwrap_pythonobject

    if not obj or _converter is None:
        return obj
    try:
        value = _unwrap_pythonobject(obj)
        result = _converter(value)
        if result is value or result is None:
            return obj
        wrapped = _py_pyobj_to_hoc_cfunctype(result, take_ref=False)
        return wrapped if wrapped else obj
    except BaseException:
        traceback.print_exc()
        return obj


def register(libnrniv):
    global _mn_rvp_rxd_to_callable_cb
    try:
        slot = ctypes.c_void_p.in_dll(libnrniv, "nrnpy_rvp_rxd_to_callable")
    except ValueError:
        return
    if slot.value:
        return  # set by NEURON's Python module
    _mn_rvp_rxd_to_callable_cb = ctypes.CFUNCTYPE(ctypes.c_void_p, ctypes.c_void_p)(
        _rvp_rxd_to_callable
    )
    slot.value = ctypes.cast(_mn_rvp_rxd_to_callable_cb, ctypes.c_void_p).value
