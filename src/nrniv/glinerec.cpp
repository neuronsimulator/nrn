#include <../../nrnconf.h>
// we want to plot correctly during the local step method when
// cvode.solve(tstop) is invoked. However, not all GraphLine  are time graphs,
// so we assume that the time graphs will be added during stdinit
// during iteration over the graph lists of the stdrun implementation which
// calls Graph.simgraph(). Constantly updating during stdinit
// overcomes the problem of new addexpr, addval, and changed locations of
// point processes.
// Note: cvode.simgraph_remove() deletes all the GLineRecord instances.

extern "C" {
#include "hocparse.h"
#include "code.h"
#undef begin
#undef add
}

#include <OS/list.h>
#include "glinerec.h"
#include "netcvode.h"
#include "cvodeobj.h"



#if HAVE_IV // to end of file
#include "graph.h"

extern "C" {
extern NetCvode* net_cvode_instance;
};

class GLineRecordList;

declarePtrList(GLineRecordList, GLineRecord)
implementPtrList(GLineRecordList, GLineRecord)
static GLineRecordList* grl;

// Since GraphLine is not an observable, its destructor calls this.
// So ivoc will work, a stub is placed in ivoc/datapath.cpp
void graphLineRecDeleted(GraphLine* gl) {
	if (!grl) { return; }
	int i, cnt = grl->count();
	for (i = 0; i < cnt; ++i) {
		GLineRecord* r = grl->item(i);
		if (r->uses(gl)) {
			delete r;
			return;
		}
	}
}

void NetCvode::simgraph_remove() {
	if (!grl) { return; }
	while (grl->count()) {
		delete grl->item(grl->count()-1);
	}
}
	
void Graph::simgraph() {
	int i, cnt;
	if (!grl) { grl = new GLineRecordList(); }
	cnt = line_list_.count();
	for (i = 0; i < cnt; ++i) {
		GraphLine* gl = line_list_.item(i);
		PlayRecord* pr = net_cvode_instance->playrec_uses(gl);
		if (pr) {
			delete pr;
		}
		GLineRecord* r = new GLineRecord(gl);
		grl->append(r);
	}
}

GLineRecordExprInfo::GLineRecordExprInfo(const char* expr) {
  symlist_ = NULL;
  esym = hoc_parse_expr(expr, &symlist_);
  Inst* inst = esym->u.u_proc->defn.in;
  int sz = esym->u.u_proc->size;
  bool saw_t = false;
#if 1
  printf("\n%s\n", expr);
  for (int i = 0; i < sz; ++i) {
    hoc_debugzz(inst + i);
  }
#endif
  // execute the expr Inst by Inst but when rangepoint or
  // rangevareval are seen, execute a series of stack machine instructions
  // that give us the pointer to the range variable (see the implementation
  // in nrn/src/nrnoc/cabcode.c) but leaving the stack as though the
  // original instruction was executed.
  Inst* pcsav = hoc_pc;
  hoc_pc = inst;
  for (hoc_pc = inst; hoc_pc != STOP;) { // run machine analogous to hoc_execute(inst);
    double* pd = NULL;
    Symbol* sym;
    Inst* pc1 = hoc_pc++; // must be incremented before the function return.
    if (hoc_pc->pf == rangepoint) {
        hoc_pushx(0.5); // arc position
        rangevarevalpointer();
        pd = hoc_pxpop();
        hoc_pushx(*pd);
    }else if (hoc_pc->pf == rangevareval) {
        rangevarevalpointer();
        pd = hoc_pxpop();
        hoc_pushx(*pd);
    }else if (hoc_pc->pf == varpush) {
        sym = (hoc_pc + 1)->sym;
        if (strcmp(sym->name, "t") == 0) {
          saw_t = true;
        }
        hoc_varpush();
    }else{
        (*((pc1)->pf))();
        break;
    }
    if (pd) {
      pd_and_vec.push_back(std::pair<double*, IvocVect*>(pd, NULL));
    }
  }
  hoc_pc = pcsav;
}

GLineRecordExprInfo::~GLineRecordExprInfo() {
  for (GLineRecordEData::iterator it = pd_and_vec.begin(); it != pd_and_vec.end(); ++it) {
    if ((*it).second) {
      delete (*it).second;
    }
  }
  hoc_free_list(&symlist_);
}

GLineRecord::GLineRecord(GraphLine* gl) : PlayRecord(NULL){
	//shouldnt be necessary but just in case
//	printf("GLineRecord %p name=%s\n", this, gl->name());
	gl_ = gl;
	gl_->simgraph_activate(true);
//	pd_ = hoc_val_pointer(gl->name());
	v_ = NULL;
	expr_info_ = NULL;
	if (pd_ == NULL) {
		expr_info_ = new GLineRecordExprInfo(gl->name());
	}
}

GLineRecord::~GLineRecord(){
//	printf("~GLineRecord %p\n", this);
	int i;
	if (v_) {
		delete v_;
		v_ = NULL;
	}
	if (expr_info_) {
		delete expr_info_;
		expr_info_ = NULL;
	}
	for (i = grl->count()-1; i >= 0; --i) {
		if (grl->item(i) == this) {
			gl_->simgraph_activate(false);
			grl->remove(i);
			return;
		}
	}
}
void GLineRecord::record_init() {
  gl_->simgraph_init();
}
void GLineRecord::continuous(double t) {
  gl_->simgraph_continuous(t);
}
void GLineRecord::plot(int vecsz, double tstop) {
  double dt = tstop/double(vecsz-1);
  DataVec* x = (DataVec*)gl_->x_data();
  DataVec* y = (DataVec*)gl_->y_data();
  if (v_) {
    double* v = vector_vec(v_);
    for (int i=0; i < vecsz; ++i) {
      x->add(dt*i);
      y->add(v[i]);
    }
  }else if (expr_info_) {
    GLineRecordExprInfo& ef = *expr_info_;
    // transfer elements to range variables and plot expression result
    for (int i=0; i < vecsz; ++i) {
      x->add(dt*i);
      for (GLineRecordEData::iterator it = ef.pd_and_vec.begin(); it != ef.pd_and_vec.end(); ++it) {
        double* pd = (*it).first;
        *pd = (*it).second->elem(i);
      }
      double val = hoc_run_expr(ef.esym);
//      y->add(val);
    }
  }else{
    assert(0);
  }
}
#else
void NetCvode::simgraph_remove() {}
void GLineRecord::record_init() {}
void GLineRecord::continuous(double) {}
void GLineREcord::plot(int, double) {}
#endif
