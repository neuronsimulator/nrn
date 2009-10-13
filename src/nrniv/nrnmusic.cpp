#include <nrnmusic.h>

extern "C" {
extern int nrnmusic;
extern MPI::Intracomm nrnmusic_comm;

void nrnmusic_init(int*, char***);
void nrnmusic_terminate();

void nrnmusic_injectlist(void* vp, double tt);
void nrnmusic_inject(void* port, int gindex, double tt);
void nrnmusic_spikehandle(void* vport, double tt, int gindex);

extern Object* nrnpy_po2ho(void*); //PyObject*
}

MUSIC::Setup* nrnmusic_setup;
MUSIC::Runtime* nrnmusic_runtime;

class NrnMusicEventHandler : public MUSIC::EventHandlerLocalIndex {
 public:
  void operator() (double t, MUSIC::LocalIndex id);
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

#include <OS/table2.h>
declareTable2(Port2PS, void*, long, PreSyn*)
implementTable2(Port2PS, void*, long, PreSyn*)
static Port2PS*  music_in_table;

void alloc_music_space() {
	if (music_in_table) { return; }
	music_in_table = new Port2PS(1024);
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

void NrnMusicEventHandler::operator () (double t, MUSIC::LocalIndex id) {
	PreSyn* ps = table[id];
	ps->send(t, net_cvode_instance, nrn_threads);
}

void NRNMUSIC::EventOutputPort::gid2index(int gid, int gi) {
	// analogous to pc.cell
	// except pc.cell(gid, nc) has already been called and this
	// will add this to the PreSyn.music_out_ list.
	PreSyn* ps;
	if (!gid2out_->find(gid, ps)) {
		return;
	}	
	ps->music_port_ = new MusicPortPair((void*)this, gi, ps->music_port_);
}

void NRNMUSIC::EventInputPort::index2netcon(int gi, PyObject* pnetcon) {
	// analogous to pc.gid_connect
	Object* onetcon = nrnpy_po2ho(pnetcon);
	if (onetcon && onetcon->ctemplate && onetcon->ctemplate->sym == netcon_sym_) {
		hoc_execerror("netcon arg must be a NetCon", 0);
	}
	alloc_music_space();
	PreSyn* ps;
	if (!music_in_table->find(ps, (void*)this, gi)) {
		ps = new PreSyn(nil, nil, nil);
		net_cvode_instance->psl_append(ps);
		music_in_table->insert((void*)this, gi, ps);
		ps->gid_ = -2;
		ps->output_index_ = -2;
	}
	NetCon* nc = (NetCon*)onetcon->u.this_pointer;
	nc->replace_src(ps);
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

static void nrnmusic_runtime_phase() {
}
