#include <../../nrnconf.h>

#if HAVE_IV
#include <InterViews/window.h>
#include "ivoc.h"
#include "scenevie.h"
#include "utility.h"
#endif

#include <ocnotify.h>
#include <stdio.h>
#include <stdlib.h>
#include "objcmd.h"
#include "oc2iv.h"

extern Object* hoc_thisobject;
int (*nrnpy_hoccommand_exec)(Object*);
int (*nrnpy_hoccommand_exec_strret)(Object*, char*, int);
void (*nrnpy_cmdtool)(Object*, int type, double x, double y, int kd);
double (*nrnpy_func_call)(Object*, int, int*);

HocCommand::HocCommand(const char* cmd) {
	init(cmd, hoc_thisobject);
}
HocCommand::HocCommand(const char* cmd, Object* obj) {
	init(cmd, obj);
}

HocCommand::HocCommand(Object* pobj) {
	// must wrap a PyObject method or tuple of (method, arg1, ...)
	// hold a reference to the wrapped PyObject
	if (strcmp(pobj->ctemplate->sym->name, "PythonObject") != 0) {
		hoc_execerror(hoc_object_name(pobj), "not a PythonObject");
	}
	po_ = pobj;
	hoc_obj_ref(po_);
	s_ = NULL;
	obj_ = NULL;
}

void HocCommand::init(const char* cmd, Object* obj) {
	s_ = new CopyString(cmd);
	obj_ = obj;
	po_ = NULL;
	if (obj_) {
		nrn_notify_when_void_freed((void*)obj_, this);
	}
}

void HocCommand::update(Observable*) { // obj_ has been freed
	obj_ = NULL;
	delete s_;
	s_ = new CopyString("");
}

HocCommand::~HocCommand() {
	if (obj_) {
		nrn_notify_pointer_disconnect(this);
	}
	if (s_) {
		delete s_;
	}
	if (po_) {
		hoc_obj_unref(po_);
	}
}

void HocCommand::help() {
#if HAVE_IV
	char buf[200];
	if (obj_) {
		sprintf(buf,"%s %s",
			s_->string(),
			obj_->ctemplate->sym->name
		);
	}else{
		sprintf(buf, "%s", s_->string());
	}
	Oc::help(buf);
#endif
}

const char* ccc = "PythonObject";
const char* HocCommand::name() {
	if (po_ == NULL) {
		return s_->string();
	}else{
		return ccc;
	}
}

void HocCommand::audit() {
	if (!s_) {
		return;
	}
	char buf[256];
	if (obj_) {
		sprintf(buf, "// execute(\"%s\", %p)\n", name(), obj_);
	}else{
		sprintf(buf, "{%s}\n", name());
	}
	hoc_audit_command(buf);
}

int HocCommand::execute(bool notify) {
	int err;
	if (po_) {
		assert(nrnpy_hoccommand_exec);
		err = (*nrnpy_hoccommand_exec)(po_);
	}else{
		if (!s_) {
			return 0;
		}
		char buf[256];
		sprintf(buf, "{%s}\n", s_->string());
		err = hoc_obj_run(buf, obj_);
	}
#if HAVE_IV
	if (notify) {
		Oc oc;
		oc.notify();
	}
#endif
	return err;
}
int HocCommand::exec_strret(char* buf, int size, bool notify) {
	assert (po_)
	int err = (*nrnpy_hoccommand_exec_strret)(po_, buf, size);
#if HAVE_IV
	if (notify) {
		Oc oc;
		oc.notify();
	}
#endif
	return err;
}
int HocCommand::execute(const char* s, bool notify) {
	assert(po_ == NULL);
	char buf[256];
	sprintf(buf, "{%s}\n", s);
	int err = hoc_obj_run(buf, obj_);
#if HAVE_IV
	if (notify) {
		Oc oc;
		oc.notify();
	}
#endif
	return err;
}

double HocCommand::func_call(int narg, int* perr) {
	if (po_) {
		if (nrnpy_func_call) {
			return (*nrnpy_func_call)(po_, narg, perr);
	  	}
		*perr = 1;
        	return 0.0;
	}
	Symbol* s = NULL;
	if (obj_ && obj_->ctemplate) {
		s = hoc_table_lookup(name(), obj_->ctemplate->symtable);
	}
	if (!s) {
		s = hoc_lookup(name());
	}
	if (!s) {
		hoc_execerror(name(), "is not a symbol in HocCommand::func_call");
	}
	return hoc_call_objfunc(s, narg, obj_);
}

#if HAVE_IV // to end of file

HocCommandAction::HocCommandAction(HocCommand* hc) {
	hc_ = hc;
}

HocCommandAction::~HocCommandAction() {delete hc_;}

void HocCommandAction::execute() {
	hc_->execute();
}

HocCommandTool::HocCommandTool(HocCommand* hc) : Rubberband() {
	hc_ = hc;
}

HocCommandTool::~HocCommandTool() {
	delete hc_;
}

bool HocCommandTool::event(Event& e) {
	char buf[256];
	Coord x, y;
	int kd;
#ifdef WIN32
	if (e.type() != Event::down && e.type() != Event::up && e.window()->canvas()->any_damage()) {
		return true;
	}
#endif
	if (e.type() == Event::down) {
		handle_old_focus();
		Resource::ref(this);
		e.grab(this);
#ifdef WIN32
		e.window()->grab_pointer();
#endif
	}
	kd = e.control_is_down() + e.shift_is_down()*2 + e.meta_is_down()*4;
//	transformer().inverse_transform(e.pointer_x(), e.pointer_y(), x, y);
//	the hoc callback may change the size of the view
	const Transformer& t = XYView::current_pick_view()->s2o();
	t.transform(e.pointer_x(), e.pointer_y(), x, y);
//printf("%g %g %g %g\n", e.pointer_x(), e.pointer_y(), x, y);
	if (e.type() == Event::up) {
		e.ungrab(this);
#ifdef WIN32
		e.window()->ungrab_pointer();
#endif
	}
	if (hc_->pyobject()) {
		(*nrnpy_cmdtool)(hc_->pyobject(), e.type(), x, y, kd);
		Oc oc;
		oc.notify();
	}else{
		sprintf(buf, "%s(%d, %g, %g, %d)", hc_->name(), e.type(), x, y, kd);
		hc_->execute(buf, true);
	}
	if (e.type() == Event::up) {
		Resource::unref(this);
	}
	return true;
}
#endif
