#include <catch2/catch_test_macros.hpp>
#include <Python.h>

#include "nrnpython.h"
#include "section.h"

extern PyObject* nrnpy_nrn(void);
extern Object* (*nrnpy_seg_from_sec_x)(Section*, double);
extern Prop* prop_alloc(Prop**, int, Node*);


/// Creates a bare section with its property initialized
struct TestSection {
    TestSection() {
        auto prop = prop_alloc(&(sec.prop), CABLESECTION, (Node*) 0);
        prop->dparam[PROP_PY_INDEX] = nullptr;
    }

    ~TestSection() {
        prop_free(&sec.prop);
    }

    Section sec{};
};


TEST_CASE("Test seg_from_sec_x", "[nrn_python::section_segment]") {
    // initialize the Python structures
    nrnpy_nrn();

    SECTION("Ensure newly created section has 1 Py reference", "[seg_from_sec_x]") {
        TestSection tsec;
        // Ensure our pysection is not set and the code path will create it
        REQUIRE(tsec.sec.prop->dparam[PROP_PY_INDEX].get<void*>() == nullptr);

        auto x = nrnpy_seg_from_sec_x(&tsec.sec, 0.5);

        auto pyseg = nrnpy_hoc2pyobject(x);
        REQUIRE(Py_REFCNT(pyseg) == 1);

        auto pysec = ((NPySegObj*) pyseg)->pysec_;
        REQUIRE(Py_REFCNT(pysec) == 1);
    }
}
