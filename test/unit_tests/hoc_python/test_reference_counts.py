import sys
import neuron
from neuron import h


def test_section():
    section = h.Section(name='section')
    assert sys.getrefcount(section) == 2

    # {Py_tp_repr, (void*) pysec_repr_safe},
    repr(section)
    assert sys.getrefcount(section) == 2

    # {Py_tp_hash, (void*) pysec_hash_safe},
    hash(section)
    assert sys.getrefcount(section) == 2

    #{Py_tp_call, (void*) NPySecObj_call_safe},
    section()
    assert sys.getrefcount(section) == 2

    # {Py_tp_getattro, (void*) section_getattro_safe},
    section.L
    assert sys.getrefcount(section) == 2

    section.Ra
    assert sys.getrefcount(section) == 2

    section.nseg
    assert sys.getrefcount(section) == 2

    section.rallbranch
    assert sys.getrefcount(section) == 2

    # not sure what the purpose of this is; return a dict of
    # {'L': None, 'Ra': None, 'nseg': None, 'rallbranch': None}
    d = section.__dict__
    assert sys.getrefcount(d) == 2
    assert sys.getrefcount(section) == 2

    # {Py_tp_richcompare, (void*) pysec_richcmp_safe},
    section == section
    section != section
    section < section
    section <= section
    section > section
    section >= section
    assert sys.getrefcount(section) == 2

    # {Py_tp_iter, (void*) seg_of_section_iter_safe},

    # {Py_tp_methods, (void*) NPySecObj_methods},
    dir(section)
    assert sys.getrefcount(section) == 2

    # {Py_tp_doc, (void*) "Section objects"},
    help(section)
    assert sys.getrefcount(section) == 2

    # {Py_sq_contains, (void*) NPySecObj_contains_safe},
    "foo" in section
    assert sys.getrefcount(section) == 2

    #static PyType_Slot nrnpy_AllSegOfSecIterType_slots[] = {
    it = section.allseg()
    assert sys.getrefcount(it) == 2
    assert sys.getrefcount(section) == 3
    del it

    # static PyType_Spec nrnpy_SegOfSecIterType_spec = {
    it = iter(section)
    assert sys.getrefcount(it) == 2
    assert sys.getrefcount(section) == 3

    n = next(it)
    assert sys.getrefcount(it) == 2
    assert sys.getrefcount(section) == 4
    del n
    assert sys.getrefcount(section) == 3
    del it
    assert sys.getrefcount(section) == 2


def test_segment():
    section = h.Section(name='section_segment')
    segment = section(0.5)
    assert sys.getrefcount(section) == 3

    assert sys.getrefcount(segment) == 2

    # {Py_tp_repr, (void*) pyseg_repr_safe},
    repr(segment)
    assert sys.getrefcount(segment) == 2

    # {Py_tp_hash, (void*) pyseg_hash_safe},
    hash(segment)
    assert sys.getrefcount(segment) == 2

    # {Py_tp_getattro, (void*) segment_getattro_safe},
    segment.v
    assert sys.getrefcount(segment) == 2

    #} else if ((otype = PyDict_GetItemString(pmech_types, n)) != NULL) {
    #    int type = PyInt_AsLong(otype);
    #    // printf("segment_getattr type=%d\n", type);
    #    Node* nd = node_exact(sec, self->x_);
    #    Prop* p = nrn_mechanism(type, nd);
    #    if (!p) {
    #        rv_noexist(sec, n, self->x_, 1);
    #        return nullptr;
    #    } else {
    #        result = (PyObject*) new_pymechobj(self, p);
    #    }

    #} else if (strncmp(n, "_ref_", 5) == 0) {
    segment._ref_v
    assert sys.getrefcount(segment) == 2

    #todo: need to find hoc_built_in_symlist values

    # not sure what the purpose of this is; return a dict of
    # {'v': None, 'diam': None, 'cm': None}
    d = segment.__dict__
    assert sys.getrefcount(d) == 2
    assert sys.getrefcount(section) == 3

    # {Py_tp_richcompare, (void*) pyseg_richcmp_safe},
    segment == segment
    segment != segment
    segment < segment
    segment <= segment
    segment > segment
    segment >= segment
    assert sys.getrefcount(segment) == 2

    # {Py_tp_iter, (void*) mech_of_segment_iter_safe},

    # {Py_tp_methods, (void*) NPySegObj_methods},
    # {Py_tp_members, (void*) NPySegObj_members},
    dir(segment)
    assert sys.getrefcount(segment) == 2

    # {Py_tp_doc, (void*) "Segment objects"},
    help(segment)
    assert sys.getrefcount(segment) == 2

    # {Py_sq_contains, (void*) NPySegObj_contains_safe},
    "foo" in segment
    assert sys.getrefcount(segment) == 2

    #static PyType_Spec nrnpy_SegmentType_spec = {
    #    "nrn.Segment",
    #    sizeof(NPySegObj),
    #    0,
    #    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    #    nrnpy_SegmentType_slots,
    #};


