#pragma once

struct Object;

typedef struct Member_func {
    const char* name;
    double (*member)(void*);
} Member_func;

typedef struct Member_ret_obj_func {
    const char* name;
    struct Object** (*member)(void*);
} Member_ret_obj_func;

typedef struct Member_ret_str_func {
    const char* name;
    const char** (*member)(void*);
} Member_ret_str_func;
