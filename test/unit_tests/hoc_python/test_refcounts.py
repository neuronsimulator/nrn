from neuron import h
import sys


def test_seg_from_sec_x_ref():
    """A reproducer of calling `seg_from_sec_x` without a Python Section

    Without the fix generally we would get (upon interpreter exit)
       Fatal Python error: bool_dealloc: "deallocating True or False: bug likely caused by a refcount error in a C extension"
    """
    ## Pure python (ok)
    # soma1 = nrn.Section("soma1")
    # nc1 = h.NetCon(soma1(0.5)._ref_v, None)
    # seg1 = nc1.preseg()
    # print(sys.getrefcount(seg1))
    # print(sys.getrefcount(seg1.sec))

    h("create soma")
    h("objref nc")
    h("objref nil")
    h('soma nc = new NetCon(&v(0.5), nil)')
    nc = h.nc
    seg = nc.preseg()  # uses `seg_from_sec_x`

    assert sys.getrefcount(seg) == 2
    assert sys.getrefcount(seg.sec) == 3
