// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
// Exercises nrn_symbol_pop and nrn_object_pop_safe, the stack-pop primitives
// used to unwind an interpreter frame from a binding (the HOC-to-Python
// component read/write-back path). nrn_symbol_pop pops a Symbol the interpreter
// left on the stack; nrn_object_pop_safe pops an object but returns NULL for a
// nil objref instead of dereferencing it the way nrn_object_pop would.
#include <array>
#include <iostream>
#include "neuronapi.h"

using std::cerr;
using std::endl;

// No public API leaves a bare Symbol (STACK_IS_SYM) on the stack -- the
// interpreter pushes one during object-component access -- so the test uses the
// internal push to put a Symbol there, the way nrniv would mid-expression.
extern void hoc_pushs(Symbol*);

extern "C" void modl_reg() {}

static bool check(bool cond, const char* msg) {
    if (!cond) {
        cerr << "FAIL: " << msg << endl;
    }
    return cond;
}

int main(void) {
    static std::array<const char*, 4> argv = {"stack_pops", "-nogui", "-nopython", nullptr};
    nrn_init(3, argv.data());

    bool ok = true;

    // nrn_symbol_pop returns the Symbol on top of the stack, LIFO.
    Symbol* v = nrn_symbol("v");
    Symbol* t = nrn_symbol("t");
    ok &= check(v != nullptr && t != nullptr, "symbols v and t resolve");
    hoc_pushs(v);
    hoc_pushs(t);
    ok &= check(nrn_stack_type() == STACK_IS_SYM, "stack top is a symbol before nrn_symbol_pop");
    ok &= check(nrn_symbol_pop() == t, "symbol pop returns the last pushed symbol");
    ok &= check(nrn_symbol_pop() == v, "symbol pop returns the earlier symbol (LIFO)");

    // nrn_object_pop_safe returns a real object (reference-counted).
    Object* vec = nrn_object_new(nrn_symbol("Vector"), 0);
    ok &= check(vec != nullptr, "Vector constructed");
    nrn_object_push(vec);
    Object* got = nrn_object_pop_safe();
    ok &= check(got == vec, "safe pop returns the pushed object");
    if (got) {
        nrn_object_unref(got);
    }

    // nrn_object_pop_safe returns NULL for a nil object reference instead of
    // crashing -- nrn_object_pop would dereference the NULL to ref it and
    // segfault here.
    nrn_object_push(nullptr);
    ok &= check(nrn_object_pop_safe() == nullptr, "safe pop returns NULL for a nil objref");

    // Stack balance: the pops above consumed exactly what was pushed.
    nrn_double_push(5.0);
    ok &= check(nrn_double_pop() == 5.0, "stack is balanced after the pops");

    return ok ? 0 : 1;
}
