#define TRY_GUI_REDIRECT_OBJ(name, obj) {\
    Object** ngh_result;\
    if (nrnpy_gui_helper_) {\
        ngh_result = nrnpy_gui_helper_(name, (Object*) obj);\
        if (ngh_result) {\
            return (void*) *ngh_result;\
        }\
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
