"""Pointer and strref arguments of the GUI redirect must not leak.

`gui_helper_3_helper_` puts each HOC argument into the tuple it passes to
the Python callback. `PyTuple_SetItem` steals one reference. `hocobj_new`
and `cpp2refstr` already return a new reference, so a further `Py_INCREF`
would leave that wrapper alive after the tuple is destroyed.

A plain string in the same call is created with `PyUnicode_FromString` and
has no extra increment. While the callback is running, the fresh pointer
and strref wrappers must have that same refcount.
"""

import ctypes
import sys

from neuron import h, nrn_dll_sym

_set_gui_callback = nrn_dll_sym("nrnpy_set_gui_callback")
_record = {}


def _gui_callback(name, _obj, _context, *params):
    # getrefcount counts the helper tuple, this *params tuple, the loop
    # variable, and the temporary passed into getrefcount. Those are the
    # same for every fresh argument, so the counts match unless one wrapper
    # was incremented an extra time.
    _record[name] = [sys.getrefcount(param) for param in params]
    if name == "xvalue":
        params[1][0] = 7.5
    elif name == "xvarlabel":
        params[0][0] = "updated"
    return 1.0


def _install(callback):
    # Without argtypes, ctypes turns None into a NULL function pointer.
    # A py_object argument passes the callback itself.
    if callback is None:
        _set_gui_callback(None)
    else:
        _set_gui_callback(ctypes.py_object(callback))


def test_gui_helper_argument_refcount():
    prompt = "gui helper prompt 3256"
    number = h.ref(1.25)
    text = h.ref("hello")
    _record.clear()
    try:
        _install(_gui_callback)
        h.xvalue(prompt, number)
        h.xvarlabel(text)
    finally:
        _install(None)

    assert set(_record) == {"xvalue", "xvarlabel"}, _record
    prompt_rc, ptr_rc = _record["xvalue"]
    (str_rc,) = _record["xvarlabel"]
    assert number[0] == 7.5
    assert text[0] == "updated"
    assert ptr_rc == prompt_rc, (ptr_rc, prompt_rc)
    assert str_rc == prompt_rc, (str_rc, prompt_rc)


if __name__ == "__main__":
    test_gui_helper_argument_refcount()
