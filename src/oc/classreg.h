#pragma once

#include <hocdec.h>
#include <hoc_membf.h>

void class2oc(const char*,
              void* (*cons)(Object*),
              void (*destruct)(void*),
              Member_func*,
              Member_ret_obj_func*,
              Member_ret_str_func*);
