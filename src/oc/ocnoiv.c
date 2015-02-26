#include <../../nrnconf.h>
#include "hoc.h"
#include "ocfunc.h"

/*ARGSUSED*/
void notify_freed(void* p) {
}

/*ARGSUSED*/
void notify_freed_val_array(double* p, size_t size) {
}

/*ARGSUSED*/
void notify_pointer_freed(void* p) {
}

/*ARGSUSED*/
void ivoc_help(const char* p) {
}

void ivoc_cleanup(void) {}

void hoc_notify_iv(void) {hoc_ret(); hoc_pushx(0.);}
void nrniv_bind_thread(void) {hoc_ret(); hoc_pushx(0.);}
void hoc_xpvalue() {	hoc_ret(); hoc_pushx(0.);}
void hoc_xlabel(void) { hoc_ret(); hoc_pushx(0.);}
void hoc_xbutton(void) { hoc_ret(); hoc_pushx(0.);}
void hoc_xcheckbox(void) { hoc_ret(); hoc_pushx(0.);}
void hoc_xstatebutton(void) { hoc_ret(); hoc_pushx(0.);}
void hoc_xmenu(void) { hoc_ret(); hoc_pushx(0.);}
void hoc_xvalue(void) { hoc_ret(); hoc_pushx(0.);}
void hoc_xpanel(void) { hoc_ret(); hoc_pushx(0.);}
void hoc_xradiobutton(void) { hoc_ret(); hoc_pushx(0.);}
void hoc_xfixedvalue(void) { hoc_ret(); hoc_pushx(0.);}
void hoc_xvarlabel(void) { hoc_ret(); hoc_pushx(0.);}
void hoc_xslider(void) { hoc_ret(); hoc_pushx(0.);}
void ivoc_style(void) { hoc_ret(); hoc_pushx(0.);}
void hoc_boolean_dialog(void) { hoc_ret(); hoc_pushx(0.);}
void hoc_continue_dialog(void) { hoc_ret(); hoc_pushx(0.);}
void hoc_string_dialog(void) { hoc_ret(); hoc_pushx(0.);}
void hoc_checkpoint(void) { hoc_ret(); hoc_pushx(0.);}
void hoc_pwman_place(void) { hoc_ret(); hoc_pushx(0.);}
void hoc_save_session(void) { hoc_ret(); hoc_pushx(0.);}
void hoc_print_session(void) { hoc_ret(); hoc_pushx(0.);}

void hoc_class_registration(void) {}

void hoc_execute1(void) { /* the ivoc version returns safely even if execerror */
	hoc_exec_cmd();
}

#if !defined(SOME_IV)
/*ARGSUSED*/
int ivoc_list_look(Object* ob, Object* oblook, char* path, int depth)
{
	return 0;
}

/*ARGSUSED*/
void hoc_obj_disconnect(Object* ob) {}

/*ARGSUSED*/
void hoc_obj_notify(Object* ob) {}

/*ARGSUSED*/
void hoc_template_notify(Object* ob, int create) {}
#endif

/*ARGSUSED*/
int hoc_readcheckpoint(char* f) { return 0; }

/*ARGSUSED*/
int vector_arg_px(int i, double** p) {
	hoc_execerror("implemented in ivoc library", "vector_arg_px");
	return 0;
}
int vector_capacity(void* v) {return 0;}
void install_vector_method(const char* name, Pfrd_vp f) {}
int vector_instance_px(void* vv, double** px){return 0;}
void vector_resize(v, n) void* v; int n; {
	hoc_execerror("implemented in ivoc library", "vector_resize");
}
void vector_append(void* vv, double x) {}
void vector_delete(void* v) {}

void* vector_arg(int i) { return (void*)0;}
void* vector_new2(void* v) { return (void*)0;}
Object** vector_pobj(void* v) { return (Object**)0;}
double* vector_vec(void* v) {
	hoc_execerror("implemented in ivoc library", "vector_vec");
	return (double*)0;
}

Object* ivoc_list_item(Object* list, int item) {
	hoc_execerror("implemented in ivoc library", "ivoc_list_item");
	return (Object*)0;
}
int ivoc_list_count(list) Object* list; { return 0; }
void bbs_done(void){}

Symbol* ivoc_alias_lookup(const char* name, Object* ob) {return (Symbol*)0;}
void ivoc_free_alias(Object* ob){}

#if carbon || defined(MINGW)
void stdin_event_ready(void) {}
#endif

void nrnbbs_context_wait(void) {}
void ivoc_final_exit(void) {}
