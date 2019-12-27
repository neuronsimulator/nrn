#include <../../nrnconf.h>

#include <stdio.h>
#include "InterViews/resource.h"
#include "oc2iv.h"
#include "ocpointer.h"

extern "C" {
#include "parse.h"
extern Symlist* hoc_top_level_symlist;
extern Objectdata *hoc_top_level_data;
extern Object* hoc_thisobject;
extern Symlist* hoc_symlist;
}

char* Oc2IV::object_str(const char* name, Object* ob) {
	if (ob && ob->ctemplate->constructor) {
		if (is_obj_type(ob, "Pointer") && strcmp(name, "s") == 0) {
			return ((OcPointer*)(ob->u.this_pointer))->s_;
		}
	}else{
		return *object_pstr(name, ob);
	}
	return 0;
}

char** Oc2IV::object_pstr(const char* name, Object* ob) {
	Objectdata* od;
	Symlist* sl;
	if (ob) {
		if (ob->ctemplate->constructor) {
			return NULL;
		}else{
			od = ob->u.dataspace;
			sl = ob->ctemplate->symtable;
		}
	}else{
		od = hoc_top_level_data;
		sl = hoc_top_level_symlist;
	}
	Symbol* sym = hoc_table_lookup(name, sl);
	if (sym && sym->type == STRING) {
		return ::object_pstr(sym, od);
	}else{
		return 0;
	}
}

ParseTopLevel::ParseTopLevel() {
	restored_ = true;
	save();
}
ParseTopLevel::~ParseTopLevel() {
	restore();
}
void ParseTopLevel::save() {
	if (restored_ == true) {
		if (hoc_objectdata == hoc_top_level_data) {
			obdsav_ = NULL;
		}else{
			obdsav_ = hoc_objectdata;
		}
		obsav_ = hoc_thisobject;
		symsav_ = hoc_symlist;
		hoc_objectdata = hoc_top_level_data;
		hoc_thisobject = NULL;
		hoc_symlist = hoc_top_level_symlist;
		restored_ = false;
	}
}

extern "C" { extern int hoc_in_template; }

void ParseTopLevel::restore() {
	if (restored_ == false) {
		if (obdsav_ || hoc_in_template) {
			hoc_objectdata = obdsav_;
		}else{
			hoc_objectdata = hoc_top_level_data;
		}
		hoc_thisobject = obsav_;
		hoc_symlist = symsav_;
		restored_ = true;
	}
}

