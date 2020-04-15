#ifndef gui_redirect_h
#define gui_redirect_h

#define TRY_GUI_REDIRECT_OBJ(name, obj) {\
    Object** ngh_result;\
    if (nrnpy_gui_helper_) {\
        ngh_result = nrnpy_gui_helper_(name, (Object*) obj);\
        if (ngh_result) {\
            return (void*) *ngh_result;\
        }\
    }\
}

#define TRY_GUI_REDIRECT_METHOD_ACTUAL_DOUBLE(name, sym, v) {\
	Object** guiredirect_result = NULL; \
	if (nrnpy_gui_helper_) {\
		Object* obj = (Object*) malloc(sizeof(Object));\
		obj->ctemplate = sym->u.ctemplate;\
		obj->refcount = 1;\
		obj->index = -1;\
		obj->u.this_pointer = v;\
		guiredirect_result = nrnpy_gui_helper_(name, obj);\
        /*free(obj);*/ \
        if (guiredirect_result) { \
            return(nrnpy_object_to_double_(*guiredirect_result)); \
        } \
	}\
}

#define TRY_GUI_REDIRECT_METHOD_ACTUAL_OBJ(name, sym, v) {\
	Object** guiredirect_result = NULL; \
	if (nrnpy_gui_helper_) {\
		Object* obj = (Object*) malloc(sizeof(Object));\
		obj->ctemplate = sym->u.ctemplate;\
		obj->refcount = 1;\
		obj->index = -1;\
		obj->u.this_pointer = v;\
		guiredirect_result = nrnpy_gui_helper_(name, obj);\
        /*free(obj);*/ \
        if (guiredirect_result) { \
            return(guiredirect_result); \
        } \
	}\
}

#define TRY_GUI_REDIRECT_NO_RETURN(name, obj) {\
    Object** ngh_result;\
    if (nrnpy_gui_helper_) {\
        ngh_result = nrnpy_gui_helper_(name, (Object*) obj);\
        if (ngh_result) {\
            return;\
        }\
    }\
}

#define TRY_GUI_REDIRECT_DOUBLE(name, obj) {\
    Object** ngh_result;\
    if (nrnpy_gui_helper_) {\
        ngh_result = nrnpy_gui_helper_(name, (Object*) obj);\
        if (ngh_result) {\
			hoc_ret();\
			hoc_pushx(nrnpy_object_to_double_(*ngh_result));\
            return;\
        }\
    }\
}

#define TRY_GUI_REDIRECT_ACTUAL_DOUBLE(name, obj) {\
    Object** ngh_result;\
    if (nrnpy_gui_helper_) {\
        ngh_result = nrnpy_gui_helper_(name, (Object*) obj);\
        if (ngh_result) {\
			return(nrnpy_object_to_double_(*ngh_result));\
        }\
    }\
}

#define TRY_GUI_REDIRECT_ACTUAL_STR(name, obj) {\
    char** ngh_result;\
    if (nrnpy_gui_helper_) {\
        ngh_result = nrnpy_gui_helper3_str_(name, (Object*) obj, 0);\
        if (ngh_result) {\
			return((const char**) ngh_result);\
        }\
    }\
}

#define TRY_GUI_REDIRECT_ACTUAL_OBJ(name, obj) {\
    Object** ngh_result;\
    if (nrnpy_gui_helper_) {\
        ngh_result = nrnpy_gui_helper_(name, (Object*) obj);\
        if (ngh_result) {\
            return ngh_result;\
        }\
    }\
}

#define TRY_GUI_REDIRECT_DOUBLE_SEND_STRREF(name, obj) {\
    Object** ngh_result;\
    if (nrnpy_gui_helper_) {\
        ngh_result = nrnpy_gui_helper3_(name, (Object*) obj, 1);\
        if (ngh_result) {\
			hoc_ret();\
			hoc_pushx(nrnpy_object_to_double_(*ngh_result));\
            return;\
        }\
    }\
}

#endif
