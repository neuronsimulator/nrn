#pragma once
#include "nrnfilewrap.h"
union Datum;
union Inst;
struct OcJumpImpl;
struct Object;
union Objectdata;
struct Symlist;
namespace nrn::oc {
struct frame;
}

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
    bool execute(Inst* p);
    bool execute(const char*, Object* ob = NULL);
    void* fpycall(void* (*) (void*, void*), void*, void*);

  private:
    void begin();
    void restore();
    void finish();

    // hoc_oop
    Object* o1{};
    Objectdata* o2{};
    int o4{};
    Symlist* o5{};

    // code
    Inst* c1{};
    Inst* c2{};
    Datum* c3{};
    nrn::oc::frame* c4{};
    int c5{};
    int c6{};
    Inst* c7{};
    nrn::oc::frame* c8{};
    Datum* c9{};
    Symlist* c10{};
    Inst* c11{};
    int c12{};

    // input_info
    const char* i1{};
    int i2{};
    int i3{};
    NrnFILEWrap* i4{};

    // cabcode
    int cc1{};
    int cc2{};
};
