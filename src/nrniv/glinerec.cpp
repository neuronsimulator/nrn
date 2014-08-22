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
#include "nrnoc2iv.h"
#include "vrecitem.h"
#include "netcvode.h"
#include "cvodeobj.h"

#if HAVE_IV // to end of file
#include "graph.h"

extern "C" {
extern NetCvode* net_cvode_instance;
};

class GLineRecordList;

class GLineRecord : public PlayRecord {
public:
	GLineRecord(GraphLine*);
	virtual ~GLineRecord();
	virtual void install(Cvode* cv) { record_add(cv); }
	virtual void record_init() {gl_->simgraph_init();}
	virtual void continuous(double t) { gl_->simgraph_continuous(t);}
	virtual bool uses(void* v) { return (void*)gl_ == v; }

	GraphLine* gl_;
};

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
}

GLineRecord::~GLineRecord(){
//	printf("~GLineRecord %p\n", this);
	int i;
	for (i = grl->count()-1; i >= 0; --i) {
		if (grl->item(i) == this) {
			gl_->simgraph_activate(false);
			grl->remove(i);
			return;
		}
	}
}
#else
void NetCvode::simgraph_remove() {}
#endif
