// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
// Exercises nrn_symbol_pop and nrn_object_pop's nil handling, the stack-pop
// primitives used to unwind an interpreter frame from a binding (the
// HOC-to-Python component read/write-back path). nrn_symbol_pop pops a Symbol
// the interpreter left on the stack; nrn_object_pop pops an object but returns
// NULL for a nil objref instead of dereferencing it to take a reference.
#include <array>
#include <cstring>
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

static bool awaiting_dimension{};
static bool dimension_is_int{};
static int observed_dimension{-1};

static int inspect_indexed_error(int, char* message) {
    if (awaiting_dimension && std::strstr(message, "not a public member")) {
        awaiting_dimension = false;
        dimension_is_int = nrn_stack_type() == STACK_IS_INT;
        observed_dimension = nrn_int_pop();
    }
    return 0;
}

// The public HOC call produces an indexed-component frame before member lookup
// fails. Its diagnostic callback can therefore inspect/pop the real ndim marker
// using only public APIs. Error recovery below must restore the caller's frame;
// this avoids depending on internal hoc_push_ndim or Python provider hooks.
static bool inspect_dimension(Object* object, const char* function, int expected) {
    observed_dimension = -1;
    dimension_is_int = false;
    awaiting_dimension = true;
    char error[256]{};
    nrn_double_push(424243.0);
    nrn_object_push(object);
    const int status = nrn_function_call_nothrow(nrn_symbol(function), 1, error, sizeof(error));
    bool ok = check(status != 0, "indexed missing member fails through the nothrow API");
    ok &= check(std::strstr(error, "not a public member") != nullptr,
                "indexed lookup preserves its original error");
    ok &= check(!awaiting_dimension, "diagnostic callback inspected the indexed frame");
    ok &= check(dimension_is_int, "ndim marker probes as STACK_IS_INT before popping");
    ok &= check(observed_dimension == expected, "int pop returns the API-produced ndim marker");
    Object* restored = nrn_object_pop();
    ok &= check(restored == object, "failed indexed lookup restores the object argument");
    if (restored) {
        nrn_object_unref(restored);
    }
    ok &= check(nrn_double_pop() == 424243.0, "failed indexed lookup preserves its lower sentinel");
    return ok;
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
    // values for both representations through the same public probe/pop APIs.
    nrn_int_push(3);
    ok &= check(nrn_stack_type() == STACK_IS_INT, "ordinary integer probes as STACK_IS_INT");
    ok &= check(nrn_int_pop() == 3, "int pop returns an ordinary integer");
    ok &= check(nrn_hoc_call("func missing_1() { return $o1.missing[7] }") == 0,
                "one-dimensional indexed helper defined");
    ok &= check(nrn_hoc_call("func missing_2() { return $o1.missing[2][7] }") == 0,
                "two-dimensional indexed helper defined");
    nrn_stdout_redirect(inspect_indexed_error);
    ok &= inspect_dimension(vec, "missing_1", 1);
    ok &= inspect_dimension(vec, "missing_2", 2);
    nrn_stdout_redirect(nullptr);
    nrn_object_unref(vec);

    // Balance: with every push above consumed by exactly one pop, the sentinel
    // from the very start is what remains on top.
    ok &= check(nrn_double_pop() == SENTINEL,
                "sentinel from before the pops is intact -- the stack is balanced");

    return ok ? 0 : 1;
}
