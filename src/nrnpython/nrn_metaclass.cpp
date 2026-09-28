// Derived from
// https://github.com/python/cpython/pull/93012#issuecomment-1138876646

#include <Python.h>

PyObject* nrn_type_from_metaclass(PyTypeObject* metaclass,
                                  PyObject* mod,
                                  PyType_Spec* spec,
                                  PyObject* base) {
#if PY_VERSION_HEX >= 0x030C0000
    // Life is good, PyType_FromMetaclass() is available
    PyObject* result = PyType_FromMetaclass(metaclass, mod, spec, base);
    if (!result) {
        // fail("nanobind::detail::nb_type_new(\"%s\"): type construction failed!", t->name);
        return result;
    }
#else
    /* The fallback code below is cursed. It provides an alternative when
       PyType_FromMetaclass() is not available we are furthermore *not*
       targeting the stable ABI interface. It calls PyType_FromSpec() to create
       a tentative type, copies its contents into a larger type with a
       different metaclass, then lets the original type expire. */

    PyObject* temp = PyType_FromSpecWithBases(spec, base);  // since 3.3
    Py_XINCREF(temp);
    PyHeapTypeObject* temp_ht = (PyHeapTypeObject*) temp;
    PyTypeObject* temp_tp = &temp_ht->ht_type;

    Py_INCREF(temp_ht->ht_name);
    Py_INCREF(temp_ht->ht_qualname);
    Py_INCREF(temp_tp->tp_base);
    Py_XINCREF(temp_ht->ht_slots);

    PyObject* result = PyType_GenericAlloc(metaclass, 0);
    if (!temp || !result) {
        // fail("nanobind::detail::nb_type_new(\"%s\"): type construction failed!", t->name);
        return nullptr;
    }
    PyHeapTypeObject* ht = (PyHeapTypeObject*) result;
    PyTypeObject* tp = &ht->ht_type;

    memcpy(ht, temp_ht, sizeof(PyHeapTypeObject));

    tp->ob_base.ob_base.ob_type = metaclass;
    tp->ob_base.ob_base.ob_refcnt = 1;
    tp->ob_base.ob_size = 0;
    tp->tp_as_async = &ht->as_async;
    tp->tp_as_number = &ht->as_number;
    tp->tp_as_sequence = &ht->as_sequence;
    tp->tp_as_mapping = &ht->as_mapping;
    tp->tp_as_buffer = &ht->as_buffer;
    tp->tp_name = strdup(spec->name);  // name_copy;
    tp->tp_flags = spec->flags | Py_TPFLAGS_HEAPTYPE;

    tp->tp_dict = tp->tp_bases = tp->tp_mro = tp->tp_cache = tp->tp_subclasses = tp->tp_weaklist =
        nullptr;
    ht->ht_cached_keys = nullptr;
    tp->tp_version_tag = 0;

    PyType_Ready(tp);
    Py_DECREF(temp);

    // PyType_FromMetaclass consistently sets the __module__ to 'hoc' (never 'neuron.hoc')
    // So rather than use PyModule_GetName or PyObject_GetAttrString(mod, "__name__") just
    // explicitly set to 'hoc'
    PyObject* module_name = PyUnicode_FromString("hoc");
    if (PyObject_SetAttrString(result, "__module__", module_name) < 0) {
        Py_DECREF(module_name);
        return nullptr;
    }
    Py_DECREF(module_name);
#endif

    return result;
}
