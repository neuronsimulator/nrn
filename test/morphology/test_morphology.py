from neuron import h, morphio_api
from os import path

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

def test_morphio_read_from_wrapper_binding():
    """test that we can import at the top level without error"""
    cell = h.HocBasicCell()
    m = morphio_api.MorphIOWrapper('test/morphology/test.h5')

    morphio_api.morphio_read(cell, m)
    h.topology()

def test_morphio_read_api():
    """test that we can import at the top level without error"""
    assert(h("""
        objref cell1 
        cell1 = new HocBasicCell()
        morphio_read(cell1, "test/morphology/test.h5")
        """))
    h.topology()

