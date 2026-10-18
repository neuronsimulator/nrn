from neuron import h, morphio_api
from os import path
import os
import pytest


def find_file(path_str):
    if os.path.isabs(path_str) and os.path.exists(path_str):
        return path_str
    if os.path.exists(path_str):
        return os.path.abspath(path_str)
    current = os.path.abspath(os.path.dirname(__file__))
    while True:
        candidate = os.path.join(current, path_str)
        if os.path.exists(candidate):
            return os.path.abspath(candidate)
        parent = os.path.dirname(current)
        if parent == current:
            break
        current = parent
    current = os.path.abspath(os.getcwd())
    while True:
        candidate = os.path.join(current, path_str)
        if os.path.exists(candidate):
            return os.path.abspath(candidate)
        parent = os.path.dirname(current)
        if parent == current:
            break
        current = parent
    return path_str


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
    with pytest.raises(RuntimeError):
        h.morphio_load(None, "test/morphology/test.h5")
    h.topology()


def test_morphio_load_from_wrapper_binding():
    """test that we can import at the top level without error"""
    cell = h.HocBasicCell()
    m = morphio_api.MorphIOWrapper("test/morphology/test.h5")
    assert len(m.morph_as_hoc()) > 0
    assert m.section_index_to_name[0] == "soma"
    assert m.section_type_distribution[0] == (1, -1, 1)
    morphio_api.morphio_load(cell, m)

    cell2 = h.HocBasicCell()
    morphio_api.morphio_load(cell2, "test/morphology/test.h5")
    with pytest.raises((RuntimeError, TypeError)):
        morphio_api.morphio_load(None, m)
    with pytest.raises(RuntimeError):
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


def test_morphio_hoc_api():
    assert h(
        """
        load_file("MorphIO.hoc")
        """
    )
    assert h(
        """
        objref cell1 
        cell1 = new HocBasicCell()
        morphio_read(cell1, "test/morphology/test.h5")
        """
    )
    assert not h(
        """
        objref empty
        morphio_read(empty, "test/morphology/test.h5")
        """
    )
    h.topology()
    if path.exists("test/morphology/test.hoc"):
        import os

        os.remove("test/morphology/test.hoc")


def test_morphio_vs_import3d_swc_threepoint():
    h.load_file("stdlib.hoc")
    h.load_file("import3d.hoc")
    fname = find_file("share/lib/python/neuron/rxdtests/tests/c91662.swc")

    # MorphIO
    c_m = h.HocBasicCell()
    h.morphio_load(c_m, fname)

    # Import3d
    r = h.Import3d_SWC_read()
    r.quiet = 1
    r.input(fname)
    g = h.Import3d_GUI(r, 0)
    c_i = h.HocBasicCell()
    g.instantiate(c_i)

    secs_m = {s.name().split(".")[-1]: s for s in c_m.all if s.n3d() > 0}
    secs_i = {s.name().split(".")[-1]: s for s in c_i.all if s.n3d() > 0}

    assert len(secs_m) == 194
    assert set(secs_m.keys()) == set(secs_i.keys())

    for k in secs_m:
        sm, si = secs_m[k], secs_i[k]
        assert sm.n3d() == si.n3d(), f"{k}: n3d mismatch {sm.n3d()} vs {si.n3d()}"
        for j in range(sm.n3d()):
            pm = (sm.x3d(j), sm.y3d(j), sm.z3d(j), sm.diam3d(j))
            pi = (si.x3d(j), si.y3d(j), si.z3d(j), si.diam3d(j))
            assert pm == pi, f"{k}[{j}] mismatch: {pm} vs {pi}"


def test_morphio_vs_import3d_asc_contour():
    h.load_file("stdlib.hoc")
    h.load_file("import3d.hoc")

    for rel_path in [
        "test/hoctests/tests/test_pt3dstyle.asc",
        "test/rxd/3d/simple.asc",
    ]:
        fname = find_file(rel_path)
        c_m = h.HocBasicCell()
        h.morphio_load(c_m, fname)

        r = h.Import3d_Neurolucida3()
        r.quiet = 1
        r.input(fname)
        g = h.Import3d_GUI(r, 0)
        c_i = h.HocBasicCell()
        g.instantiate(c_i)

        secs_m = {s.name().split(".")[-1]: s for s in c_m.all if s.n3d() > 0}
        secs_i = {s.name().split(".")[-1]: s for s in c_i.all if s.n3d() > 0}

        assert set(secs_m.keys()) == set(secs_i.keys())

        for k in secs_m:
            sm, si = secs_m[k], secs_i[k]
            assert sm.n3d() == si.n3d(), f"{k}: n3d mismatch {sm.n3d()} vs {si.n3d()}"
            for j in range(sm.n3d()):
                pm = (sm.x3d(j), sm.y3d(j), sm.z3d(j), sm.diam3d(j))
                pi = (si.x3d(j), si.y3d(j), si.z3d(j), si.diam3d(j))
                assert pm == pi, f"{fname} {k}[{j}] mismatch: {pm} vs {pi}"


