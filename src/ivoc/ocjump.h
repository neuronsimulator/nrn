#pragma once
union Inst;
struct Object;
union Objectdata;
struct Symlist;

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
