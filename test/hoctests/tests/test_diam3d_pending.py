# A diam write on a section with 3-d points must not depend on whether the
# segment diameters had already been derived from those points.
#
# After pt3dadd, the segment diameters are only derived from the 3-d points at
# the next area recompute. A diam write copies every segment's diam into the
# 3-d points (see pt3dconst), so if the untouched segments still held their
# old value, e.g. the 500 um default of a new section, the points' own
# diameters were overwritten. Each case below compares a write on a fresh
# section with the same write on an identical section whose diameters were read
# first, and checks that the 3-d points outside the written segment keep their
# diameter of 3 um.
from math import isclose
from neuron import h


def make(name):
    # Three 3-d points 10 um apart, all diam 3, and one segment per point.
    h(f"create {name}")
    sec = getattr(h, name)
    sec.nseg = 3
    for i in range(3):
        sec.pt3dadd(10 * i, 0, 0, 3)
    return sec


def check_same(fresh, reference):
    for i in range(fresh.n3d()):
        assert isclose(fresh.diam3d(i), reference.diam3d(i)), (fresh, i)
    for f, r in zip(fresh, reference):
        assert isclose(f.diam, r.diam), (f, r)


def test_hoc_diam_x():
    fresh = make("hoc_fresh")
    reference = make("hoc_reference")
    reference(0.5).diam  # derive the segment diameters from the 3-d points
    h("hoc_fresh.diam(0.5) = 42")
    h("hoc_reference.diam(0.5) = 42")
    assert [fresh.diam3d(i) for i in range(3)] == [3, 42, 3]
    check_same(fresh, reference)


def test_hoc_diam_range():
    # The range form writes only the segments whose centres lie in 0:0.3.
    fresh = make("hocr_fresh")
    reference = make("hocr_reference")
    reference(0.5).diam
    h("hocr_fresh.diam(0:0.3) = 42:42")
    h("hocr_reference.diam(0:0.3) = 42:42")
    assert [fresh.diam3d(i) for i in range(3)] == [42, 3, 3]
    check_same(fresh, reference)


def test_python_seg_diam():
    fresh = make("py_fresh")
    reference = make("py_reference")
    reference(0.5).diam
    fresh(0.5).diam = 42
    reference(0.5).diam = 42
    assert [fresh.diam3d(i) for i in range(3)] == [3, 42, 3]
    check_same(fresh, reference)


if __name__ == "__main__":
    test_hoc_diam_x()
    test_hoc_diam_range()
    test_python_seg_diam()
