import sys
from neuron import h


def test_section():
    section = h.Section(name="section")

    # this is 1 + the extra ref from getting called by sys.getrefcount
    base_refcount = 2

    assert sys.getrefcount(section) == base_refcount

    # {Py_tp_repr, (void*) pysec_repr_safe},
    repr(section)
    assert sys.getrefcount(section) == base_refcount

    # {Py_tp_hash, (void*) pysec_hash_safe},
    hash(section)
    assert sys.getrefcount(section) == base_refcount

    # {Py_tp_call, (void*) NPySecObj_call_safe},
    section()
    assert sys.getrefcount(section) == base_refcount

    # {Py_tp_getattro, (void*) section_getattro_safe},
    section.L
    assert sys.getrefcount(section) == base_refcount

    section.Ra
    assert sys.getrefcount(section) == base_refcount

    section.nseg
    assert sys.getrefcount(section) == base_refcount

    section.rallbranch
    assert sys.getrefcount(section) == base_refcount

    d = section.__dict__
    assert sys.getrefcount(d) == 2
    assert sys.getrefcount(section) == base_refcount

    # {Py_tp_richcompare, (void*) pysec_richcmp_safe},
    section == section
    section != section
    section < section
    section <= section
    section > section
    section >= section
    assert sys.getrefcount(section) == base_refcount

    # {Py_tp_iter, (void*) seg_of_section_iter_safe},

    # {Py_tp_methods, (void*) NPySecObj_methods},
    for method in (
        "name",
        "hname",
        "cell",
        "hoc_internal_name",
        "disconnect",
        "parentseg",
        "trueparentseg",
        "orientation",
        "children",
        "subtree",
        "wholetree",
        "n3d",
        "is_pysec",
        "psection",
    ):
        getattr(section, method)()
        assert sys.getrefcount(section) == base_refcount

    dir(section)
    assert sys.getrefcount(section) == base_refcount

    # {Py_tp_doc, (void*) "Section objects"},
    help(section)
    assert sys.getrefcount(section) == base_refcount

    # {Py_sq_contains, (void*) NPySecObj_contains_safe},
    "foo" in section
    assert sys.getrefcount(section) == base_refcount

    # static PyType_Slot nrnpy_AllSegOfSecIterType_slots[] = {
    it = section.allseg()
    assert sys.getrefcount(it) == 2
    assert sys.getrefcount(section) == base_refcount + 1
    del it

    # static PyType_Spec nrnpy_SegOfSecIterType_spec = {
    it = iter(section)
    assert sys.getrefcount(it) == 2
    assert sys.getrefcount(section) == base_refcount + 1

    n = next(it)
    assert sys.getrefcount(it) == 2
    assert sys.getrefcount(section) == base_refcount + 2
    del n
    assert sys.getrefcount(section) == base_refcount + 1
    del it
    assert sys.getrefcount(section) == base_refcount


def test_segment():
    section = h.Section(name="section_segment")
    segment = section(0.5)
    assert sys.getrefcount(section) == 3

    # this is 1 + the extra ref from getting called by sys.getrefcount
    base_refcount = 2
    assert sys.getrefcount(segment) == base_refcount

    # {Py_tp_repr, (void*) pyseg_repr_safe},
    repr(segment)
    assert sys.getrefcount(segment) == base_refcount

    # {Py_tp_hash, (void*) pyseg_hash_safe},
    hash(segment)
    assert sys.getrefcount(segment) == base_refcount

    # {Py_tp_getattro, (void*) segment_getattro_safe},
    segment.v
    assert sys.getrefcount(segment) == base_refcount

    # } else if (strncmp(n, "_ref_", 5) == 0) {
    segment._ref_v
    assert sys.getrefcount(segment) == base_refcount

    # todo: need to find hoc_built_in_symlist values

    d = segment.__dict__
    assert sys.getrefcount(d) == 2
    assert sys.getrefcount(segment) == base_refcount

    # {Py_tp_richcompare, (void*) pyseg_richcmp_safe},
    segment == segment
    segment != segment
    segment < segment
    segment <= segment
    segment > segment
    segment >= segment
    assert sys.getrefcount(segment) == base_refcount

    # {Py_tp_iter, (void*) mech_of_segment_iter_safe},

    # {Py_tp_methods, (void*) NPySegObj_methods},
    for method in ("point_processes", "node_index", "area", "ri", "volume"):
        getattr(segment, method)()
        assert sys.getrefcount(segment) == base_refcount

    # {Py_tp_members, (void*) NPySegObj_members},
    for method in (
        "x",
        "sec",
    ):
        getattr(segment, method)
        assert sys.getrefcount(segment) == base_refcount

    # {Py_tp_doc, (void*) "Segment objects"},
    help(segment)
    assert sys.getrefcount(segment) == base_refcount

    # {Py_sq_contains, (void*) NPySegObj_contains_safe},
    "foo" in segment
    assert sys.getrefcount(segment) == base_refcount

    assert sys.getrefcount(section) == 3


