#pragma once
#include <memory>
union Inst;
class OcJumpImpl;
struct Symlist;
struct Object;
union Objectdata;

class ObjectContext {
  public:
    ObjectContext(Object*);
    virtual ~ObjectContext();
    void restore();

  private:
    Object* a1;
    Objectdata* a2;
    int a4;
    Symlist* a5;
    bool restored_;
};

struct OcJump {
    OcJump();
    virtual ~OcJump();
    bool execute(Inst* p);
    bool execute(const char*, Object* ob = NULL);
    void* fpycall(void* (*) (void*, void*), void*, void*);
  private:
    std::unique_ptr<OcJumpImpl> impl_;
};
