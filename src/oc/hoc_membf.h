#pragma once

#include <functional>
#include <unordered_map>

// Give Symbol
#include <hocdec.h>

struct Object;

using MemberPtr = double (*)(void*);
using Member_func = std::unordered_map<const char*, std::function<double(void*)>>;
using MemberRetObjPtr = Object** (*) (void*);
using Member_ret_obj_func = std::unordered_map<const char*, MemberRetObjPtr>;
using MemberRetStrPtr = const char** (*) (void*);
using Member_ret_str_func = std::unordered_map<const char*, MemberRetStrPtr>;

void class2oc_base(const char* name,
                   void* (*cons)(Object*),
                   void (*destruct)(void*),
                   const Member_func& m,
                   int (*checkpoint)(void**),
                   const Member_ret_obj_func& mobjret,
                   const Member_ret_str_func& strret);

void class2oc(const char* name,
              void* (*cons)(Object*),
              void (*destruct)(void*),
              const Member_func& members,
              int (*checkpoint)(void**),
              const Member_ret_obj_func& mobjret,
              const Member_ret_str_func& strret);
