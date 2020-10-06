#include <../../nrnconf.h>

#if !HAVE_IV // to end of file

// things we DO NOT want

#include "hocdec.h"

extern void hoc_ret();
extern void hoc_pushx(double);
extern void nrn_shape_update();
extern Object** (*nrnpy_gui_helper_)(const char* name, Object* obj);
extern Object** (*nrnpy_gui_helper3_)(const char* name, Object* obj, int handle_strptr);
extern double (*nrnpy_object_to_double_)(Object*);

void ivoc_help(const char*){}
void ivoc_cleanup() {}
int hoc_readcheckpoint(char*){ return 0; }

void hoc_notify_iv() {
    nrn_shape_update();
    hoc_ret();
    hoc_pushx(0.);
}
void hoc_xpvalue() {
    if (nrnpy_gui_helper_) {
		nrnpy_gui_helper_("xpvalue", NULL);
	}
    hoc_ret();
    hoc_pushx(0.);
}
void hoc_xlabel() {
    if (nrnpy_gui_helper_) {
		nrnpy_gui_helper_("xlabel", NULL);
	}
    hoc_ret();
    hoc_pushx(0.);
}
void hoc_xbutton() {
    if (nrnpy_gui_helper_) {
		nrnpy_gui_helper_("xbutton", NULL);
	}
    hoc_ret();
    hoc_pushx(0.);
}
void hoc_xcheckbox() {
    if (nrnpy_gui_helper_) {
		nrnpy_gui_helper_("xcheckbox", NULL);
	}
    hoc_ret();
    hoc_pushx(0.);
}
void hoc_xstatebutton() {
    if (nrnpy_gui_helper_) {
		nrnpy_gui_helper_("xstatebutton", NULL);
	}
    hoc_ret();
    hoc_pushx(0.);
}
void hoc_xmenu() {
    if (nrnpy_gui_helper_) {
		nrnpy_gui_helper_("xmenu", NULL);
	}
    hoc_ret();
    hoc_pushx(0.);
}
void hoc_xvalue() {
    if (nrnpy_gui_helper_) {
		nrnpy_gui_helper_("xvalue", NULL);
	}
    hoc_ret();
    hoc_pushx(0.);
}
void hoc_xpanel() {
    if (nrnpy_gui_helper_) {
		nrnpy_gui_helper_("xpanel", NULL);
	}
    hoc_ret();
    hoc_pushx(0.);
}
void hoc_xradiobutton() {
    if (nrnpy_gui_helper_) {
		nrnpy_gui_helper_("xradiobutton", NULL);
	}
    hoc_ret();
    hoc_pushx(0.);
}
void hoc_xfixedvalue() {
    if (nrnpy_gui_helper_) {
		nrnpy_gui_helper_("xfixedvalue", NULL);
	}
    hoc_ret();
    hoc_pushx(0.);
}
void hoc_xvarlabel() {
    if (nrnpy_gui_helper3_) {
		nrnpy_gui_helper3_("xvarlabel", NULL, 1);
	}
    hoc_ret();
    hoc_pushx(0.);
}
void hoc_xslider() {
    if (nrnpy_gui_helper_) {
		nrnpy_gui_helper_("xslider", NULL);
	}
    hoc_ret();
    hoc_pushx(0.);
}
void hoc_boolean_dialog() {
    if (nrnpy_gui_helper_) {
		Object** const result = nrnpy_gui_helper_("boolean_dialog", NULL);
        if (result) {
            hoc_ret();
            hoc_pushx(nrnpy_object_to_double_(*result));
            return;
        }
	}
    hoc_ret();
    hoc_pushx(0.);
}
void hoc_continue_dialog() {
    if (nrnpy_gui_helper_) {
		nrnpy_gui_helper_("continue_dialog", NULL);
	}
    hoc_ret();
    hoc_pushx(0.);
}
void hoc_string_dialog() {
    // TODO: needs to work with strrefs so can actually change the string
    if (nrnpy_gui_helper_) {
		Object** const result = nrnpy_gui_helper_("string_dialog", NULL);
        if (result) {
            hoc_ret();
            hoc_pushx(nrnpy_object_to_double_(*result));
        }
	}
    hoc_ret();
    hoc_pushx(0.);
}
void hoc_checkpoint() {
    // not redirecting checkpoint because not a GUI function
    /*if (nrnpy_gui_helper_) {
		nrnpy_gui_helper_("checkpoint", NULL);
	} */
    hoc_ret();
    hoc_pushx(0.);
}
void hoc_pwman_place() {
    if (nrnpy_gui_helper_) {
		nrnpy_gui_helper_("pwman_place", NULL);
	}
    hoc_ret();
    hoc_pushx(0.);
}
void hoc_save_session() {
    if (nrnpy_gui_helper_) {
		nrnpy_gui_helper_("save_session", NULL);
	}
    hoc_ret();
    hoc_pushx(0.);
}
void hoc_print_session() {
    if (nrnpy_gui_helper_) {
		nrnpy_gui_helper_("print_session", NULL);
	}
    hoc_ret();
    hoc_pushx(0.);
}
void ivoc_style() {
    if (nrnpy_gui_helper_) {
		nrnpy_gui_helper_("ivoc_style", NULL);
	}
    hoc_ret();
    hoc_pushx(0.);
}
#endif
