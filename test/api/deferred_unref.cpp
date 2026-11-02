// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
// HOC keeps the temporary object behind `vec.c.x[i]` alive in a deferred slot
// until the next statement starts. nrn_function_call_nothrow and
// nrn_method_call_nothrow release it before returning, as NEURON's Python
// binding does after each call, so the temporary does not outlive the call.
#include <array>
#include <iostream>
#include "neuronapi.h"

using std::cerr;
using std::endl;

extern "C" void modl_reg() {}

static bool check(bool cond, const char* msg) {
    if (!cond) {
        cerr << "FAIL: " << msg << endl;
    }
    return cond;
}

// Number of Vector instances, counted with a nothrow call so nothing else
// (such as nrn_hoc_call, which starts a new statement) flushes the slot first.
static double vector_count(bool& ok) {
    char err[256]{};
    ok &= check(nrn_function_call_nothrow(nrn_symbol("vector_count"), 0, err, sizeof(err)) == 0,
                "vector_count call succeeds");
    return nrn_double_pop();
}

int main(void) {
    static std::array<const char*, 4> argv = {"deferred_unref", "-nogui", "-nopython", nullptr};
    nrn_init(3, argv.data());

    bool ok = true;
    char err[256]{};
    nrn_hoc_call(
        "objref src\n"
        "src = new Vector(3)\n"
        "src.x[1] = 5\n"
        "func vector_count() { localobj l  l = new List(\"Vector\")  return l.count() }\n"
        "func copy_element() { return src.c.x[1] }\n"
        "begintemplate CopyElement\n"
        "public get\n"
        "external src\n"
        "func get() { return src.c.x[1] }\n"
        "endtemplate CopyElement\n"
        "objref reader\n"
        "reader = new CopyElement()\n");

    ok &= check(vector_count(ok) == 1.0, "only src exists before the calls");

    ok &= check(nrn_function_call_nothrow(nrn_symbol("copy_element"), 0, err, sizeof(err)) == 0,
                "function call succeeds");
    ok &= check(nrn_double_pop() == 5.0, "function returns the copied element");
    ok &= check(vector_count(ok) == 1.0, "function call released the temporary copy");

    Object* reader = nrn_symbol_object_get(nrn_symbol("reader"));
    ok &= check(
        nrn_method_call_nothrow(reader, nrn_method_symbol(reader, "get"), 0, err, sizeof(err)) == 0,
        "method call succeeds");
    ok &= check(nrn_double_pop() == 5.0, "method returns the copied element");
    ok &= check(vector_count(ok) == 1.0, "method call released the temporary copy");

    return ok ? 0 : 1;
}
