#pragma once

#include <map>

class Object;

using MemberPtr = double(void*);
using MemberRetObjPtr = Object**(void*);
using MemberRetStrPtr = const char**(void*);

void class2oc(const char* name,
              void* (*cons)(Object*),
              void (*destruct)(void*),
              std::map<const char*, MemberPtr> members,
              int (*checkpoint)(void**),
              std::map<const char*, MemberRetObjPtr> mobjret,
              std::map<const char*, MemberRetStrPtr> strret);
