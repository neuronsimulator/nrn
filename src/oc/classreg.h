#ifndef classreg_h
#define classreg_h
#include <stdio.h>

#include <oc/hocdec.h>
#include <oc/hoc_membf.h>


extern void class2oc(const char*,
                     void* (*cons)(Object*),
                     void (*destruct)(void*),
                     Member_func*,
                     int (*checkpoint)(void**),
                     Member_ret_obj_func*,
                     Member_ret_str_func*);


#endif
