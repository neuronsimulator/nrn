#define NO_PYTHON_H 1
#include <../neuronmusic/nrnmusic.h>

extern int nrnmusic;
extern MPI_Comm nrnmusic_comm;

void nrnmusic_init(int*, char***);
void nrnmusic_terminate();

void nrnmusic_injectlist(void* vp, double tt);
void nrnmusic_inject(void* port, int gindex, double tt);
void nrnmusic_spikehandle(void* vport, double tt, int gindex);

extern Object* nrnpy_po2ho(PyObject*);
extern PyObject* nrnpy_ho2po(Object*);
extern Object* hoc_new_object(Symbol*, void*);

MUSIC::Setup* nrnmusic_setup;
MUSIC::Runtime* nrnmusic_runtime;

class NrnMusicEventHandler : public MUSIC::EventHandlerLocalIndex {
 public:
  void operator() (double t, MUSIC::LocalIndex id);
  void filltable(NRNMUSIC::EventInputPort*, int);
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
	//printf("nrnmusic_inject %p %d %g\n", vport, gi, tt);
	((MUSIC::EventOutputPort*)vport)->
	  insertEvent(tt / 1000.0, // unit conversion ms -> s
		      (MUSIC::GlobalIndex)gi);
}

void NrnMusicEventHandler::filltable(NRNMUSIC::EventInputPort* port, int cnt) {
	table = new PreSyn*[cnt];
	int j = 0;
	for (TableIterator(Gi2PreSynTable) i(*port->gi_table); i.more(); i.next()) {
		PreSyn* ps = i.cur_value();
		table[j] = ps;
		++j;
	}
}

void NrnMusicEventHandler::operator () (double t, MUSIC::LocalIndex id) {
	PreSyn* ps = table[id];
	ps->send(1000.0 * t, net_cvode_instance, nrn_threads);
	//printf("event handler t=%g id=%d ps=%p\n", t, int(id), ps);
}

NRNMUSIC::EventOutputPort* NRNMUSIC::publishEventOutput (std::string id) {
  return new NRNMUSIC::EventOutputPort (nrnmusic_setup, id);
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
	nrn_assert(!gi_table->find(ps2, gi));
//printf("gid2index insert %p %d\n", this, gi);
	gi_table->insert(gi, ps);
}

NRNMUSIC::EventInputPort::EventInputPort(MUSIC::Setup* s, std::string id)
  : MUSIC::Port (s, id), MUSIC::EventInputPort(s, id) {
	gi_table = new Gi2PreSynTable(1024);	
}

NRNMUSIC::EventInputPort* NRNMUSIC::publishEventInput (std::string id) {
  return new NRNMUSIC::EventInputPort (nrnmusic_setup, id);
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
	//nrn_assert (!gi_table->find(ps, gi));
	if (!gi_table->find(ps, gi)) {
	  ps = new PreSyn(NULL, NULL, NULL);
	  net_cvode_instance->psl_append(ps);
	  gi_table->insert(gi, ps);
	}
	ps->gid_ = -2;
	ps->output_index_ = -2;

	NetCon* nc = new NetCon(ps, target);
	Object* o = hoc_new_object(netcon_sym_, nc);
	nc->obj_ = o;
	PyObject* po = nrnpy_ho2po(o);
//printf("index2target %d %s\n", gi, hoc_object_name(target));
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
	if (getenv ("_MUSIC_CONFIG_"))
	  nrnmusic = 1; // temporary kludge
	if (nrnmusic) {
		nrnmusic_setup = new MUSIC::Setup(argc, argv);
		nrnmusic_comm = nrnmusic_setup->communicator();
	}
}

void nrnmusic_terminate() {
	if (!nrnmusic_runtime) {
		nrnmusic_runtime = new MUSIC::Runtime(nrnmusic_setup, 1);
	}
	nrnmusic_runtime->finalize ();
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
		std::vector<MUSIC::GlobalIndex> gindices;
		//iterate over pst and create indices
		int cnt = 0;
		for (TableIterator(Gi2PreSynTable) j(*pst); j.more(); j.next()) {
			int gi = j.cur_key();
//printf("input port eip=%p gi=%d\n", eip, gi);
			gindices.push_back (gi);
			++cnt;
		}
		eh->filltable(eip, cnt);
		MUSIC::PermutationIndex indices (&gindices.front (),
						 gindices.size ());
		eip->map(&indices, eh, usable_mindelay_ / 1000.0);
		delete eip->gi_table;
	}
	delete music_input_ports;

	// call map on all the output ports
	for (TableIterator(PortTable) i(*music_output_ports); i.more(); i.next()) {
		NRNMUSIC::EventOutputPort* eop = (NRNMUSIC::EventOutputPort*)i.cur_key();
		Gi2PreSynTable* pst = eop->gi_table;
		std::vector<MUSIC::GlobalIndex> gindices;
		//iterate over pst and create indices
		for (TableIterator(Gi2PreSynTable) j(*pst); j.more(); j.next()) {
			int gi = j.cur_key();
//printf("output port eop=%p gi = %d\n", eop, gi);
			gindices.push_back (gi);
		}
		MUSIC::PermutationIndex indices (&gindices.front (),
						 gindices.size ());
		eop->map(&indices, MUSIC::Index::GLOBAL);
		delete eop->gi_table;
	}
	delete music_output_ports;

	//switch to the runtime phase
	//printf("usable_mindelay = %g\n", usable_mindelay_);
	nrnmusic_runtime = new MUSIC::Runtime(nrnmusic_setup,
					      usable_mindelay_ / 1000.0);
	npme = new NetParMusicEvent();
	npme->send(0, net_cvode_instance, nrn_threads);
}
