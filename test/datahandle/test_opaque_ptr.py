from neuron import gui
from neuron import h
import neuron

import gc
import pytest


def check_get_dangling_pointer(s, mech_name):
    def check(callback):
        with pytest.raises(Exception) as e:
            callback()

        assert "point to anything" in str(e.value)

    check(lambda: getattr(s, f"ptr_{mech_name}"))
    check(lambda: getattr(s, f"_ref_ptr_{mech_name}"))

    mech = getattr(s, mech_name)
    check(lambda: getattr(mech, "ptr"))
    check(lambda: getattr(mech, "_ref_ptr"))


def check_set_dangling_pointer(s, mech_name):
    def check(callback):
        with pytest.raises(Exception) as e:
            callback()

        assert "point to anything" in str(e.value)

    check(lambda: setattr(s, f"ptr_{mech_name}", 42.0))

    mech = getattr(s, mech_name)
    check(lambda: setattr(mech, "ptr", 42.0))


def check_set_string_as_double(s, mech_name):
    def check(callback):
        with pytest.raises(ValueError) as e:
            callback()

        message = str(e.value)
        option1 = "bad value" in message
        option2 = "must be a double" in message
        assert option1 or option2, message

    check(lambda: setattr(s, f"ptr_{mech_name}", "42.0"))

    mech = getattr(s, mech_name)
    check(lambda: setattr(mech, "ptr", "42.0"))


def check_set_double_as_pointer(s, mech_name):
    def check(callback):
        with pytest.raises(ValueError) as e:
            callback()

        assert "must be a hoc pointer" in str(e.value)

    # check(lambda : setattr(s, f"_ref_ptr_{mech_name}", 42.0))

    mech = getattr(s, mech_name)
    check(lambda: setattr(mech, "_ref_ptr", 42.0))


def check_dangling_pointer(s, mech_name):
    check_get_dangling_pointer(s, mech_name)
    check_set_dangling_pointer(s, mech_name)
    check_set_double_as_pointer(s, mech_name)


def check_invalid_assign_value(s, mech_name):
    def check_value(callback):
        with pytest.raises(Exception) as e:
            callback(42.0)
        assert "opaque pointer" in str(e.value), str(e.value)

    check_value(lambda value: setattr(s, f"ptr_{mech_name}", value))

    mech = getattr(s, mech_name)
    check_value(lambda value: setattr(mech, "ptr", value))


def check_get_opaque_pointer(s, mech_name):
    mech = getattr(s, mech_name)

    assert isinstance(getattr(s, f"ptr_{mech_name}"), neuron.nrn.OpaquePointer)
    assert isinstance(getattr(s, f"_ref_ptr_{mech_name}"), neuron.nrn.OpaquePointer)
    assert isinstance(mech.ptr, neuron.nrn.OpaquePointer)
    assert isinstance(mech._ref_ptr, neuron.nrn.OpaquePointer)


def check_opaque_pointers(s, mech_name):
    check_get_opaque_pointer(s, mech_name)
    check_invalid_assign_value(s, mech_name)
    check_set_double_as_pointer(s, mech_name)


def check_reassigned(s, mech_name, expected_value):
    mech = getattr(s, mech_name)

    assert getattr(s, f"ptr_{mech_name}") == expected_value
    assert mech.ptr == expected_value

    assert "data_handle<double>" in str(mech._ref_ptr)
    assert "data_handle<double>" in str(getattr(s, f"_ref_ptr_{mech_name}"))


def test_both():
    # Optional, but let's start with a clean slate.
    gc.collect()

    section = h.Section()
    section.insert("ptr_mod")
    section.insert("opaque_ptr_mod")
    s = section(0.5)

    check_dangling_pointer(s, "ptr_mod")
    check_dangling_pointer(s, "opaque_ptr_mod")

    h.stdinit()

    check_opaque_pointers(s, "opaque_ptr_mod")

    for mech_name in ["ptr_mod", "opaque_ptr_mod"]:
        getattr(s, mech_name)._ref_ptr = h._ref_t
        h.t = 4.0

        check_reassigned(s, mech_name, h.t)
        check_set_string_as_double(s, mech_name)
        check_set_double_as_pointer(s, mech_name)

        setattr(s, f"_ref_ptr_{mech_name}", s._ref_v)
        check_reassigned(s, mech_name, s.v)

    # Essential, because we can't run `stdinit` if a section
    # with a reassigned opaque pointer is still alive.
    gc.collect()


if __name__ == "__main__":
    import sys

    m = sys.modules["__main__"]
    for f in filter(lambda x: x.startswith("test"), dir()):
        getattr(m, f)()
