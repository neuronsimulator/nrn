#include <../../nrnconf.h>
#include "hoc.h"

/*ARGSUSED*/
notify_freed(p)
	void* p;
{
}

/*ARGSUSED*/
notify_freed_val_array(p, size)
	double* p;
	int size;
{
}

/*ARGSUSED*/
notify_pointer_freed(p)
	void* p;
{
}

/*ARGSUSED*/
ivoc_help(p)
	char* p;
{
}

ivoc_cleanup() {}

hoc_notify_iv() {hoc_ret(); hoc_pushx(0.);}
hoc_xpvalue() {	hoc_ret(); hoc_pushx(0.);}
hoc_xlabel() { hoc_ret(); hoc_pushx(0.);}
hoc_xbutton() { hoc_ret(); hoc_pushx(0.);}
hoc_xcheckbox() { hoc_ret(); hoc_pushx(0.);}
hoc_xstatebutton() { hoc_ret(); hoc_pushx(0.);}
hoc_xmenu() { hoc_ret(); hoc_pushx(0.);}
hoc_xvalue() { hoc_ret(); hoc_pushx(0.);}
hoc_xpanel() { hoc_ret(); hoc_pushx(0.);}
hoc_xradiobutton() { hoc_ret(); hoc_pushx(0.);}
hoc_xfixedvalue() { hoc_ret(); hoc_pushx(0.);}
hoc_xvarlabel() { hoc_ret(); hoc_pushx(0.);}
hoc_xslider() { hoc_ret(); hoc_pushx(0.);}
ivoc_style() { hoc_ret(); hoc_pushx(0.);}
hoc_boolean_dialog() { hoc_ret(); hoc_pushx(0.);}
hoc_continue_dialog() { hoc_ret(); hoc_pushx(0.);}
hoc_string_dialog() { hoc_ret(); hoc_pushx(0.);}
hoc_checkpoint() { hoc_ret(); hoc_pushx(0.);}
hoc_pwman_place() { hoc_ret(); hoc_pushx(0.);}
hoc_save_session() { hoc_ret(); hoc_pushx(0.);}
hoc_print_session() { hoc_ret(); hoc_pushx(0.);}

class_registration() {}

hoc_execute1() { /* the ivoc version returns safely even if execerror */
	int ifarg();
	char* gargstr();
	Object** hoc_objgetarg();
	Object* ob = (Object*)0;
	if (ifarg(2)) {
		ob = *hoc_objgetarg(2);
	}
	hoc_exec_cmd(gargstr(1), ob);
	return 1;
}

#if !defined(SOME_IV)
/*ARGSUSED*/
int ivoc_list_look(ob, oblook, path, depth)
	struct Object *ob, *oblook;
	char* path;
	int depth;
{
	return 0;
}

/*ARGSUSED*/
hoc_obj_disconnect(ob) Object* ob; {}

/*ARGSUSED*/
hoc_obj_notify(ob, create) Object* ob; int create; {}

/*ARGSUSED*/
hoc_template_notify(ob, create) Object* ob; int create; {}
#endif

/*ARGSUSED*/
int hoc_readcheckpoint(f) char* f; { return 0; }

/*ARGSUSED*/
int vector_arg_px(i, p) int i; double* p; {
	hoc_execerror("implemented in ivoc library", "vector_arg_px");
}
int vector_capacity(v) void* v; {return 0;}
install_vector_method() {}
int vector_instance_px(){return 0;}
void vector_resize(v, n) void* v; int n; {
	hoc_execerror("implemented in ivoc library", "vector_resize");
}
void vector_delete(v) void* v; {}

void* vector_arg(i) int i; { return (void*)0;}
void* vector_new2(v) void* v; { return (void*)0;}
Object** vector_pobj(v) void* v; { return (Object**)0;}
double* vector_vec(v) void* v; {
	hoc_execerror("implemented in ivoc library", "vector_vec");
}

Object* ivoc_list_item(list, item) Object* list; int item; {
	hoc_execerror("implemented in ivoc library", "ivoc_list_item");
	return (Object*)0;
}
int ivoc_list_count(list) Object* list; { return 0; }
void bbs_done(){}

Symbol* ivoc_alias_lookup(name, ob) char* name; Object* ob; {return (Symbol*)0;}
void ivoc_free_alias(ob) Object* ob; {}

#if carbon
void stdin_event_ready() {}
#endif

void nrnbbs_context_wait() {}
void ivoc_final_exit() {}
