#pragma once
union Inst;
struct Object;
union Objectdata;
struct Symlist;

/** @brief How many NEURON try { ... } catch(...) { ... } blocks are in the call
 * stack.
 *
 *  Errors inside NEURON are triggered using hoc_execerror, which ultimately
 *  throws an exception. To replicate the old logic, we sometimes need to insert
 *  a try/catch block *only if* there is no try/catch block less deeply nested
 *  on the call stack. This global variable tracks how many such blocks are
 *  currently present on the stack.
 */
extern int nrn_try_catch_nest_depth;

// After a floating-point trap is reported, unblock SIGFPE and rearm the traps.
extern void nrn_fpe_reset_mask();

#if defined(__APPLE__) && !defined(__arm64__)
#include <setjmp.h>
// Intel macOS cannot throw through a SIGFPE frame: std::terminate aborts.
// arm() sigsetjmps. The handler siglongjmps here, then hoc_execerror throws
// from ordinary code. The try that calls arm() is what catches that throw.
struct nrn_fpe_catch_jump {
    nrn_fpe_catch_jump() = default;
    ~nrn_fpe_catch_jump() {
        unlink();
    }
    nrn_fpe_catch_jump(const nrn_fpe_catch_jump&) = delete;
    nrn_fpe_catch_jump& operator=(const nrn_fpe_catch_jump&) = delete;

    void arm();
    // From the signal handler. Returns false when no try is armed.
    static bool leave_handler();

  private:
    void unlink() noexcept;
    static void landed();

    sigjmp_buf buf_{};
    nrn_fpe_catch_jump* prev_{nullptr};
    bool linked_{false};
    static thread_local nrn_fpe_catch_jump* top_;
};
#else
// Linux and Apple silicon throw from the handler. Windows aborts there.
struct nrn_fpe_catch_jump {
    void arm() {}
};
#endif

/** @brief Helper type for incrementing/decrementing nrn_try_catch_nest_depth.
 */
struct try_catch_depth_increment {
    try_catch_depth_increment() {
        ++nrn_try_catch_nest_depth;
    }
    ~try_catch_depth_increment() {
        --nrn_try_catch_nest_depth;
        nrn_fpe_reset_mask();
    }
};

struct ObjectContext {
    ObjectContext(Object*);
    ~ObjectContext();

  private:
    Object* a1;
    Objectdata* a2;
    int a4;
    Symlist* a5;
};

struct OcJump {
    static bool execute(Inst* p);
    static bool execute(const char*, Object* ob = NULL);
    static void* fpycall(void* (*) (void*, void*), void*, void*);
    static void execute_throw_on_exception(Symbol* sym, int narg);
    static void execute_throw_on_exception(Object* obj, Symbol* sym, int narg);
    static Object* newobj_throw_on_exception(Symbol* sym, int narg);
};
