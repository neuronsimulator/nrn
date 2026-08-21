// Verify that a runtime PythonObject provider receives component reads and
// writes even when NEURON itself is compiled without Python support. This test
// supplies minimal callbacks and exact values; it does not link to Python.
#include <array>
#include <cstring>
#include <iostream>

#include "neuronapi.h"
#include "nrnpy.h"

using std::cerr;
using std::endl;

extern "C" void modl_reg() {}

static int read_calls{};
static int write_calls{};
static bool read_frame_matches{};
static bool write_frame_matches{};
static double assigned_value{};

static bool check(bool cond, const char* msg) {
    if (!cond) {
        cerr << "FAIL: " << msg << endl;
    }
    return cond;
}

static void read_component(Object* obj, Symbol* member, int nindex, int isfunc) {
    ++read_calls;
    Object* frame_obj = nrn_object_pop();
    read_frame_matches = frame_obj == obj && member &&
                         std::strcmp(nrn_symbol_name(member), "answer") == 0 && nindex == 0 &&
                         isfunc == 0;
    if (frame_obj) {
        nrn_object_unref(frame_obj);
    }
    nrn_double_push(42.5);
}

static void write_component(Object* obj, int) {
    ++write_calls;
    assigned_value = nrn_double_pop();
    Object* frame_obj = nrn_object_pop();
    Symbol* member = nrn_symbol_pop();
    int nindex = nrn_int_pop();
    write_frame_matches = frame_obj == obj && member &&
                          std::strcmp(nrn_symbol_name(member), "answer") == 0 && nindex == 0;
    if (frame_obj) {
        nrn_object_unref(frame_obj);
    }
}

int main(void) {
    static std::array<const char*, 4> argv = {"runtime_pythonobject_marshaling",
                                              "-nogui",
                                              "-nopython",
                                              nullptr};
    nrn_init(3, argv.data());

    bool ok = true;
    Symbol* pyobject = nrn_symbol("PythonObject");
    ok &= check(pyobject != nullptr, "stub PythonObject class is registered");

    // A host provider identifies its PythonObject template and installs the
    // two callbacks. In a Python-disabled build these pointers are otherwise
    // null, so ordinary stub behavior remains unchanged until this happens.
    nrnpy_pyobj_sym_ = pyobject;
    neuron::python::methods.py2n_component = read_component;
    neuron::python::methods.hpoasgn = write_component;

    Object* obj = nrn_object_new(pyobject, 0);
    ok &= check(obj != nullptr, "PythonObject constructed");
    ok &= check(nrn_hoc_call("func read_component() { return $o1.answer }") == 0,
                "read helper defined");
    ok &= check(nrn_hoc_call("proc write_component() { $o1.answer = $2 }") == 0,
                "write helper defined");

    nrn_object_push(obj);
    nrn_function_call(nrn_symbol("read_component"), 1);
    ok &= check(nrn_double_pop() == 42.5, "component read returns the provider's exact value");
    ok &= check(read_calls == 1, "component read callback runs exactly once");
    ok &= check(read_frame_matches, "component read callback receives the expected frame");

    nrn_object_push(obj);
    nrn_double_push(73.25);
    nrn_function_call(nrn_symbol("write_component"), 2);
    nrn_double_pop();
    ok &= check(assigned_value == 73.25, "component write receives the exact assigned value");
    ok &= check(write_calls == 1, "component write callback runs exactly once");
    ok &= check(write_frame_matches, "component write callback receives the expected frame");

    nrn_object_unref(obj);
    return ok ? 0 : 1;
}