def test_mechanisms():
    assert "XXX"
    #static PyType_Slot nrnpy_MechOfSegIterType_slots[] = {
    #    {Py_tp_dealloc, (void*) NPyMechOfSegIter_dealloc_safe},
    #    {Py_tp_iter, (void*) PyObject_SelfIter},
    #    {Py_tp_iternext, (void*) mech_of_seg_next_safe},
    #    {Py_tp_doc, (void*) "Iterate over Mechanisms in a Segment of a Section"},
    #    {0, 0},
    #};

    #} else if (strcmp(n, "__dict__") == 0) {
    #    nb::dict out_dict{};
    #    int cnt = mechsym->s_varn;
    #    for (int i = 0; i < cnt; ++i) {
    #        Symbol* s = mechsym->u.ppsym[i];
    #        if (!striptrail(buf, bufsz, s->name, mname)) {
    #            strcpy(buf, s->name);
    #        }
    #        out_dict[buf] = nb::none();
    #    }
    #    // FUNCTION and PROCEDURE
    #    for (auto& it: nrn_mech2funcs_map[self->prop_->_type]) {
    #        out_dict[it.first.c_str()] = nb::none();
    #    }
    #    result = out_dict.release().ptr();

    #static PyType_Spec nrnpy_MechOfSegIterType_spec = {
    #    "nrn.MechOfSegIter",
    #    sizeof(NPyMechOfSegIter),
    #    0,
    #    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    #    nrnpy_MechOfSegIterType_slots,
    #};

    #static PyType_Slot nrnpy_MechanismType_slots[] = {
    #    {Py_tp_dealloc, (void*) NPyMechObj_dealloc_safe},
    #    {Py_tp_repr, (void*) pymech_repr_safe},
    #    {Py_tp_getattro, (void*) mech_getattro_safe},
    #    {Py_tp_setattro, (void*) mech_setattro_safe},
    #    {Py_tp_iter, (void*) var_of_mech_iter_safe},
    #    {Py_tp_methods, (void*) NPyMechObj_methods},
    #    {Py_tp_members, (void*) NPyMechObj_members},
    #    {Py_tp_init, (void*) NPyMechObj_init_safe},
    #    {Py_tp_new, (void*) NPyMechObj_new_safe},
    #    {Py_tp_doc, (void*) "Mechanism objects"},
    #    {0, 0},
    #};
    #static PyType_Spec nrnpy_MechanismType_spec = {
    #    "nrn.Mechanism",
    #    sizeof(NPyMechObj),
    #    0,
    #    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    #    nrnpy_MechanismType_slots,
    #};

    #static PyType_Slot nrnpy_MechFuncType_slots[] = {
    #    {Py_tp_dealloc, (void*) NPyMechFunc_dealloc_safe},
    #    {Py_tp_repr, (void*) pymechfunc_repr_safe},
    #    {Py_tp_methods, (void*) NPyMechFunc_methods},
    #    {Py_tp_call, (void*) NPyMechFunc_call_safe},
    #    {Py_tp_doc, (void*) "Mechanism Function"},
    #    {0, 0},
    #};
    #static PyType_Spec nrnpy_MechFuncType_spec = {
    #    "nrn.MechFunc",
    #    sizeof(NPyMechFunc),
    #    0,
    #    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    #    nrnpy_MechFuncType_slots,
    #};

    #static PyType_Slot nrnpy_VarOfMechIterType_slots[] = {
    #    {Py_tp_dealloc, (void*) NPyVarOfMechIter_dealloc_safe},
    #    {Py_tp_iter, (void*) PyObject_SelfIter},
    #    {Py_tp_iternext, (void*) var_of_mech_next_safe},
    #    {Py_tp_doc, (void*) "Iterate over variables  in a Mechanism"},
    #    {0, 0},
    #};
    #static PyType_Spec nrnpy_VarOfMechIterType_spec = {
    #    "nrn.VarOfMechIter",
    #    sizeof(NPyVarOfMechIter),
    #    0,
    #    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    #    nrnpy_VarOfMechIterType_slots,
    #};

def test_range():
    section = h.Section(name='section')
    for name in ('vext', 'xraxial', 'xg', 'xc', ):
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

        # {Py_sq_item, (void*) rv_getitem_safe},
        #range_var[1] = 1
        #assert sys.getrefcount(range_var) == 2

        assert sys.getrefcount(section) == 3

#def test_opaque_pointer():
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
