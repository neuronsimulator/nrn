#include <nrnmusic.h>
#include <music/index_map_factory.hh>

extern "C" {
extern int nrnmusic;
extern MPI_Comm nrnmusic_comm;

void nrnmusic_init(int*, char***);
void nrnmusic_terminate();

void nrnmusic_injectlist(void* vp, double tt);
void nrnmusic_inject(void* port, int gindex, double tt);
void nrnmusic_spikehandle(void* vport, double tt, int gindex);

struct PyObject;
extern Object* nrnpy_po2ho(PyObject*);
extern PyObject* nrnpy_ho2po(Object*);
extern Object* hoc_new_object(Symbol*, void*);
}

MUSIC::Setup* nrnmusic_setup;
MUSIC::Runtime* nrnmusic_runtime;

class NrnMusicEventHandler : public MUSIC::EventHandlerLocalIndex {
 public:
  void operator() (double t, MUSIC::LocalIndex id);
  void filltable(NRNMUSIC::EventInputPort*);
  PreSyn** table;
};

class MusicPortPair {
public:
	MusicPortPair(void* port, int gindex, void* p);
	virtual ~MusicPortPair();
	void clear(); // avoid recursion on delete by first calling clear
	void* port_;
	int gindex_;
	MusicPortPair* next_;
};
MusicPortPair::MusicPortPair(void* port, int gindex, void* vp) {
	port_ = port;
	gindex_ = gindex;
	next_ = (MusicPortPair*)vp;
}
MusicPortPair::~MusicPortPair() {
	if (next_) { delete next_; }
}
void MusicPortPair::clear() {
	MusicPortPair* i, *j;
	for (i=next_; i; i = j) {
		j = i->next_;
		i->next_ = 0;
		delete i;
	}
}

class NetParMusicEvent: public DiscreteEvent {
public:
	NetParMusicEvent();
	virtual ~NetParMusicEvent();
	virtual void send(double, NetCvode*, NrnThread*);
	virtual void deliver(double, NetCvode*, NrnThread*);
	virtual int type() { return 100; }
};
static NetParMusicEvent* npme;

#include <OS/table.h>
declareTable(PortTable, void*, int) // used as set
implementTable(PortTable, void*, int)
static PortTable* music_input_ports;
static PortTable* music_output_ports;

declareTable(Gi2PreSynTable, int, PreSyn*)
implementTable(Gi2PreSynTable, int, PreSyn*)

NetParMusicEvent::NetParMusicEvent() {
}
NetParMusicEvent::~NetParMusicEvent() {
}
void NetParMusicEvent::send(double t, NetCvode* nc, NrnThread* nt) {
	nc->event(t + usable_mindelay_, this, nt);
}
void NetParMusicEvent::deliver(double t, NetCvode* nc, NrnThread* nt) {
	nrnmusic_runtime->tick();
	send(t, nc, nt);
}

void alloc_music_space() {
	if (music_input_ports) { return; }
	music_input_ports = new PortTable(64);
	music_output_ports = new PortTable(64);
}

void nrnmusic_injectlist(void* vp, double tt) {
	MusicPortPair* mpp;
	for (mpp = (MusicPortPair*)vp; mpp; mpp = mpp->next_) {
		nrnmusic_inject(mpp->port_, mpp->gindex_, tt);
	}
}

void nrnmusic_inject(void* vport, int gi, double tt) {
	((MUSIC::EventOutputPort*)vport)->
	  insertEvent(tt, (MUSIC::GlobalIndex)gi);
}

void NrnMusicEventHandler::filltable(NRNMUSIC::EventInputPort* port) {
}

void NrnMusicEventHandler::operator () (double t, MUSIC::LocalIndex id) {
	PreSyn* ps = table[id];
	ps->send(t, net_cvode_instance, nrn_threads);
}

void NRNMUSIC::EventOutputPort::gid2index(int gid, int gi) {
	// analogous to pc.cell
	// except pc.cell(gid, nc) has already been called and this
	// will add this to the PreSyn.music_out_ list.
	alloc_music_space();
	PreSyn* ps;
	if (!gid2out_->find(gid, ps)) {
		return;
	}	
	ps->music_port_ = new MusicPortPair((void*)this, gi, ps->music_port_);

	// to create an IndexList for a later map, need to
	// save the indices used for each port
	int i = 0;
	if (!music_output_ports->find(i, (void*)this)) {
		music_output_ports->insert((void*)this, i);
		gi_table = new Gi2PreSynTable(1024);
	}
	PreSyn* ps2;
	assert(!gi_table->find(ps2, gi));
	gi_table->insert(gi, ps);
}

