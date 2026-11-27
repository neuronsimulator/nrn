"""A redirected GUI destructor must be called without HOC arguments.

`~Graph` and the other destructor redirects run when the object is freed, which
can happen inside an unrelated HOC call (here, a procedure that has arguments)
or outside any HOC call. `gui_helper_3_helper_` must not scan the current HOC
frame for arguments then: it would pass the arguments of that unrelated
procedure, or read a stale frame.
"""

import ctypes

from neuron import h, nrn_dll_sym

_set_gui_callback = nrn_dll_sym("nrnpy_set_gui_callback")
_params = {}
_holder = []


def _gui_callback(name, _obj, _context, *params):
    _params.setdefault(name, []).append(params)
    return 1.0


def _install(callback):
    # Without argtypes, ctypes turns None into a NULL function pointer.
    # A py_object argument passes the callback itself.
    if callback is None:
        _set_gui_callback(None)
    else:
        _set_gui_callback(ctypes.py_object(callback))


def _drop():
    _holder.clear()


def test_gui_destructor_has_no_arguments():
    import __main__

    __main__._gui_destructor_drop = _drop
    h('proc gui_dtor_outer() { nrnpython("_gui_destructor_drop()") }')
    _params.clear()
    try:
        _install(_gui_callback)
        _holder.append(h.Graph())
        # Freed while gui_dtor_outer's frame, with three arguments, is current.
        h.gui_dtor_outer(1.0, 2.0, 3.0)
        assert not _holder
        _holder.append(h.Graph())
        _drop()  # and freed outside any HOC call
    finally:
        _install(None)
        del __main__._gui_destructor_drop

    assert "~Graph" in _params, _params
    assert _params["~Graph"] == [(), ()], _params["~Graph"]


if __name__ == "__main__":
    test_gui_destructor_has_no_arguments()
