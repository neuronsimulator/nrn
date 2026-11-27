"""What NEURON asks of this interface when it runs Python for HOC.

Registered with nrn_python_hook_set (docs/capi.rst). An exception is the HOC
error of the statement that needed the hook.
"""

from . import _coreneuron
from .api import EPOCH, HOC_LOCK, nrn_python_hook_set
from .nrnref import reference
from .sections import Section, Segment
from .utils import hoc_class


def set_hook(name, hook):
    if nrn_python_hook_set(name.encode(), hook):
        raise RuntimeError(f"NEURON refuses the Python hook {name!r}")


def set_savestate_callbacks(store, restore, replace=False):
    """Register the Python state SaveState keeps, for packages such as neuron.rxd.

    ``store()`` returns a bytes-like object and ``restore(bytearray)`` receives
    it back. A failure is the HOC error of the SaveState call. ``replace`` is
    accepted for the released core's signature: here a hook always replaces.
    """
    set_hook("savestate_store", store)
    set_hook("savestate_restore", restore)
    return True


def set_rvp_converter(converter):
    """Register the function that turns a RangeVarPlot's Python object, such as
    an rxd species, into a function of x; it returns anything else unchanged."""
    set_hook("rvp_callable", converter)


def install():
    for name, hook in (
        ("hoc_lock", HOC_LOCK),
        ("model_epoch", EPOCH),
        ("object_class", hoc_class),
        ("reference", reference),
        ("section_class", Section),
        ("segment_class", Segment),
        ("coreneuron_enable", _coreneuron.enable_value),
        ("coreneuron_file_mode", _coreneuron.file_mode_value),
        ("coreneuron_arg", _coreneuron.nrncore_arg),
    ):
        set_hook(name, hook)
