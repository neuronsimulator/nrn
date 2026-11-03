#pragma once

#include "hocdec.h"

typedef struct Psym {
    Symbol* sym;
    Arrayinfo* arayinfo;
    int nsub;
    int sub[1];
} Psym;

Psym* hoc_getsym(const char*);
double hoc_getsymval(Psym*);
void hoc_assignsym(Psym* p, double val);
void hoc_execstr(const char* cp);
