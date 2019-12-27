#ifndef classreg_h
#define classreg_h
#include <stdio.h>

#include <hocdec.h>
#include <hoc_membf.h>

#if defined(__cplusplus)
extern "C" {
#endif

extern void class2oc(const char*,
	void* (*cons)(Object*),
	void (*destruct)(void*),
	Member_func*,
	int (*checkpoint)(void**),
	Member_ret_obj_func*,
	Member_ret_str_func*
);

#if defined(__cplusplus)
}
#endif

#endif