NRNMUSIC::EventInputPort::EventInputPort(MUSIC::Setup* s, std::string id)
  : MUSIC::EventInputPort(s, id) {
	gi_table = new Gi2PreSynTable(1024);	
}

PyObject* NRNMUSIC::EventInputPort::index2target(int gi, PyObject* ptarget) {
	// analogous to pc.gid_connect
	Object* target = nrnpy_po2ho(ptarget);
	if (!is_point_process(target)) {
		hoc_execerror("target arg must be a Point_process", 0);
	}
	alloc_music_space();
	PreSyn* ps;
	int i = 0;
	if (!music_input_ports->find(i, (void*)this)) {
		music_input_ports->insert((void*)this, i);
	}
	assert (!gi_table->find(ps, gi));
	ps = new PreSyn(nil, nil, nil);
	net_cvode_instance->psl_append(ps);
	gi_table->insert(gi, ps);
	ps->gid_ = -2;
	ps->output_index_ = -2;

	NetCon* nc = new NetCon(ps, target);
	Object* o = hoc_new_object(netcon_sym_, nc);
	nc->obj_ = o;
	PyObject* po = nrnpy_ho2po(o);
	return po;
}

void nrnmusic_init(int* pargc, char*** pargv) {
	int i;
	int& argc = *pargc;
	char**& argv = *pargv;
	if (strlen(argv[0]) >= 5 && 
	  strcmp(argv[0] + strlen(argv[0]) - 5, "music") == 0) {
		nrnmusic = 1;
	}
	for (i=0; i < argc; ++i) {
		if (strcmp(argv[i], "-music") == 0) {
			nrnmusic = 1;
		}
	}
	if (nrnmusic) {
		nrnmusic_setup = new MUSIC::Setup(argc, argv);
		nrnmusic_comm = nrnmusic_setup->communicator();
	}
}

void nrnmusic_terminate() {
	if (!nrnmusic_runtime) {
		nrnmusic_runtime = new MUSIC::Runtime(nrnmusic_setup, 1);
	}
	delete nrnmusic_runtime;
}

// Called from nrn_spike_exchange_init so usable_mindelay is ready to use
// For now, can only be called once.
static void nrnmusic_runtime_phase() {
	static int called = 0;
	assert(!called);
	called = 1;

	// call map on all the input ports
	for (TableIterator(PortTable) i(*music_input_ports); i.more(); i.next()) {
		NRNMUSIC::EventInputPort* eip = (NRNMUSIC::EventInputPort*)i.cur_key();
		NrnMusicEventHandler* eh = new NrnMusicEventHandler();
		Gi2PreSynTable* pst = eip->gi_table;
		MUSIC::IndexMapFactory indices;
		int li = 0;
		//iterate over pst and create indices
		for (TableIterator(Gi2PreSynTable) j(*pst); j.more(); j.next()) {
			int gi = j.cur_key();
			indices.add(gi, gi + 1, li);
			++li;
		}
		eip->map(&indices, eh, usable_mindelay_);
		delete eip->gi_table;
	}
	delete music_input_ports;

	// call map on all the output ports
	for (TableIterator(PortTable) i(*music_output_ports); i.more(); i.next()) {
		NRNMUSIC::EventOutputPort* eop = (NRNMUSIC::EventOutputPort*)i.cur_key();
		Gi2PreSynTable* pst = eop->gi_table;
		MUSIC::IndexMapFactory indices;
		int li = 0;
		//iterate over pst and create indices
		for (TableIterator(Gi2PreSynTable) j(*pst); j.more(); j.next()) {
			int gi = j.cur_key();
			indices.add(gi, gi + 1, li);
			++li;
		}
		eop->map(&indices, MUSIC::Index::GLOBAL);
		delete eop->gi_table;
	}
	delete music_output_ports;

	//switch to the runtime phase
	nrnmusic_runtime = new MUSIC::Runtime(nrnmusic_setup, usable_mindelay_);
	npme = new NetParMusicEvent();
	npme->send(0, net_cvode_instance, nrn_threads);
}
