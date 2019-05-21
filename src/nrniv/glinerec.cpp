#include <../../nrnconf.h>
// we want to plot correctly during the local step method when
// cvode.solve(tstop) is invoked. However, not all GraphLine  are time graphs,
// so we assume that the time graphs will be added during stdinit
// during iteration over the graph lists of the stdrun implementation which
// calls Graph.simgraph(). Constantly updating during stdinit
// overcomes the problem of new addexpr, addval, and changed locations of
// point processes.
// Note: cvode.simgraph_remove() deletes all the GLineRecord instances.

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

GLineRecord::GLineRecord(GraphLine* gl) : PlayRecord(NULL){
	//shouldnt be necessary but just in case
//	printf("GLineRecord %p name=%s\n", this, gl->name());
	gl_ = gl;
	gl_->simgraph_activate(true);
	pd_ = hoc_val_pointer(gl->name());
	v_ = NULL;
}

GLineRecord::~GLineRecord(){
//	printf("~GLineRecord %p\n", this);
	int i;
	if (v_) {
		delete v_;
		v_ = NULL;
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
  double* v = vector_vec(v_);
  DataVec* x = (DataVec*)gl_->x_data();
  DataVec* y = (DataVec*)gl_->y_data();
  for (int i=0; i < vecsz; ++i) {
    x->add(dt*i);
    y->add(v[i]);
  }
}
#else
void NetCvode::simgraph_remove() {}
void GLineRecord::record_init() {}
void GLineRecord::continuous(double) {}
void GLineREcord::plot(int, double) {}
#endif
