#pragma once
#include <memory>
union Inst;
class OcJumpImpl;
struct Symlist;
struct Object;
union Objectdata;

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
    OcJump();
    virtual ~OcJump();
    bool execute(Inst* p);
    bool execute(const char*, Object* ob = NULL);
    void* fpycall(void* (*) (void*, void*), void*, void*);

  private:
    // https://en.cppreference.com/w/cpp/language/pimpl
    std::unique_ptr<OcJumpImpl> impl_;
};
