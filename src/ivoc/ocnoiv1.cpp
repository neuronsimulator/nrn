#include <../../nrnconf.h>

#if !HAVE_IV  // to end of file

// things we DO NOT want

#include "hocdec.h"
#include "nrnpy.h"

extern void hoc_ret();
extern void hoc_pushx(double);
extern "C" void nrn_shape_update();

void ivoc_help(const char*) {}
void ivoc_cleanup() {}
int hoc_readcheckpoint(char*) {
    return 0;
}

void hoc_notify_iv() {
    nrn_shape_update();
    hoc_ret();
    hoc_pushx(0.);
}
void hoc_xpvalue() {
    neuron::python::methods.try_gui_helper("xpvalue", nullptr);
    hoc_ret();
    hoc_pushx(0.);
}
void hoc_xlabel() {
    neuron::python::methods.try_gui_helper("xlabel", nullptr);
    hoc_ret();
    hoc_pushx(0.);
}
void hoc_xbutton() {
    neuron::python::methods.try_gui_helper("xbutton", nullptr);
    hoc_ret();
    hoc_pushx(0.);
}
void hoc_xcheckbox() {
    neuron::python::methods.try_gui_helper("xcheckbox", nullptr);
    hoc_ret();
    hoc_pushx(0.);
}
void hoc_xstatebutton() {
    neuron::python::methods.try_gui_helper("xstatebutton", nullptr);
    hoc_ret();
    hoc_pushx(0.);
}
void hoc_xmenu() {
    neuron::python::methods.try_gui_helper("xmenu", nullptr);
    hoc_ret();
    hoc_pushx(0.);
}
void hoc_xvalue() {
    neuron::python::methods.try_gui_helper("xvalue", nullptr);
    hoc_ret();
    hoc_pushx(0.);
}
void hoc_xpanel() {
    neuron::python::methods.try_gui_helper("xpanel", nullptr);
    hoc_ret();
    hoc_pushx(0.);
}
void hoc_xradiobutton() {
    neuron::python::methods.try_gui_helper("xradiobutton", nullptr);
    hoc_ret();
    hoc_pushx(0.);
}
void hoc_xfixedvalue() {
    neuron::python::methods.try_gui_helper("xfixedvalue", nullptr);
    hoc_ret();
    hoc_pushx(0.);
}
void hoc_xvarlabel() {
    if (neuron::python::methods.gui_helper3) {
        neuron::python::methods.gui_helper3("xvarlabel", nullptr, 1);
    }
    hoc_ret();
    hoc_pushx(0.);
}
void hoc_xslider() {
    neuron::python::methods.try_gui_helper("xslider", nullptr);
    hoc_ret();
    hoc_pushx(0.);
}
void hoc_boolean_dialog() {
    if (auto* const result = neuron::python::methods.try_gui_helper("boolean_dialog", nullptr)) {
        hoc_ret();
        hoc_pushx(neuron::python::methods.object_to_double(*result));
    } else {
        hoc_ret();
        hoc_pushx(0.);
    }
}
void hoc_continue_dialog() {
    neuron::python::methods.try_gui_helper("continue_dialog", nullptr);
    hoc_ret();
    hoc_pushx(0.);
}
void hoc_string_dialog() {
    // TODO: needs to work with strrefs so can actually change the string
    if (auto* const result = neuron::python::methods.try_gui_helper("string_dialog", nullptr);
        result) {
        hoc_ret();
        hoc_pushx(neuron::python::methods.object_to_double(*result));
    } else {
        hoc_ret();
        hoc_pushx(0.);
    }
}
void hoc_checkpoint() {
    // not redirecting checkpoint because not a GUI function
    // neuron::python::methods.try_gui_helper("checkpoint", nullptr);
    hoc_ret();
    hoc_pushx(0.);
}
void hoc_pwman_place() {
    neuron::python::methods.try_gui_helper("pwman_place", nullptr);
    hoc_ret();
    hoc_pushx(0.);
}
void hoc_save_session() {
    neuron::python::methods.try_gui_helper("save_session", nullptr);
    hoc_ret();
    hoc_pushx(0.);
}
void hoc_print_session() {
    neuron::python::methods.try_gui_helper("print_session", nullptr);
    hoc_ret();
    hoc_pushx(0.);
}
void ivoc_style() {
    neuron::python::methods.try_gui_helper("ivoc_style", nullptr);
    hoc_ret();
    hoc_pushx(0.);
}
#endif
