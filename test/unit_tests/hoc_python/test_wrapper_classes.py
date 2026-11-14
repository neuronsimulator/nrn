"""new_point_process and new_hoc_class wrappers forward attributes to the
HOC object, including names the interface resolves in __getattr__."""

import pytest

from neuron import h, new_hoc_class, new_point_process


def test_point_process_wrapper_reads_and_writes_parameters():
    sec = h.Section(name="wrapper_pp_sec")
    ExpSyn = new_point_process("ExpSyn")
    syn = ExpSyn(sec, 0.5)

    assert syn.tau == pytest.approx(0.1)
    assert syn.e == 0.0
    assert syn.get_loc() == 0.5
    h.pop_section()
    assert syn.hname() == syn.hoc_obj.hname()

    syn.tau = 7.5
    assert syn.tau == 7.5
    assert syn.hoc_obj.tau == 7.5

    with pytest.raises(AttributeError):
        syn.no_such_attribute


def test_hoc_class_wrapper_forwards_methods():
    Vector = new_hoc_class("Vector")
    vec = Vector()
    vec.resize(3)
    vec.fill(2.0)

    assert vec.size() == 3
    assert vec.sum() == 6.0
    assert vec.x[1] == 2.0
