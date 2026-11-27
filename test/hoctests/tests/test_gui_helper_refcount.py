"""Pointer and strref arguments of the GUI redirect must not leak.

While the callback runs, they have the refcount of a plain string in the same call.
"""

import ctypes
import sys

from neuron import h, nrn_dll_sym

_set_gui_callback = nrn_dll_sym("nrnpy_set_gui_callback")
_record = {}


def _gui_callback(name, _obj, _context, *params):
    # The same for every fresh argument, unless one was incremented an extra time.
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
        graph = h.Graph()
        assert graph.begin() == 1 and h.boolean_dialog("q") == 1
        # Its C++ object is the callback's; with InterViews, freeing it unredirected crashes.
        del graph
    finally:
        _install(None)

    names = "xvalue xvarlabel Graph Graph.begin boolean_dialog ~Graph"
    assert list(_record) == names.split(), _record
    prompt_rc, ptr_rc = _record["xvalue"]
    (str_rc,) = _record["xvarlabel"]
    assert number[0] == 7.5
    assert text[0] == "updated"
    assert ptr_rc == prompt_rc, (ptr_rc, prompt_rc)
    assert str_rc == prompt_rc, (str_rc, prompt_rc)


if __name__ == "__main__":
    test_gui_helper_argument_refcount()
