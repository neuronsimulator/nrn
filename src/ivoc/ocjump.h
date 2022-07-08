#pragma once
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

class OcJump {
  public:
    OcJump();
    virtual ~OcJump();
    bool execute(Inst* p);
    bool execute(const char*, Object* ob = NULL);
    void* fpycall(void* (*) (void*, void*), void*, void*);
    static void save_context(ObjectContext*);
    static void restore_context(ObjectContext*);

  private:
    OcJumpImpl* impl_;
};
