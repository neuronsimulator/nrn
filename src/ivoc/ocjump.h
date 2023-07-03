#pragma once
union Inst;
struct Object;
union Objectdata;
struct Symlist;

/** @brief How many NEURON try { ... } catch(...) { ... } blocks are in the call stack.
 *
 *  Errors inside NEURON are triggered using hoc_execerror, which ultimately
 *  throws an exception. To replicate the old logic, we sometimes need to insert
 *  a try/catch block *only if* there is no try/catch block less deeply nested
 *  on the call stack. This global variable tracks how many such blocks are
 *  currently present on the stack.
 */
extern int nrn_try_catch_nest_depth;

/** @brief Helper type for incrementing/decrementing nrn_try_catch_nest_depth.
 */
struct try_catch_depth_increment {
    try_catch_depth_increment() {
        ++nrn_try_catch_nest_depth;
    }
    ~try_catch_depth_increment() {
        --nrn_try_catch_nest_depth;
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
};
