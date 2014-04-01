#include <../../nrnconf.h>

#if !HAVE_IV // to end of file

// things we DO NOT want

extern "C" {
#include "hocdec.h"

extern void hoc_ret();
extern void hoc_pushx(double);

void ivoc_help(const char*){}
void ivoc_cleanup() {}
int hoc_readcheckpoint(char*){ return 0; }

void hoc_notify_iv() {hoc_ret(); hoc_pushx(0.);}
void hoc_xpvalue() {	hoc_ret(); hoc_pushx(0.);}
void hoc_xlabel() { hoc_ret(); hoc_pushx(0.);}
void hoc_xbutton() { hoc_ret(); hoc_pushx(0.);}
void hoc_xcheckbox() { hoc_ret(); hoc_pushx(0.);}
void hoc_xstatebutton() { hoc_ret(); hoc_pushx(0.);}
void hoc_xmenu() { hoc_ret(); hoc_pushx(0.);}
void hoc_xvalue() { hoc_ret(); hoc_pushx(0.);}
void hoc_xpanel() { hoc_ret(); hoc_pushx(0.);}
void hoc_xradiobutton() { hoc_ret(); hoc_pushx(0.);}
void hoc_xfixedvalue() { hoc_ret(); hoc_pushx(0.);}
void hoc_xvarlabel() { hoc_ret(); hoc_pushx(0.);}
void hoc_xslider() { hoc_ret(); hoc_pushx(0.);}
void hoc_boolean_dialog() { hoc_ret(); hoc_pushx(0.);}
void hoc_continue_dialog() { hoc_ret(); hoc_pushx(0.);}
void hoc_string_dialog() { hoc_ret(); hoc_pushx(0.);}
void hoc_checkpoint() { hoc_ret(); hoc_pushx(0.);}
void hoc_pwman_place() { hoc_ret(); hoc_pushx(0.);}
void hoc_save_session() { hoc_ret(); hoc_pushx(0.);}
void hoc_print_session() { hoc_ret(); hoc_pushx(0.);}
void ivoc_style() { hoc_ret(); hoc_pushx(0.);}
}
#endif