def test_morphio_vs_import3d_cylinders():
    h.load_file("stdlib.hoc")
    h.load_file("import3d.hoc")

    for rel_path in [
        "external/MorphIO/src/morphio-external/tests/data/soma_single_frustum.swc",
        "external/MorphIO/src/morphio-external/tests/data/soma_multiple_frustums.swc",
        "external/MorphIO/src/morphio-external/tests/data/soma_three_points_cylinder.swc",
    ]:
        fname = find_file(rel_path)
        if not os.path.exists(fname):
            continue
        c_m = h.HocBasicCell()
        h.morphio_load(c_m, fname)

        r = h.Import3d_SWC_read()
        r.quiet = 1
        r.input(fname)
        g = h.Import3d_GUI(r, 0)
        c_i = h.HocBasicCell()
        g.instantiate(c_i)

        secs_m = {s.name().split(".")[-1]: s for s in c_m.all if s.n3d() > 0}
        secs_i = {s.name().split(".")[-1]: s for s in c_i.all if s.n3d() > 0}

        assert set(secs_m.keys()) == set(secs_i.keys())
        for k in secs_m:
            sm, si = secs_m[k], secs_i[k]
            assert sm.n3d() == si.n3d(), f"{k}: n3d mismatch {sm.n3d()} vs {si.n3d()}"
            for j in range(sm.n3d()):
                pm = (sm.x3d(j), sm.y3d(j), sm.z3d(j), sm.diam3d(j))
                pi = (si.x3d(j), si.y3d(j), si.z3d(j), si.diam3d(j))
                assert pm == pi, f"{fname} {k}[{j}] mismatch: {pm} vs {pi}"


def test_morphio_heterogeneous_neurites():
    fname = find_file(
        "external/MorphIO/src/morphio-external/tests/data/simple-heterogeneous-neurite.swc"
    )
    if not os.path.exists(fname):
        pytest.skip(f"{fname} not found")
    m = morphio_api.MorphIOWrapper(fname)
    # verify distribution groups contiguous counts
    assert len(m.section_type_distribution) == 3
    # verify section names are unique
    names = m.section_index_to_name
    assert len(names) == len(set(names))
    assert names[0] == "soma"
    assert "axon[0]" in names
    assert "dend[0]" in names


def test_morphio_vs_import3d_swc_four_types():
    """Verify bit-for-bit parity across all 4 standard section types: soma, axon, dend, apic"""
    fname = find_file(
        "external/MorphIO/src/morphio-external/tests/data/three_point_soma.swc"
    )
    if not os.path.exists(fname):
        pytest.skip(f"{fname} not found")

    h.load_file("stdlib.hoc")
    h.load_file("import3d.hoc")

    # MorphIO
    c_m = h.HocBasicCell()
    h.morphio_load(c_m, fname)

    # Import3d
    r = h.Import3d_SWC_read()
    r.quiet = 1
    r.input(fname)
    g = h.Import3d_GUI(r, 0)
    c_i = h.HocBasicCell()
    g.instantiate(c_i)

    secs_m = {s.name().split(".")[-1]: s for s in c_m.all if s.n3d() > 0}
    secs_i = {s.name().split(".")[-1]: s for s in c_i.all if s.n3d() > 0}

    assert len(secs_m) == 5
    assert set(secs_m.keys()) == set(secs_i.keys())
    assert set(secs_m.keys()) == {"soma[0]", "axon[0]", "axon[1]", "dend[0]", "apic[0]"}

    for k in secs_m:
        sm, si = secs_m[k], secs_i[k]
        assert sm.n3d() == si.n3d(), f"{k}: n3d mismatch {sm.n3d()} vs {si.n3d()}"
        for j in range(sm.n3d()):
            pm = (sm.x3d(j), sm.y3d(j), sm.z3d(j), sm.diam3d(j))
            pi = (si.x3d(j), si.y3d(j), si.z3d(j), si.diam3d(j))
            assert pm == pi, f"{fname} {k}[{j}] mismatch: {pm} vs {pi}"


