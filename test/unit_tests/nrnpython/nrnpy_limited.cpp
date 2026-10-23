#include "nrnwrap_Python.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdio>

// Smoke tests (happy paths only) for the helpers in src/nrnpython/nrnpy_limited.hpp.
TEST_CASE("nrnpy_limited helpers", "[nrnpython]") {
    if (!Py_IsInitialized()) {
        Py_Initialize();
    }
    SECTION("nrnpy_run_simple_string") {
        REQUIRE(nrnpy_run_simple_string("x = 6 * 7") == 0);
        PyObject* x = PyObject_GetAttrString(PyImport_AddModule("__main__"), "x");
        REQUIRE(PyLong_AsLong(x) == 42);
        Py_DECREF(x);
    }
    SECTION("nrnpy_run_file") {
        FILE* fp = std::tmpfile();
        std::fputs("y = 'from file'\n", fp);
        std::rewind(fp);
        REQUIRE(nrnpy_run_file(fp, "smoke.py") == 0);
        std::fclose(fp);
        REQUIRE(nrnpy_run_simple_string("assert y == 'from file' and __file__ == 'smoke.py'") == 0);
    }
    SECTION("nrnpy_type_mro") {
        PyObject* mro = nrnpy_type_mro(&PyBool_Type);  // (bool, int, object)
        REQUIRE(PyTuple_Size(mro) == 3);
        REQUIRE(PyTuple_GetItem(mro, 1) == reinterpret_cast<PyObject*>(&PyLong_Type));
        Py_DECREF(mro);
    }
    SECTION("nrnpy_tp_alloc and nrnpy_tp_free") {
        PyType_Slot slots[] = {{0, nullptr}};
        PyType_Spec spec{"smoke.T", sizeof(PyObject), 0, Py_TPFLAGS_DEFAULT, slots};
        auto* type = reinterpret_cast<PyTypeObject*>(PyType_FromSpec(&spec));
        PyObject* o = nrnpy_tp_alloc(type);
        REQUIRE(Py_TYPE(o) == type);
        nrnpy_tp_free(o);
        Py_DECREF(type);  // the instance's reference, which tp_dealloc would normally drop
        Py_DECREF(type);
    }
}
