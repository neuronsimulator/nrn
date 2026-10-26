import ctypes

import pytest
from neuron import h, hoc


def value_at(address):
    return ctypes.c_double.from_address(address).value


def test_global():
    address = hoc.ref_address(h._ref_t)
    h.t = 12.5
    assert value_at(address) == 12.5
    ctypes.c_double.from_address(address).value = 3.0
    assert h.t == 3.0
    h.t = 0


def test_range_variable_and_vector_element():
    sec = h.Section(name="ref_address_sec")
    sec.nseg = 3
    sec(0.5).v = -42.0
    assert value_at(hoc.ref_address(sec(0.5)._ref_v)) == -42.0
    assert hoc.ref_address(sec(0.5)._ref_v) != hoc.ref_address(sec(0.1)._ref_v)
    vec = h.Vector([1.0, 2.0, 3.0])
    assert value_at(hoc.ref_address(vec._ref_x[1])) == 2.0
    assert hoc.ref_address(vec._ref_x[1]) == hoc.ref_address(vec._ref_x[0]) + 8


def test_reference_to_a_number():
    ref = h.ref(4.0)
    ctypes.c_double.from_address(hoc.ref_address(ref)).value = 7.0
    assert ref[0] == 7.0


@pytest.mark.parametrize("value", [h.Vector(2), h.ref("text")])
def test_not_a_reference_to_a_number(value):
    with pytest.raises(TypeError):
        hoc.ref_address(value)


def test_not_a_hoc_object():
    with pytest.raises(TypeError):
        hoc.ref_address(1.0)
