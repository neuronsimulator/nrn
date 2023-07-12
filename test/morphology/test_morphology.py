from neuron import h, morphio_api
from os import path
import pytest


def test_setup_cell():
    h(
        """
        begintemplate HocBasicCell
        objref this, somatic, all, axonal, basal, apical
        proc init() {
                all     = new SectionList()
                somatic = new SectionList()
                basal   = new SectionList()
                apical  = new SectionList()
                axonal  = new SectionList()
        }

        create soma[1], dend[1], apic[1], axon[1]
        endtemplate HocBasicCell
        """
    )


def test_morphio_load_from_python():
    cell0 = h.HocBasicCell()
    h.morphio_load(cell0, "test/morphology/test.h5")
    with pytest.raises(Exception) as _:
        h.morphio_load(None, "test/morphology/test.h5")
    h.topology()


def test_morphio_load_from_wrapper_binding():
    """test that we can import at the top level without error"""
    cell = h.HocBasicCell()
    m = morphio_api.MorphIOWrapper("test/morphology/test.h5")
    morphio_api.morphio_load(cell, m)
    with pytest.raises(Exception) as _:
        h.morphio_load(None, "test/morphology/test.h5")
    h.topology()


def test_morphio_load_hoc():
    """test that we can import at the top level without error"""
    assert h(
        """
        objref cell1 
        cell1 = new HocBasicCell()
        morphio_load(cell1, "test/morphology/test.h5")
        """
    )
    assert not h(
        """
        objref empty
        morphio_load(empty, "test/morphology/test.h5")
        """
    )
    h.topology()
