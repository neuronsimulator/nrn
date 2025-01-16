from neuron import h, nrn
import gc
import sys


def test_seg_from_sec_x_ref_python():
    """
    A reproducer of calling `seg_from_sec_x` with a Python Section
    """
    soma1 = nrn.Section("soma1")
    nc1 = h.NetCon(soma1(0.5)._ref_v, None)
    seg1 = nc1.preseg()
    del soma1

    assert sys.getrefcount(seg1) == 2
    # It is unclear why we get 3 ref-counts when we use plain python. With nrniv we get 2.
    # At least it is consistent with the test below and we assert that the section
    # really gets deleted (in hoc)
    assert sys.getrefcount(seg1.sec) == 3  # seg->pysec_ + 1

    del seg1  # NOTE: section fully destroyed together with segment
    gc.collect()

    h("n_sections=0")
    h("forall { n_sections+=1 }")
    h('soma_is_valid=section_exists("soma1")')
    assert h.n_sections == 0
    assert not h.soma_is_valid


def test_seg_from_sec_x_ref_hocpy():
    """A reproducer of calling `seg_from_sec_x` without a Python Section

    Without the fix we would get one extra ref to sec and possibly (upon interpreter exit):
      "Fatal Python error: bool_dealloc: "deallocating True or False: bug likely caused by a
       refcount error in a C extension"
    """
    h("create soma")
    h("objref nc")
    h("objref nil")
    h("soma nc = new NetCon(&v(0.5), nil)")
    nc = h.nc
    seg = nc.preseg()  # uses `seg_from_sec_x`

    assert sys.getrefcount(seg) == 2
    assert sys.getrefcount(seg.sec) == 3  # seg->pysec_ + 1

    del seg  # NOTE: section py wrapper destroyed with segment, hoc-level section kept
    gc.collect()

    h("n_sections=0")
    h("forall { n_sections+=1 }")
    h('soma_is_valid=section_exists("soma")')
    assert h.n_sections == 1
    assert h.soma_is_valid