def test_mechanisms():
    section = h.Section(name="section")
    section.insert("hh")

    # this is 1 + the extra ref from getting called by sys.getrefcount
    base_refcount = 2

    mech = section(0.5).hh
    assert sys.getrefcount(mech) == base_refcount

    # note: a mechanism at the same place is not the same one, ie: mech != mech0
    mech0 = section(0.5).hh
    assert sys.getrefcount(mech) == base_refcount
    del mech0

    #    {Py_tp_repr, (void*) pymech_repr_safe},
    repr(mech)
    assert sys.getrefcount(mech) == base_refcount

    #    {Py_tp_getattro, (void*) mech_getattro_safe},

    #    {Py_tp_iter, (void*) var_of_mech_iter_safe},
    it = iter(mech)
    assert sys.getrefcount(it) == 2
    assert sys.getrefcount(mech) == base_refcount + 1

    help(it)
    assert sys.getrefcount(it) == 2

    n = next(it)
    assert sys.getrefcount(it) == 2
    assert sys.getrefcount(mech) == base_refcount + 2
    del n
    assert sys.getrefcount(mech) == base_refcount + 1
    del it
    assert sys.getrefcount(mech) == base_refcount

    #    {Py_tp_methods, (void*) NPyMechObj_methods},
    for method in (
        "name",
        "is_ion",
        "segment",
    ):
        getattr(mech, method)()
        assert sys.getrefcount(mech) == base_refcount

    #    {Py_tp_members, (void*) NPyMechObj_members},
    # there are none

    #    {Py_tp_doc, (void*) "Mechanism objects"},
    help(mech)
    assert sys.getrefcount(mech) == base_refcount

    # } else if (strcmp(n, "__dict__") == 0) {
    mech.__dict__
    assert sys.getrefcount(mech) == base_refcount

    # static PyType_Spec nrnpy_MechOfSegIterType_spec = {
    segment = section(0.5)
    assert sys.getrefcount(segment) == base_refcount

    it = iter(segment)
    assert sys.getrefcount(it) == base_refcount

    # static PyType_Slot nrnpy_MechFuncType_slots[] = {
    mech_func = mech.rates
    assert sys.getrefcount(mech_func) == base_refcount

    # {Py_tp_repr, (void*) pymechfunc_repr_safe},
    repr(mech_func)
    assert sys.getrefcount(mech_func) == base_refcount

    # {Py_tp_methods, (void*) NPyMechFunc_methods},
    for method in (
        "name",
        "mech",
    ):
        getattr(mech_func, method)()
        assert sys.getrefcount(mech_func) == base_refcount

    # {Py_tp_call, (void*) NPyMechFunc_call_safe},
    mech_func(0)
    assert sys.getrefcount(mech_func) == base_refcount

    # {Py_tp_doc, (void*) "Mechanism Function"},
    help(mech_func)
    assert sys.getrefcount(mech_func) == base_refcount

    assert sys.getrefcount(section) == 5
    assert sys.getrefcount(segment) == 3
    assert sys.getrefcount(mech) == 3


def test_range():
    section = h.Section(name="section")
    for name in (
        "vext",
        "xraxial",
        "xg",
        "xc",
    ):
        range_var = getattr(section, name)
        assert sys.getrefcount(range_var) == 2

        # {Py_tp_methods, (void*) NPyRangeVar_methods},
        dir(range_var)
        assert sys.getrefcount(range_var) == 2

        # {Py_tp_doc, (void*) "Range Variable Array objects"},
        help(range_var)
        assert sys.getrefcount(range_var) == 2

        # {Py_sq_length, (void*) rv_len_safe},
        len(range_var)
        assert sys.getrefcount(range_var) == 2

        assert sys.getrefcount(section) == 3


# def test_opaque_pointer():
#    #static PyType_Slot nrnpy_OpaquePointerType_slots[] = {
#    #    {Py_tp_doc, (void*) "Opaque pointer."},
#    #    {0, 0},
#    #};
#    #static PyType_Spec nrnpy_OpaquePointerType_spec = {
#    #    "nrn.OpaquePointer",
#    #    sizeof(NPyOpaquePointer),
#    #    0,
#    #    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
#    #    nrnpy_OpaquePointerType_slots,
#    #};
