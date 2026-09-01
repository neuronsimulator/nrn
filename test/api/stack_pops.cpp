// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
// Exercises nrn_symbol_pop and nrn_object_pop's nil handling, the stack-pop
// primitives used to unwind an interpreter frame from a binding (the
// HOC-to-Python component read/write-back path). nrn_symbol_pop pops a Symbol
// the interpreter left on the stack; nrn_object_pop pops an object but returns
// NULL for a nil objref instead of dereferencing it to take a reference.
#include <array>
#include <iostream>
#include "neuronapi.h"

using std::cerr;
using std::endl;

// No public API leaves a bare Symbol (STACK_IS_SYM) on the stack -- the
// interpreter pushes one during object-component access -- so the test uses the
// internal push to put a Symbol there, the way nrniv would mid-expression.
extern void hoc_pushs(Symbol*);
extern void hoc_push_ndim(int);

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

    // Push a sentinel FIRST, below everything else the test does. The stack is
    // LIFO, so if every push below is matched by exactly one pop the sentinel
    // resurfaces on top at the end; recovering it then proves the pops neither
    // over-popped into what lay beneath them nor left anything behind. A
    // sentinel pushed *after* the pops could prove neither -- it would only ever
    // probe the current top.
    const double SENTINEL = 424242.0;
    nrn_double_push(SENTINEL);

    // nrn_symbol_pop returns the Symbol on top of the stack, LIFO.
    Symbol* v = nrn_symbol("v");
    Symbol* t = nrn_symbol("t");
    ok &= check(v != nullptr && t != nullptr, "symbols v and t resolve");
    hoc_pushs(v);
    hoc_pushs(t);
    ok &= check(nrn_stack_type() == STACK_IS_SYM, "stack top is a symbol before nrn_symbol_pop");
    ok &= check(nrn_symbol_pop() == t, "symbol pop returns the last pushed symbol");
    ok &= check(nrn_symbol_pop() == v, "symbol pop returns the earlier symbol (LIFO)");

    // nrn_object_pop returns a real object (reference-counted).
    Object* vec = nrn_object_new(nrn_symbol("Vector"), 0);
    ok &= check(vec != nullptr, "Vector constructed");
    nrn_object_push(vec);
    Object* got = nrn_object_pop();
    ok &= check(got == vec, "object pop returns the pushed object");
    if (got) {
        nrn_object_unref(got);
    }

    // nrn_object_pop returns NULL for a nil object reference instead of
    // crashing -- a naive pop would dereference the NULL to take a reference and
    // segfault here.
    nrn_object_push(nullptr);
    ok &= check(nrn_object_pop() == nullptr, "object pop returns NULL for a nil objref");

    // nrn_int_pop handles both ordinary USERINT values and the distinct array-
    // dimension marker used by indexed object-component access. Exercise exact
    // values and LIFO ordering for both representations through the same API.
    nrn_int_push(3);
    ok &= check(nrn_int_pop() == 3, "int pop returns an ordinary integer");
    hoc_push_ndim(2);
    hoc_push_ndim(7);
    ok &= check(nrn_int_pop() == 7, "int pop returns the last pushed ndim marker");
    ok &= check(nrn_int_pop() == 2, "int pop returns the earlier ndim marker (LIFO)");

    // Balance: with every push above consumed by exactly one pop, the sentinel
    // from the very start is what remains on top.
    ok &= check(nrn_double_pop() == SENTINEL,
                "sentinel from before the pops is intact -- the stack is balanced");

    return ok ? 0 : 1;
}