def test_morphio_vs_import3d_pia_asc():
    """Verify bit-for-bit parity on real Neurolucida ASC reconstruction with contour soma and axon"""
    fname = find_file("external/MorphIO/src/morphio-external/tests/data/pia.asc")
    if not os.path.exists(fname):
        pytest.skip(f"{fname} not found")

    h.load_file("stdlib.hoc")
    h.load_file("import3d.hoc")

    c_m = h.HocBasicCell()
    h.morphio_load(c_m, fname)

    r = h.Import3d_Neurolucida3()
    r.quiet = 1
    r.input(fname)
    g = h.Import3d_GUI(r, 0)
    c_i = h.HocBasicCell()
    g.instantiate(c_i)

    secs_m = {s.name().split(".")[-1]: s for s in c_m.all if s.n3d() > 0}
    secs_i = {s.name().split(".")[-1]: s for s in c_i.all if s.n3d() > 0}

    assert set(secs_m.keys()) == set(secs_i.keys())
    assert set(secs_m.keys()) == {"soma[0]", "axon[0]"}

    for k in secs_m:
        sm, si = secs_m[k], secs_i[k]
        assert sm.n3d() == si.n3d(), f"{k}: n3d mismatch {sm.n3d()} vs {si.n3d()}"
        for j in range(sm.n3d()):
            pm = (sm.x3d(j), sm.y3d(j), sm.z3d(j), sm.diam3d(j))
            pi = (si.x3d(j), si.y3d(j), si.z3d(j), si.diam3d(j))
            assert pm == pi, f"{fname} {k}[{j}] mismatch: {pm} vs {pi}"


def test_morphio_vs_import3d_neurite_parity():
    """Verify bit-for-bit parity on all neurite branches for single-point soma morphologies"""
    h.load_file("stdlib.hoc")
    h.load_file("import3d.hoc")

    test_files = [
        ("external/MorphIO/src/morphio-external/tests/data/nrn_ordering.swc", "swc"),
        ("external/MorphIO/src/morphio-external/tests/data/mono-type.asc", "asc"),
        (
            "external/MorphIO/src/morphio-external/tests/data/nested_single_children.asc",
            "asc",
        ),
    ]

    for rel_path, fmt in test_files:
        fname = find_file(rel_path)
        if not os.path.exists(fname):
            continue

        c_m = h.HocBasicCell()
        h.morphio_load(c_m, fname)

        if fmt == "swc":
            r = h.Import3d_SWC_read()
        else:
            r = h.Import3d_Neurolucida3()
        r.quiet = 1
        r.input(fname)
        g = h.Import3d_GUI(r, 0)
        c_i = h.HocBasicCell()
        g.instantiate(c_i)

        secs_m = {s.name().split(".")[-1]: s for s in c_m.all if s.n3d() > 0}
        secs_i = {s.name().split(".")[-1]: s for s in c_i.all if s.n3d() > 0}

        # Compare neurite sections (exclude soma)
        neurites_m = {k: v for k, v in secs_m.items() if not k.startswith("soma")}
        neurites_i = {k: v for k, v in secs_i.items() if not k.startswith("soma")}

        assert set(neurites_m.keys()) == set(
            neurites_i.keys()
        ), f"{fname}: neurite section set mismatch"

        for k in neurites_m:
            sm, si = neurites_m[k], neurites_i[k]
            assert (
                sm.n3d() == si.n3d()
            ), f"{fname} {k}: n3d mismatch {sm.n3d()} vs {si.n3d()}"
            for j in range(sm.n3d()):
                pm = (sm.x3d(j), sm.y3d(j), sm.z3d(j), sm.diam3d(j))
                pi = (si.x3d(j), si.y3d(j), si.z3d(j), si.diam3d(j))
                assert pm == pi, f"{fname} {k}[{j}] mismatch: {pm} vs {pi}"


def test_morphio_h5_morphologies():
    """Verify loading of HDF5 v1 morphologies (pyramidal cell and large astrocyte)"""
    for rel_path, expected_secs in [
        ("external/MorphIO/src/morphio-external/tests/data/h5/v1/Neuron.h5", 85),
        ("external/MorphIO/src/morphio-external/tests/data/astrocyte.h5", 1316),
    ]:
        fname = find_file(rel_path)
        if not os.path.exists(fname):
            continue
        m = morphio_api.MorphIOWrapper(fname)
        assert len(m.section_index_to_name) == expected_secs
        assert m.section_index_to_name[0] == "soma"
        assert len(m.morph_as_hoc()) > 0


def test_morphio_multi_contour_error():
    """Verify multi-contour ASC files raise exception per MorphIO specification"""
    fname = find_file("test/hoctests/tests/test_pt3dstyle_stack.asc")
    if not os.path.exists(fname):
        pytest.skip(f"{fname} not found")

    with pytest.raises(RuntimeError):
        morphio_api.MorphIOWrapper(fname)
