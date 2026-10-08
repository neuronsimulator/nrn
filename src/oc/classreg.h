#pragma once

struct Object;

using ctor_f = void*(Object*);
using dtor_f = void(void*);

struct Member_func {
    const char* name;
    double (*member)(void*);
};

struct Member_ret_obj_func {
    const char* name;
    struct Object** (*member)(void*);
};

struct Member_ret_str_func {
    const char* name;
    const char** (*member)(void*);
};
void class2oc(const char*,
              ctor_f* cons,
              dtor_f* destruct,
              Member_func*,
              Member_ret_obj_func*,
              Member_ret_str_func*);
