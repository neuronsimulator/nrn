#include <../../nrnconf.h>

/*

The goal is to be able to save state to a file and restore state when the
save and restore contexts have different numbers of processors, different
distribution of gids, and different splitting. It needs to work efficiently
in the context of many GByte file sizes and many thousands of processors.

The major assumption is that Point_process order is the same within a Node.
It is difficult to assert the correctness of this assumption unless the user
defines a global identifier for each Point_process and modifies the
BBSaveState implementation to explicitly check that parameter with
f->i(ppgid, 1).
This has been relaxed a bit to allow point processes to be marked IGNORE
which will skip them on save/restore. However, it still holds that
the order of the non-ignored point processes must be the same within a Node.
When a property is encountered the type is checked. However there is no
way to determine if two point processes of the same type are restored in
the saved order in a Node. Also note that when a point process is ignored,
that also implies that the NetCons for which it is a target are also
ignored. Finally, note that a non-ignored point process that is saved
cannot be marked IGNORE on restore. Similarly, if a point process is
marked IGNORE on save it cannot be non-ignored on restore.

The user can mark a point process IGNORE by calling the method
bbss.ignore(point_process_object)
on all the point processes to be ignored.
The internal list of ignored point processes can be cleared by calling
bbss.ignore()

Because a restore clears the event queue and because one cannot call
finitialize from hoc without vitiating the restore, Vector.play will
not work unless one calls BBSaveState.vector_play_init() after a 
restore (similarly frecord() must be called for Vector.record to work.
Note that it is necessary that Vector.play use a tvec argument with
a first element greater than or equal to the restore time.

Because of many unimplemented aspects to the general problem,
this implementation is more or less limited to BlueBrain cortical models.
There are also conceptual difficulties with the general problem since
necessary information is embodied in the Hoc programs.

Some model restrictions:
1) "Real" cells must have gids.
2) Artificial cells can have gids. If not they must be directly connected
   to just one synapse (e.g. NetStim -> NetCon -> synapse).
3) There is only one spike output port per cell and that is associated
   with a base gid.
4) NetCon.event in Hoc can be used only with NetCon's with a None source.

Note: On reading, the only things that need to exist in the file are the gids
and sections that are needed and the others are ignored. So subsets of a written
model can read the same file. However, again, all the Point_process objects
have to exist for each section that were written by the file (unless they
they are eplicitly marked IGNORE).

We keep together, all the information associated with a section which includes
Point_processes and the NetCons, and SelfEvents targeting each Point_process.
Sometimes a NetStim stimulus is used to stimulate the PointProcess.
For those cases, when the NetCon has no srcid, the source is also associated
with the Point_process.

Originally, NetCon with an ARTIFICIAL_CELL source were ignored and did
not appear in the DEList of the point process.  This allowed one to have
different numbers of NetStim with NetCon connected locally before a save
and restore.  Note that outstanding events from these NetCon on the
queue were not saved.  PreSyn with state are associated with the cell
(gid)that contains them. We required PreSyn to be associated with gids.
Although that convention had some nice properties (albeit, it involved some
extra programming complexity) it has been abandoned because in practice,
stimuli such as InhPoissonStim are complex and it is desirable that they
continue past a save/restore.

We share more or less the same code for counting, writing, and reading by
using subclasses of BBSS_IO.

The implementation is based on what was learned from a prototype
bbsavestate python module which turned out to be too slow. In particular,
the association of Point_Process and its list of NetCons and queue SelfEvents
needs to be kept in a hash map. NetCon order in the list is assumed to
be consistent for writing and reading but srcgids are checked for consistency
on reading.(note that is not a guarantee of correctness since it is possible
for two NetCons to have the same source and the same target. However the
Blue Brain project models at present have a one-to-one correspondence between
NetCon and synapse Point_process so even the idea of a list is overdoing it.)

We assume only NetCon and SelfEvent queue events need to be saved. Others
that are under user control have to be managed by the user since reading
will clear the queue (except for an initialized NetParEvent). In particular
all cvode.event need to be re-issued.  One efficiency we realize is to avoid
saving the queue with regard to NetCon events and instead save the
gid, spiketime information and re-issue the NetCon.send appropriately if not
already delivered.

On further thought, the last sentence above needs some clarification.
Because of the variation in NetCon.delay, in general some spikes
for a given PreSyn have already been delivered and some spikes are
still on the queue. Nevertheless, it is only necessary to issue
the PreSyn.send with its proper initiation time on the source machine
and then do a spike exchange. On the target machine only the
NetCon spikes with delivery time >= t need to be re-issued from the
PreSyn.send. For this reason all the global NetCon events are
resolved into a set of source PreSyn events and associated with the
cell portion containing the spike generation site. This is very similar
to the problem solved by nrn_fake_fire when helping implement PatternStim
and to extend that for use here we only needed an extra mode
(fake_out = 2) with a one line change.
On the other hand, all the SelfEvents are associated with the cell
portion containing the target synapse.

So, associated with a Section are the point processes and PreSyn, and
associated with point processes are NetCon and SelfEvent

To allow Random state to be saved for POINT_PROCESS, if a POINT_PROCESS
declares FUNCTION bbsavestate, that function is called when the
POINT_PROCESS instance is saved and restored.
Also now allow Random state to be saved for a SUFFIX (density mechanism)
if it declares FUNCTION bbsavestate. Same interface.
The API of FUNCTION bbsavestate has been modified to allow saving/restoring
several values. FUNCTION bbsavestate takes two pointers to double arrays,
xdir and xval.
The first double array, xdir, has length 1 and xdir[0] is -1.0, 0.0, or 1.0 
If xdir[0] == -1.0, then replace the xdir[0] with the proper number of elements
of xval and return 0.0.  If xdir[0] == 0.0, then save double values into
the xval array (which will be sized correctly from a previous call with
xdir[0] == -1.0). If xdir[0] == 1.0, then the saved double values are in
xval and should be restored to their original values.
The number of elements saved/restored has to be apriori known by the instance
since the size of the xval that was saved is not sent to the instance on
restore.

For example
  FUNCTION bbsavestate() {
    bbsavestate = 0
  VERBATIM
    double *xdir, *xval, *hoc_pgetarg();
    xdir = hoc_pgetarg(1);
    if (*xdir == -1.) { *xdir = 2; return 0.0; }
    xval = hoc_pgetarg(2);
    if (*xdir == 0.) { xval[0] = 20.; xval[1] = 21.;}
    if (*xdir == 1) { printf("%d %d\n", xval[0]==20.0, xval[1] == 21.0); }
  ENDVERBATIM
  }

When spike compression is used, there are only 256 time slots available
for spike exchange time within an integration interval. However, during
restore, we are sending the gid spike initiation time which can be
earlier than the current integration interval (and therefore may have
a negative slot index.
To avoid this problem, we must temporarily turn off both
spike and gid compression so that PreSyn::send steers to
nrn2ncs_outputevent(output_index_, tt) instead of nrn_outputevent(localgid,tt)
and so nrn_spike_exchange does a non-compressed exchange.
We do not need to worry about bin queueing since it is the delivery time that
is enqueueed and that is always in the future.

When bin queueing is used, a mechanism is needed to avoid the assertion
error in BinQ::enqueue (see nrncvode/sptbinq.cpp) when the enqueued event
has a delivery time earlier than the binq current time. One possibility
is to turn off bin queueing and force all events on the standard queue
to be on binq boundaries. Another possibility is for bbsavestate to
take over bin queueing in this situation.  The first possibility is
simpler but entails a slight loss of performance when dealing with the
normally binned events on the standard queue when simulation takes up again.
Let's try trapping the assertion error in BinQ::enqueue and executing a
callback to bbss_early when needed.
*/

#include <stdio.h>
#include <stdlib.h>
#include "ocfile.h"
#include "nrnoc2iv.h"
#include "classreg.h"
#include "ndatclas.h"
#include "bbsavestate.h"
#include <nrnhash.h>
#include <nrnmpiuse.h>
#include <OS/list.h>

#include "tqueue.h"
#include "netcon.h"
#include "vrecitem.h"

// on mingw, OUT became defined
#undef IN
#undef OUT
#undef CNT

extern bool nrn_use_bin_queue_;
extern void (*nrn_binq_enqueue_error_handler)(double, TQItem*);
static void bbss_early(double td, TQItem* tq);

typedef void (*ReceiveFunc)(Point_process*, double*, double);

extern "C" {
#include "membfunc.h"
extern int section_count;
extern void nrn_shape_update();
extern Section* nrn_section_exists(char* name, int index, Object* cell);
extern Section** secorder;
extern ReceiveFunc* pnt_receive;
extern NetCvode* net_cvode_instance;
extern TQueue* net_cvode_instance_event_queue(NrnThread*);
extern void clear_event_queue();
extern cTemplate** nrn_pnt_template_;
extern hoc_Item* net_cvode_instance_psl();
extern PlayRecList* net_cvode_instance_prl();
extern void nrn_netcon_event(NetCon*, double);
extern void net_send(void**, double*, Point_process*, double, double);
extern double t;
typedef void (*PFIO)(int, Object*);
extern void nrn_gidout_iter(PFIO);
extern short* nrn_is_artificial_;
extern void nrn_fake_fire(int gid, double firetime, int fake_out);
extern Object* nrn_gid2obj(int gid);
extern PreSyn* nrn_gid2presyn(int gid);
extern int nrn_gid_exists(int gid);

#if NRNMPI
extern void nrn_spike_exchange(NrnThread*);
extern void nrnmpi_barrier();
extern void nrnmpi_int_alltoallv(int*, int*, int*, int*, int*, int*);
extern void nrnmpi_dbl_alltoallv(double*, int*, int*, double*, int*, int*);
extern int nrnmpi_int_allmax(int);
extern void nrnmpi_int_allgather(int* s, int* r, int n);
extern void nrnmpi_int_allgatherv(int* s, int* r, int* n, int* dspl);
extern void nrnmpi_dbl_allgatherv(double* s, double* r, int* n, int* dspl);
#else
static void nrn_spike_exchange(NrnThread*) {}
static void nrnmpi_barrier() {}
static void nrnmpi_int_alltoallv(int* s, int* scnt, int* sdispl, int* r, int* rcnt, int* rdispl) {
  for (int i=0; i < scnt[0]; ++i) {
    r[i] = s[i];
  }
}
static void nrnmpi_dbl_alltoallv(double* s, int* scnt, int* sdispl, double* r, int* rcnt, int* rdispl) {
  for (int i=0; i < scnt[0]; ++i) {
    r[i] = s[i];
  }
}
static int nrnmpi_int_allmax(int x) { return x; }
static void nrnmpi_int_allgather(int* s, int* r, int n) {
  for (int i=0; i < n; ++i) {
    r[i] = s[i];
  }
}
static void nrnmpi_int_allgatherv(int* s, int* r, int* n, int* dspl) {
  for (int i=0; i < n[0]; ++i) {
    r[i] = s[i];
  }
}
static void nrnmpi_dbl_allgatherv(double* s, double* r, int* n, int* dspl) {
  for (int i=0; i < n[0]; ++i) {
    r[i] = s[i];
  }
}
#endif // NRNMPI

#if BGPDMA
extern int use_bgpdma_;
#endif

extern Point_process* ob2pntproc(Object*);
extern void nrn_play_init();
extern Symlist* hoc_built_in_symlist;

// turn off compression to avoid problems with presyn deliver earlier than
// restore time.
#if NRNMPI
extern bool nrn_use_compress_;
extern bool nrn_use_localgid_;
#endif
static bool use_spikecompress_;
static bool use_gidcompress_;

static int callback_mode;
static void tqcallback(const TQItem* tq, int i);
declarePtrList(TQItemList, TQItem)
implementPtrList(TQItemList, TQItem)
static TQItemList* tq_presyn_fanout;
static TQItemList* tq_removal_list;

#define QUEUECHECK 1
#if QUEUECHECK
static void bbss_queuecheck();
#endif

// API
// see save_test_bin and restore_test_bin for an example of
// the use of this following interface. Note in particular the
// use in restore_test_bin of a prior clear_event_queue() in order
// to allow bbss_buffer_counts to pass an assert during the restore
// process.


void* bbss_buffer_counts(int* len, int** gids, int** sizes, int* global_size);
// First call to return the information needed to make the other
// calls. Returns a pointer used by the other methods.
// Caller is reponsible for freeing (using free() and not delete [])
// the returned gids and sizes arrays
// when finished. The sizes array and global_size are needed for the
// caller to construct proper buffer sizes to pass to
// bbss_save_global and bbss_save for filling in. The size of these
// arrays is returned in *len.
// They are not needed for restoring
// (since the caller is passing already filled in buffers that are read
// by bbss_restore_global and bbss_restore
// The gids returned are base gids. It is the callers responsibility
// to somehow concatenate buffers with the same gid (from different hosts)
// either after save or before restore and preserve the piece count
// of the number of concatenated buffers for each base gid.
// Global_size will only be non_zero for host 0.

void bbss_save_global(void* bbss, char* buffer, int sz);
// call only on host 0 with a buffer of size equal to the
// global_size returned from the bbss_buffer_counts call on host 0
// sz is the size of the buffer (for error checking only, buffer+sz is
// out of bounds)

void bbss_restore_global(void* bbss, char* buffer, int sz);
// call on all hosts with the buffer contents returned from the call
// to bbss_save_global
// This must be called prior to any calls to bbss_restore
// sz is the size of the buffer (error checking only)
// This also does some other important restore initializations.

void bbss_save(void* bbss, int gid, char* buffer, int sz);
// Call this for each item of the gids from bbss_buffer_counts along with
// a buffer of size from the corresponding sizes array. The buffer will
// be filled in with savestate information. The gid may be the same on
// different hosts, in which case it is the callers responsibility to
// concatentate buffers at some point to allow calling of bbss_restore
// sz is the size of the buffer (error checking only)

void bbss_restore(void* bbss, int gid, int npiece, char* buffer, int sz);
// Call this for each item of the gids from bbss_buffer_counts, the
// number of buffers that were concatenated for the gid, and the
// concatenated buffer (the concatenated buffer does NOT contain npiece
// as the first value in the char* buffer pointer)
// sz is the size of the buffer (error checking only)

void bbss_save_done(void* bbss);
// At the end of the save process, call this to cleanup.
// when this call returns, bbss will be invalid.

void bbss_restore_done(void* bbss);
// At the end of the restore process, call this to do
// some extra setting up and cleanup.
// when this call returns, bbss will be invalid.

};


// 0 no debug, 1 print to stdout, 2 read/write to IO file
#define DEBUG 0
static int debug = DEBUG;
static char dbuf[1024];
#if DEBUG == 1
#define PDEBUG printf("%s\n", dbuf)
#else
#define PDEBUG f->s(dbuf, 1)
#endif

static BBSaveState* bbss;

BBSS_IO::BBSS_IO(){}

static int usebin_; // 1 if using Buffer, 0 if using TxtFile

class BBSS_Cnt : public BBSS_IO {
public:
	BBSS_Cnt();
	virtual ~BBSS_Cnt();
	virtual void i(int& j, int chk=0);
	virtual void d(int n, double& p);
	virtual void d(int n, double* p);
	virtual void s(char* cp, int chk=0);
	virtual Type type();
	int bytecnt();
	int ni, nd, ns, nl;
private:
	int bytecntbin();
	int bytecntasc();
};
BBSS_Cnt::BBSS_Cnt() { ni = nd = ns = nl = 0; }
BBSS_Cnt::~BBSS_Cnt() {}
void BBSS_Cnt::i(int& j, int chk) { ++ni; ++nl;}
void BBSS_Cnt::d(int n, double& p) { nd += n; ++nl;}
void BBSS_Cnt::d(int n, double* p) { nd += n; ++nl;}
void BBSS_Cnt::s(char* cp, int chk) { ns += strlen(cp) + 1; }
BBSS_IO::Type BBSS_Cnt::type() {return BBSS_IO::CNT;}
int BBSS_Cnt::bytecnt() { return usebin_ ? bytecntbin() : bytecntasc(); }
int BBSS_Cnt::bytecntbin() { return ni*sizeof(int) + nd*sizeof(double) + ns; }
int BBSS_Cnt::bytecntasc() { return ni*12 + nd*23 + ns + nl; }

class BBSS_TxtFileOut : public BBSS_IO {
public:
	BBSS_TxtFileOut(const char*);
	virtual ~BBSS_TxtFileOut();
	virtual void i(int& j, int chk=0);
	virtual void d(int n, double& p);
	virtual void d(int n, double* p);
	virtual void s(char* cp, int chk=0);
	virtual Type type();
	FILE* f;
};
BBSS_TxtFileOut::BBSS_TxtFileOut(const char* fname) {
	f = fopen(fname, "w");
	assert(f);
}
BBSS_TxtFileOut::~BBSS_TxtFileOut() {
	fclose(f);
}
void BBSS_TxtFileOut::i(int& j, int chk) { fprintf(f, "%12d\n", j); }
void BBSS_TxtFileOut::d(int n, double& p) { d(n, &p); }
void BBSS_TxtFileOut::d(int n, double* p){
	for (int i=0; i < n; ++i) { fprintf(f, " %22.15g", p[i]); }
	fprintf(f, "\n");
}
void BBSS_TxtFileOut::s(char* cp, int chk){fprintf(f, "%s\n", cp); }
BBSS_IO::Type BBSS_TxtFileOut::type() {return BBSS_IO::OUT;}

class BBSS_TxtFileIn : public BBSS_IO {
public:
	BBSS_TxtFileIn(const char*);
	virtual ~BBSS_TxtFileIn();
	virtual void i(int& j, int chk=0);
	virtual void d(int n, double& p) { d(n, &p); }
	virtual void d(int n, double* p);
	virtual void s(char* cp, int chk=0);
	virtual Type type() {return BBSS_IO::IN;}
	virtual void skip(int);
	FILE* f;
};
BBSS_TxtFileIn::BBSS_TxtFileIn(const char* fname) {
	f = fopen(fname, "r");
	assert(f);
}
BBSS_TxtFileIn::~BBSS_TxtFileIn() {
	fclose(f);
}
void BBSS_TxtFileIn::i(int& j, int chk) {
	int k;
	nrn_assert(fscanf(f, "%d\n", &k) == 1);
	if (chk) {
		assert (j == k);
	}
	j = k;
}
void BBSS_TxtFileIn::d(int n, double* p) {
	for (int i=0; i < n; ++i) { nrn_assert(fscanf(f, " %lf", p+i) == 1); }
	nrn_assert(fscanf(f, "\n") == 0);
}
void BBSS_TxtFileIn::s(char* cp, int chk) {
	char buf[100];
	nrn_assert(fscanf(f, "%[^\n]\n", buf)==1);
	if (chk) {
		assert(strcmp(buf, cp) == 0);
	}
	strcpy(cp, buf);
}
void BBSS_TxtFileIn::skip(int n) {
	for (int i=0; i < n; ++i) {
		fgetc(f);
	}
}

class BBSS_BufferOut : public BBSS_IO {
public:
	BBSS_BufferOut(char* buffer, int size);
	virtual ~BBSS_BufferOut();
	virtual void i(int& j, int chk=0);
	virtual void d(int n, double& p);
	virtual void d(int n, double* p);
	virtual void s(char* cp, int chk=0);
	virtual Type type();
	virtual void a(int);
	virtual void cpy(int size, char* cp);
	int sz;
	char* b;
	char* p;
};
BBSS_BufferOut::BBSS_BufferOut(char* buffer, int size) { b = buffer; p = b; sz = size; }
BBSS_BufferOut::~BBSS_BufferOut() {}
void BBSS_BufferOut::i(int& j, int chk) {cpy(sizeof(int), (char*)(&j));}
void BBSS_BufferOut::d(int n, double& d) {cpy(sizeof(double), (char*)(&d));}
void BBSS_BufferOut::d(int n, double* d) {cpy(n*sizeof(double), (char*)d);}
void BBSS_BufferOut::s(char* cp, int chk) {cpy(strlen(cp)+1, cp);}
void BBSS_BufferOut::a(int i) {	assert((p - b) + i <= sz);}
BBSS_IO::Type BBSS_BufferOut::type() {return BBSS_IO::OUT;}
void BBSS_BufferOut::cpy(int ns, char* cp){
	a(ns);
	for (int ii = 0; ii < ns; ++ii) {
		p[ii] = cp[ii];
	}
	p += ns;
}
class BBSS_BufferIn : public BBSS_BufferOut {
public:
	BBSS_BufferIn(char* buffer, int size);
	virtual ~BBSS_BufferIn();
	virtual void i(int& j, int chk=0);
	virtual void s(char* cp, int chk=0);
	virtual void skip(int n) { p += n; }
	virtual Type type();
	virtual void cpy(int size, char* cp);
};
BBSS_BufferIn::BBSS_BufferIn(char* buffer, int size)
	:BBSS_BufferOut(buffer, size) {}
BBSS_BufferIn::~BBSS_BufferIn() {}
void BBSS_BufferIn::i(int& j, int chk) {
	int k;
	cpy(sizeof(int), (char*)(&k));
	if (chk) {
		assert (j == k);
	}
	j = k;
}
void BBSS_BufferIn::s(char* cp, int chk) {
	if (chk) {
		assert(strcmp(p, cp) == 0);
	}
	cpy(strlen(p)+1, cp);
}
BBSS_IO::Type BBSS_BufferIn::type() {return BBSS_IO::IN;}
void BBSS_BufferIn::cpy(int ns, char* cp){
	a(ns);
	for (int ii = 0; ii < ns; ++ii) {
		cp[ii] = p[ii];
	}
	p += ns;
}

static void* cons(Object*) {
        BBSaveState* ss = new BBSaveState();
        return (void*)ss;
}

static void destruct(void* v) {
        BBSaveState* ss = (BBSaveState*)v;
        delete ss;
}

static double save(void* v) {
        BBSaveState* ss = (BBSaveState*)v;
	BBSS_IO* io = new BBSS_TxtFileOut(gargstr(1));
        ss->apply(io);
	delete io;
        return 1.;
}

static double restore(void* v) {
        BBSaveState* ss = (BBSaveState*)v;
	BBSS_IO* io = new BBSS_TxtFileIn(gargstr(1));
        ss->apply(io);
	delete io;
        return 1.;
}

#include <ivocvect.h>
static double save_request(void* v) {
	int* gids, *sizes;
	Vect* gidvec = vector_arg(1);
	Vect* sizevec = vector_arg(2);
        BBSaveState* ss = (BBSaveState*)v;
	int len = ss->counts(&gids, &sizes);
	gidvec->resize(len);
	sizevec->resize(len);
	for (int i = 0; i < len; ++i) {
		gidvec->elem(i) = double(gids[i]);
		sizevec->elem(i) = double(sizes[i]);
	}
	if (len) {
		free(gids);
		free(sizes);
	}
	return double(len);
}

static double save_gid(void* v) {
	printf("save_gid not implemented\n");
	return 0.;
}

static double restore_gid(void* v) {
	printf("restore_gid not implemented\n");
	return 0.;
}

static double save_test(void* v) {
	int* gids, *sizes;
	BBSaveState* ss = (BBSaveState*)v;
	usebin_ = 0;
	if (nrnmpi_myid == 0) { // save global time
		BBSS_IO* io = new BBSS_TxtFileOut("out/tmp");
		io->d(1, nrn_threads->_t);
		delete io;
	}
	int len = ss->counts(&gids, &sizes);
	for (int i = 0; i < len; ++i) {
		char fn[200];
		sprintf(fn, "out/tmp.%d.%d", gids[i], nrnmpi_myid);
		BBSS_IO* io = new BBSS_TxtFileOut(fn);
		ss->f = io;
		ss->gidobj(gids[i]);
		delete io;
	}
	if (len) {
		free(gids);
		free(sizes);
	}
	return 0.;
}

static double save_test_bin(void* v) {//only for whole cells
	int len, *gids, *sizes, global_size;
	char* buf;
	char fname[100];
	FILE* f;
	BBSaveState* ss = (BBSaveState*)v;
	usebin_ = 1;
	void* ref = bbss_buffer_counts(&len, &gids, &sizes, &global_size);
	if (nrnmpi_myid == 0) { // save global time
		buf = new char[global_size];
		bbss_save_global(ref, buf, global_size);
		sprintf(fname, "binbufout/global.%d", global_size);
		nrn_assert(f = fopen(fname, "w"));
		fwrite(buf, sizeof(char), global_size, f);
		fclose(f);
		delete [] buf;

		sprintf(fname, "binbufout/global.size");
		nrn_assert(f = fopen(fname, "w"));
		fprintf(f, "%d\n", global_size);
		fclose(f);
	}
	for (int i = 0; i < len; ++i) {
		buf = new char[sizes[i]];
		bbss_save(ref, gids[i], buf, sizes[i]);
		sprintf(fname, "binbufout/%d.%d", gids[i], sizes[i]);
		nrn_assert(f = fopen(fname, "w"));
		fwrite(buf, sizeof(char), sizes[i], f);
		fclose(f);
		delete [] buf;

		sprintf(fname, "binbufout/%d.size", gids[i]);
		nrn_assert(f = fopen(fname, "w"));
		fprintf(f, "%d\n", sizes[i]);
		fclose(f);
	}
	if (len) {
        free(gids);
        free(sizes);
    }
	bbss_save_done(ref);
	return 0.;
}

declareNrnHash(PointProcessMap, Point_process*, int)
implementNrnHash(PointProcessMap, Point_process*, int)
static PointProcessMap* pp_ignore_map;

static double ppignore(void* v) {
	if (ifarg(1)) {
		Point_process* pp = ob2pntproc(*(hoc_objgetarg(1)));
		if (!pp_ignore_map) {
			pp_ignore_map = new PointProcessMap(100);
		}
		(*pp_ignore_map)[pp] = 0; // naive set instead of map
	}else if (pp_ignore_map) { // clear
		delete pp_ignore_map;
		pp_ignore_map = 0;
	}
	return 0.;
}

static int ignored(Prop* p) {
	int i;
	Point_process* pp = (Point_process*)p->dparam[1]._pvoid;
	if (pp_ignore_map) {
		if (pp_ignore_map->find(pp, i)) {
			return 1;
		}
	}
	return 0;
}

void* bbss_buffer_counts(int* len, int** gids, int** sizes, int* global_size) {
	usebin_ = 1;
	BBSaveState* ss = new BBSaveState();
	*global_size = 0;
	if (nrnmpi_myid == 0) { // save global time
		BBSS_Cnt* io = new BBSS_Cnt();
		io->d(1, nrn_threads->_t);
		*global_size = io->bytecnt();
		delete io;
	}
	*len = ss->counts(gids, sizes);
	return ss;
}
void bbss_save_global(void* bbss, char* buffer, int sz) { // call only on host 0
	usebin_ = 1;
	BBSS_IO* io = new BBSS_BufferOut(buffer, sz);
	io->d(1, nrn_threads->_t);
	delete io;
}
void bbss_restore_global(void* bbss, char* buffer, int sz) { // call on all hosts
	usebin_ = 1;
	BBSS_IO* io = new BBSS_BufferIn(buffer, sz);
	io->d(1, nrn_threads->_t);
	t = nrn_threads->_t;
	delete io;
	clear_event_queue();
#if NRNMPI
	use_spikecompress_ = nrn_use_compress_;
	use_gidcompress_ = nrn_use_localgid_;
	nrn_use_compress_ = false;
	nrn_use_localgid_ = false;
#endif
	if (nrn_use_bin_queue_) {
		nrn_binq_enqueue_error_handler = bbss_early;
	}
}
void bbss_save(void* bbss, int gid, char* buffer, int sz) {
	usebin_ = 1;
	BBSaveState* ss = (BBSaveState*) bbss;
	BBSS_IO* io = new BBSS_BufferOut(buffer, sz);
	ss->f = io;
	ss->gidobj(gid);
	delete io;
}
void bbss_restore(void* bbss, int gid, int ngroup, char* buffer, int sz) {
	usebin_ = 1;
	BBSaveState* ss = (BBSaveState*) bbss;
	BBSS_IO* io = new BBSS_BufferIn(buffer, sz);
	ss->f = io;
	for (int i = 0; i < ngroup; ++i) {
		ss->gidobj(gid);
		t = nrn_threads->_t;
	}
	delete io;
}
void bbss_save_done(void* bbss) {
	BBSaveState* ss = (BBSaveState*) bbss;
	delete ss;
}

static void bbss_remove_delivered() {
	TQueue* tq = net_cvode_instance_event_queue(nrn_threads);

	// PreSyn and NetCon spikes are on the queue. To determine the spikes
	// that have already been delivered the PreSyn items that have
	// NetCon delivery times < t need to get fanned out to NetCon items
	// on the queue before checking the times.
	tq_presyn_fanout = new TQItemList(100);
	callback_mode = 2;
	tq->forall_callback(tqcallback);
	for (int i = tq_presyn_fanout->count()-1; i >= 0; --i) {
		TQItem* qi = tq_presyn_fanout->item(i);
		double td = qi->t_;
		PreSyn* ps = (PreSyn*)qi->data_;
		tq->remove(qi);
		ps->fanout(td, net_cvode_instance, nrn_threads);
	}
	delete tq_presyn_fanout;

	// now everything on the queue which is subject to removal is a NetCon
	tq_removal_list = new TQItemList(100);
	callback_mode = 3;
	tq->forall_callback(tqcallback);
	for (int i = tq_removal_list->count()-1; i >= 0; --i) {
		TQItem* qi = tq_removal_list->item(i);
		int type = ((DiscreteEvent*)qi->data_)->type();
if (type != NetConType) {
printf("%d type=%d\n", nrnmpi_myid, type);
}
		assert(type == NetConType);
		tq->remove(qi);
	}
	delete tq_removal_list;
}

void bbss_restore_done(void* bbss) {
	if (bbss) {
		BBSaveState* ss = (BBSaveState*) bbss;
		delete ss;
	}
	// We did not save the NetParEvent. In fact, during
	// saving, the minimum delay might be different than
	// what is needed with this distribution and nhost.
	NetParEvent* npe = new NetParEvent();
	npe->ithread_ = 0;
	npe->savestate_restore(t, net_cvode_instance);
	delete npe;
	nrn_spike_exchange(nrn_threads);
#if BGPDMA
	// only necessary if multisend method is using two subintervals
	if (use_bgpdma_) {
		nrn_spike_exchange(nrn_threads);
	}
#endif
	// The queue may now contain spikes which have already been
	// delivered and so those must be removed if delivery time < t.
	// Actually, the Presyn spikes may end up as NetCon spikes some
	// of which may have been already delivered and some not (due to
	// variations in NetCon.delay .
	bbss_remove_delivered();

#if NRNMPI
	// turn spike compression back on 
	nrn_use_localgid_ = use_gidcompress_;
	nrn_use_compress_ = use_spikecompress_;
#endif

	// compare the restored queue count for each presyn spike with the
	// expected undelivered NetCon count what was read from the file
	// for each PreSyn. This is optional due to the computational
	// expense but is helpful to point toward a diagnosis of errors.
#if QUEUECHECK
	bbss_queuecheck();
#endif
	nrn_binq_enqueue_error_handler = 0;
}

static double restore_test(void* v) {
	usebin_ = 0;
	int* gids, *sizes;
	BBSaveState* ss = (BBSaveState*)v;
	// restore global time
	BBSS_IO* io = new BBSS_TxtFileIn("in/tmp");
	io->d(1, nrn_threads->_t);
	t = nrn_threads->_t;
	delete io;
	
 	clear_event_queue();
	// turn off compression. Will turn back on in bbs_restore_done.
#if NRNMPI
	use_spikecompress_ = nrn_use_compress_;
	use_gidcompress_ = nrn_use_localgid_;
	nrn_use_compress_ = false;
	nrn_use_localgid_ = false;
#endif

	if (nrn_use_bin_queue_) {
		nrn_binq_enqueue_error_handler = bbss_early;
	}

	int len = ss->counts(&gids, &sizes);
	for (int i = 0; i < len; ++i) {
		char fn[200];
		sprintf(fn, "in/tmp.%d", gids[i]);
		BBSS_IO* io = new BBSS_TxtFileIn(fn);
		ss->f = io;
		int ngroup;
		ss->f->i(ngroup);
		for (int j=0; j < ngroup; ++j) {
			ss->gidobj(gids[i]);
		}
		delete io;
	}
	if (len) {
        free(gids);
        free(sizes);
	}
	bbss_restore_done(0);
	return 0.;
}

static double restore_test_bin(void* v) { //assumes whole cells
	usebin_ = 1;
	int len, *gids, *sizes, global_size, npiece, sz;
	void* ref;
	char* buf;
	char fname[100];
	FILE* f;
	clear_event_queue();
	ref = bbss_buffer_counts(&len, &gids, &sizes, &global_size);

	sprintf(fname, "binbufin/global.size");
	nrn_assert(f = fopen(fname, "r"));
	nrn_assert(fscanf(f, "%d\n", &sz) == 1);
	fclose(f);
	global_size = sz;
	buf = new char[sz];

	sprintf(fname, "binbufin/global.%d", global_size);
	f = fopen(fname, "r");
	if (!f) { printf("%d fail open for read %s\n", nrnmpi_myid, fname);}
	assert(f);
	nrn_assert(fread(buf, sizeof(char), global_size, f) == global_size);
	fclose(f);
	bbss_restore_global(ref, buf, global_size);
	delete [] buf;

	for (int i = 0; i < len; ++i) {
		npiece = 1;

		sprintf(fname, "binbufin/%d.size", gids[i]);
		nrn_assert(f = fopen(fname, "r"));
		nrn_assert(fscanf(f, "%d\n", &sz) == 1);
		fclose(f);
		//if (sz != sizes[i]) {
		//	printf("%d note sz=%d size=%d\n", nrnmpi_myid, sz, sizes[i]);
		//}

		buf = new char[sz];
		sprintf(fname, "binbufin/%d.%d", gids[i], sz);
		f = fopen(fname, "r");
		if (!f) { printf("%d fail open for read %s\n", nrnmpi_myid,fname);}
		assert(f);
		nrn_assert(fread(buf, sizeof(char), sz, f) == sz);
		fclose(f);
		bbss_restore(ref, gids[i], npiece, buf, sz);
		delete [] buf;
	}

	if (len) {
        free(gids);
        free(sizes);
    }
	bbss_restore_done(ref);
	return 0.;
}

static double vector_play_init(void* v) {
	nrn_play_init();
	return 0.;
}

static Member_func members[] = {
	// text test
        "save", save,
        "restore", restore,
	"save_test", save_test,
	"restore_test", restore_test,
	"save_test_bin", save_test_bin,
	"restore_test_bin", restore_test_bin,
	// binary test
	"save_request", save_request,
	"save_gid", save_gid,
	"restore_gid", restore_gid,
	// indicate which point processes are to be ignored
	"ignore", ppignore,
	// allow Vector.play to work
	"vector_play_init", vector_play_init,
        0, 0
};

void BBSaveState_reg() {
        class2oc("BBSaveState", cons, destruct, members, NULL, NULL, NULL);
}

// from savstate.cpp
struct StateStructInfo {
	int offset;
	int size;
	Symbol* callback;
};
static StateStructInfo* ssi;
static cTemplate* nct;
static void ssi_def() {
	if (nct) { return; }
	Symbol* s = hoc_lookup("NetCon");
	nct = s->u.ctemplate;
	ssi = new StateStructInfo[n_memb_func];
	for (int im=0; im < n_memb_func; ++im) {
		ssi[im].offset = -1;
		ssi[im].size = 0;
		ssi[im].callback = 0;
		if (!memb_func[im].sym) {
			continue;
		}
		NrnProperty* np = new NrnProperty(memb_func[im].sym->name);
		// generally we only save STATE variables. However for
		// models containing a NET_RECEIVE block, we also need to
		// save everything except the parameters
		// because they often contain
		// logic and analytic state values. Unfortunately, it is often
		// the case that the ASSIGNED variables are not declared as
		// RANGE variables so to avoid missing state, save the whole
		// param array including PARAMETERs.
	    if  (pnt_receive[im]) {
		ssi[im].offset = 0;
		ssi[im].size = np->prop()->param_size;
	    }else{
		int type = STATE;
		for (Symbol* sym = np->first_var(); np->more_var(); sym = np->next_var()) {
			if (np->var_type(sym) == type
			    || np->var_type(sym) == STATE
			    || sym->subtype == _AMBIGUOUS) {
				if (ssi[im].offset < 0) {
					ssi[im].offset = np->prop_index(sym);
				}
				ssi[im].size += hoc_total_array_data(sym, 0);
			}
		}
	    }
	    if (memb_func[im].is_point) {
		//check for callback named bbsavestate
		cTemplate* ts = nrn_pnt_template_[im];
		ssi[im].callback = hoc_table_lookup("bbsavestate", ts->symtable);
		//if (ssi[im].callback) {
		//	printf("callback %s.%s\n", ts->sym->name, ssi[im].callback->name);
		//}
	    }else{
		// check for callback named bbsavestate in a density mechanism
		char name[256];
		sprintf(name, "bbsavestate_%s", memb_func[im].sym->name);
		ssi[im].callback = hoc_table_lookup(name, hoc_built_in_symlist);
		//if (ssi[im].callback) {
		//	printf("callback %s\n", ssi[im].callback->name);
		//}
	    }
	    delete np;
	}
}

// if we know the Point_process, we can find the NetCon
// BB project never has more than one NetCon connected to a Synapse.
// But that may not hold in general so extend to List of NetCon using DEList.
// and assume the list is same order on save/restore.
typedef struct DEList { DiscreteEvent* de; struct DEList* next; } DEList;
declareNrnHash(PP2DE, Point_process*, DEList*)
implementNrnHash(PP2DE, Point_process*, DEList*)
static PP2DE* pp2de;
// NetCon.events
declareList(DblList, double)
implementList(DblList, double)
declareNrnHash(NetCon2DblList, NetCon*, DblList*)
implementNrnHash(NetCon2DblList, NetCon*, DblList*)
static NetCon2DblList* nc2dblist;

class SEWrap : public DiscreteEvent {
public:
	SEWrap(const TQItem*, DEList*);
	~SEWrap();
	int type() { return se->type();}
	SelfEvent* se;
	double tt;
	int ncindex; // in the DEList or -1 if no NetCon for self event.
};
SEWrap::SEWrap(const TQItem* tq, DEList* dl) {
	tt = tq->t_;
	se = (SelfEvent*)tq->data_;
	if (se->weight_) {
		ncindex = 0;
		for (;dl && dl->de && dl->de->type() == NetConType; dl = dl->next, ++ncindex) {
			if (se->weight_ == ((NetCon*)dl->de)->weight_) {
				return;
			}
		}
		// There used to be an assert(0) here.
		// There is no associated NetCon (or the NetCon is being
		// ignored, perhaps because it is used to reactivate a
		// BinReportHelper after a bbsavestate restore).
		// In either case, we should also ignore the
		// SelfEvent. So send back a special ncindex sentinal value.

		ncindex = -2;
	}else{
		ncindex = -1;
	}
}
SEWrap::~SEWrap() {}

declarePtrList(SEWrapList, SEWrap)
implementPtrList(SEWrapList, SEWrap)
static SEWrapList* sewrap_list;

declareNrnHash(Int2Int, int, int)
implementNrnHash(Int2Int, int, int)
static Int2Int* base2spgid; // base gids are the host independent key for a cell which was multisplit

declareNrnHash(Int2DblList, int, DblList*)
implementNrnHash(Int2DblList, int, DblList*)
static Int2DblList* src2send; // gid to presyn send time map
static int src2send_cnt;
// the DblList was needed in place of just a double because there might
// be several spikes from a single PreSyn (interval between spikes less
// than the maximum NetCon delay for that PreSyn).
// Given a current bug regarding dropped spikes with bin queuing as well
// as the question about the proper handling of a spike with delivery
// time equal to the save state time, it is useful to provide a mechanism
// that allows us to assert that the number of spikes on the queue
// at a save (conceptually the number of NetCon fanned out from the
// PreSyn that have not yet been delivered)
// is identical to the number of spikes on the queue after
// a restore. To help with this, we add a count of the "to be delivered"
// NetCon spikes on the queue to each PreSyn item in the saved file.
// For least effort, given the need to handle counts in the same
// fashion as we handle the presyn send times in the DblList, we merely
// represent (ts, cnt) pairs as adjacent items in the DblList.
// For saving, the undelivered netcon count is always computed. After
// restore this information can be checked against the actual
// netcon count on the queue by defining QUEUECHECK to 1. Note that
// computing the undelivered netcon count involves: 1) each  machine
// processing its queue's NetCon and PreSyn spikes using tqcallback
// with callback_mode 1. 2) aggregating the PreSyn counts on some
// machine using scatteritems(). 3) lastly sending the total count per
// PreSyn to the machine that owns the PreSyn gid (see mk_presyn_info).
// This 3-fold process is reused during restore to check the counts ane
// we have factored out the relevant code so it can be used for both
// save and restore (for the latter see bbss_queuecheck()).
#if QUEUECHECK
static Int2DblList* queuecheck_gid2unc;
#endif

static double binq_time(double tt) {
	double dt = nrn_threads->_dt;
	return int(tt/dt + 0.5 + 1.e-10)*dt; // 0.5 due to deliver at dt/2
}

static void bbss_early(double td, TQItem* tq) {
	int type = ((DiscreteEvent*)tq->data_)->type();
//	printf("bbss_early td=%g tq->t=%g t=%g bqt=%g type=%d\n", td, tq->t_, t, binq_time(td), type);
	// if NetCon, discard. If PreSyn, fanout.
	if (type == NetConType) {
		return;
	}else if (type == PreSynType) {
		PreSyn* ps = (PreSyn*)tq->data_;
		ps->fanout(binq_time(td), net_cvode_instance, nrn_threads);
	}else{
		assert(0);
	}
}

static void tqcallback(const TQItem* tq, int i) {
	int type = ((DiscreteEvent*)tq->data_)->type();
	switch (callback_mode) {
	case 0: { // the self events
		if (type == SelfEventType) {
			Point_process* pp = ((SelfEvent*)tq->data_)->target_;
			DEList *dl=0, *dl1=0;
			SEWrap* sew = 0;
			if(pp2de->find(pp, dl1)) {
				sew = new SEWrap(tq, dl1);
			}else{
				dl1 = 0;
				sew = new SEWrap(tq, 0);
			}
			if (sew->ncindex == -2) { // ignore the self event
//printf("%d Ignoring a SelfEvent to %s\n", nrnmpi_myid, memb_func[pp->prop->type].sym->name);
				delete sew;
				sew = 0;
			}
			if (sew) {
				sewrap_list->append(sew);
				DEList* dl = new DEList;
				dl->next = 0;
				dl->de = sew;
				if (dl1) {
					while(dl1->next) { dl1 = dl1->next; }
					dl1->next = dl;
				}else{
					(*pp2de)[pp] = dl;
				}
			}
		}
	    }
		break;

	case 1: { // the NetCon (and PreSyn) events
		int srcid, i; double ts, tt; PreSyn* ps;
		int cntinc;// number on queue to be delivered due to this event
		NetCon* nc = 0;
		if (type == NetConType) {
			nc = (NetCon*)tq->data_;
			ps = nc->src_;
			ts = tq->t_ - nc->delay_;
			cntinc = 1;
		}else if (type == PreSynType) {
			ps = (PreSyn*)tq->data_;
			ts = tq->t_ - ps->delay_;
			cntinc = ps->dil_.count();
		}else{
			return;
		}
		if (ps) {
			if (ps->gid_ >= 0) { // better not be from NetCon.event
				srcid = ps->gid_;
				DblList* dl;
				if (src2send->find(srcid, dl)) {
					// If delay is long and spiking
					// is fast there may be 
					// another source spike when
					// its previous spike is still on
					// the queue. So we might have
					// to add another initiation time.
// originally there was an assert error here under the assumption that all
// spikes from a PreSyn were delivered before that PreSyn fired again.
// The assumption did not hold for existing Blue Brain models. Therefore
// we extend the algorithm to any number of spikes with different initiation
// times from the same PreSyn.
// For sanity we assume Presyns do not fire more than once every 0.1 ms.
// Unfortunately this possibility makes mpi exchange much more difficult as
// the number of doubles exchanged can be greater than the number of src2send
// gids.
					// Add another initiation time if needed
					double m = 1e9; // > .1 add
					int im = -1; // otherwise assert < 1e-12
					for (i = 0; i < dl->count(); i+=2) {
						double x = fabs(dl->item(i) - ts);
						if (x < m) {
							m = x;
							im = i;
						}
					}
					if (m > 0.1) {
						dl->append(ts);
						dl->append(cntinc);
					}else if (m > 1e-12) {
						assert(0);
					}else{
						dl->item_ref(im+1) += cntinc;
					}
				}else{
					dl = new DblList(2);
					dl->append(ts);
					dl->append(cntinc);
					(*src2send)[srcid] = dl;
					++src2send_cnt; // distinct PreSyn
				}
			
			}else{
				// osrc state should already have been associated
				// with the target Point_process and we should
				// now associate this event as well
				if (ps->osrc_) { // NetStim possibly
					// does not matter if NetCon.event
					// or if the event was sent from
					// the local stimulus. Can't be from
					// a PreSyn event since we demand
					// that there be only one NetCon
					// from this stimulus.
					assert(nc);
					DblList* db = 0;
					if (!nc2dblist->find(nc, db)) {
						db = new DblList(10);
						(*nc2dblist)[nc] = db;
					}
					db->append(tq->t_);
				}else{ // assume from NetCon.event
					// ps should be unused_presyn
//printf("From NetCon.event\n");
					assert(nc);
					DblList* db = 0;
					if (!nc2dblist->find(nc, db)) {
						db = new DblList(10);
						(*nc2dblist)[nc] = db;
					}
					db->append(tq->t_);
				}
			}			
		}
	    }
		break;

	case 2: {
		// some PreSyns may get fanned out into NetCon, only
		// some of which have already been delivered. If this is the
		// case, add the q item to the fanout list. Do not put in
		// list if all or none have been delivered.
		// Actually, for simplicity, just put all the PreSyn items in
		// the list for fanout if PreSyn::deliver time < t.
		if (type == PreSynType) {
PreSyn* ps = (PreSyn*)tq->data_;
printf("PreSyn %d deliver %g\n", ps->gid_, tq->t_);
			if (tq->t_ < t) {
				tq_presyn_fanout->append((TQItem*)tq);
			}
		}
	    }
		break;
	case 3: {
		// ??? have the ones at t been delivered or not?
		if (type != NetConType) { break; }
		if (tq->t_ == t) {
printf("%d Have not decided whether to remove this event\n", nrnmpi_myid);
			assert(0);
		}
		double td = tq->t_;
		double tt = t;
		// td should be on bin queue boundary
		if (nrn_use_bin_queue_) {
			td = binq_time(td);
			tt = binq_time(tt);
		}
		if (td < tt) {
//((DiscreteEvent*)tq->data_)->pr("tq_removal_list", td, net_cvode_instance);
			tq_removal_list->append((TQItem*)tq);
		}
	    }
		break;
	}
}
void BBSaveState::mk_pp2de() {
	hoc_Item* q;
	assert(!pp2de); // one only or make it a field.
	int n = nct->count;
	pp2de = new PP2DE(n+1);
	sewrap_list = new SEWrapList(1000);
	ITERATE(q, nct->olist) {
		NetCon* nc = (NetCon*)OBJ(q)->u.this_pointer;
		// ignore NetCon with no PreSyn.
		// i.e we do not save or restore information about
		// NetCon.event. We do save NetCons with local NetStim source.
		// But that NetStim can only be the source for one NetCon.
		// (because its state info will be attached to the
		// target synapse)
		if (!nc->src_) {
			continue;
		}
		//has a gid or else only one connection
		assert(nc->src_->gid_ >= 0 || nc->src_->dil_.count() == 1);
		Point_process* pp = nc->target_;
		DEList* dl = new DEList;
		dl->de = nc;
		dl->next = 0;
		DEList* dl1;
		// NetCons first then SelfEvents
		if (pp2de->find(pp, dl1)) {
			while(dl1->next) { dl1 = dl1->next; }
			dl1->next = dl;
		}else{
			(*pp2de)[pp] = dl;
		}
	}
	TQueue* tq = net_cvode_instance_event_queue(nrn_threads);
	callback_mode = 0;
	tq->forall_callback(tqcallback);
}

static Int2DblList* presyn_queue;

static void del_presyn_info() {
	NrnHashIterate(Int2DblList, presyn_queue, DblList*, dl) {
		delete dl;
	}}}
	delete presyn_queue;
	presyn_queue = 0;
	if (nc2dblist) {
		NrnHashIterate(NetCon2DblList, nc2dblist, DblList*, dl) {
			delete dl;
		}}}
		delete nc2dblist;
		nc2dblist = 0;
	}
}

void BBSaveState::del_pp2de() {
	DEList* dl, *dl1;
	if (!pp2de) { return; }
	NrnHashIterate(PP2DE, pp2de, DEList*, dl) {
		for (; dl; dl = dl1) {
			dl1 = dl->next;
			// need to delete SEWrap items but dl->de that
			// are not SEWrap may already be deleted.
			// Hence the extra sewrap_list and items are
			// deleted below.
			delete dl;
		}
	}}}
	delete pp2de;
	pp2de = 0;
	if (sewrap_list) {
		for (int i=0; i < sewrap_list->count(); ++i) {
			delete sewrap_list->item(i);
		}
		delete sewrap_list;
		sewrap_list = NULL;
	}
	del_presyn_info();
}

BBSaveState::BBSaveState(){
	if (!ssi) {
		ssi_def();
	}
}
BBSaveState::~BBSaveState(){
	if (pp2de) {
		del_pp2de();
	}
}
void BBSaveState::apply(BBSS_IO* io) {
	f = io;
	bbss = this;
	core();
};

void BBSaveState::core() {
	if (debug) { sprintf(dbuf, "Enter core()"); PDEBUG; }
	char buf[100];
	sprintf(buf, "//core");
	f->s(buf, 1);
	init();
	gids();
	finish();
	if (debug) { sprintf(dbuf, "Leave core()"); PDEBUG; }
}

void BBSaveState::init() {
	mk_base2spgid();
	mk_pp2de();
	mk_presyn_info();
}

void BBSaveState::finish() {
	del_pp2de();
	del_presyn_info();
	if (base2spgid) {
		delete base2spgid;
		base2spgid = 0;
	}
	if (f->type() == BBSS_IO::IN) {
		nrn_spike_exchange(nrn_threads);
	}
}

static void base2spgid_item(int spgid, Object* obj) {
	int base = spgid % 10000000;
	int sp;
	if (spgid == base || !base2spgid->find(base, sp)) {
		(*base2spgid)[base] = spgid;
	}
}

void BBSaveState::mk_base2spgid() {
	if (base2spgid) { delete base2spgid; }
	base2spgid = new Int2Int(1000);
	nrn_gidout_iter(&base2spgid_item);
}

// c++ blue brain write interface in two phases. First return to bb what is
// needed for each gid and then get from bb a series of gid, buffer
// pairs to write the buffer. The read interface only requires a single
// phase where we receive from bb a series of gid, buffer pairs for
// reading. However a final step is required to transfer PreSyn spikes by
// calling BBSaveState:finish().

// first phase for writing
// the caller is responsible for free(*gids) and free(*cnts)
// when finished with them.
int BBSaveState::counts(int** gids, int** cnts) {
	f = new BBSS_Cnt();
	BBSS_Cnt* c = (BBSS_Cnt*)f;
	bbss = this;
	init();
	// how many
	int gidcnt = 0;
	NrnHashIterateKeyValue(Int2Int, base2spgid, int, base, int, spgid) {
		++gidcnt;
	}}}		
	if (gidcnt) {
        // using malloc instead of new as we might need to
        // deallocate from c code (of mod files)
		*gids = (int *) malloc(gidcnt * sizeof(int));
		*cnts = (int *) malloc(gidcnt * sizeof(int));

        if(*gids==NULL || *cnts==NULL) {
            printf("Error : Memory allocation failure in BBSaveState\n");
#if NRNMPI
            nrnmpi_abort(-1);
#else
            abort();
#endif
        }
	}
	gidcnt = 0;
	NrnHashIterateKeyValue(Int2Int, base2spgid, int, base, int, spgid) {
		(*gids)[gidcnt] = base;
		c->ni = c->nd = c->ns = c->nl = 0;
		Object* obj = nrn_gid2obj(spgid);
		gidobj(spgid, obj);
		(*cnts)[gidcnt] = c->bytecnt();
		++gidcnt;
	}}}		
	delete f;
	return gidcnt;
}

// note that a cell on a processor consists of any number of
// pieces of a whole cell and each piece has its own spgid
// (see nrn/share/lib/hoc/binfo.hoc) The piece with the output port
// has spgid == basegid with a PreSyn.output_index_ = basegid
// and the others have a PreSyn.output_index_ = -2
// in all cases the PreSyn.gid_ = spgid. (see netpar.cpp BBS::cell())
// Note that binfo.hoc provides base_gid(spgid) which is spgid%msgid
// where msgid is typically 1e7 if the maximum basegid is less than that.
// thishost_gid(basegid) returns the minimum spgid
// of the pieces for the cell on thishost. It would be great if we
// did not have to use the value of msgid. How could we safely derive it?
// In general, different models invent different mappings between basegid
// and msgid so ultimately (if not restricted to Blue Brain usage)
// it is necessary to supply a user defined hoc (or python) callback
// to compute base_gid(spgid). Then the writer and reader can create
// a map of basegid to cell and use only basegid (never reading or writing
// the spgid). If there is no basegid callback then we presume there
// are no split cells.

// a gid item for second phase of writing
void BBSaveState::gid2buffer(int gid, char* buffer, int size) {
	if (f) { delete f; }
	f = new BBSS_BufferOut(buffer, size);
	Object* obj = nrn_gid2obj(gid);	
	gidobj(gid, obj);
	delete f;
	f = 0;
}

// a gid item for first phase of reading
void BBSaveState::buffer2gid(int gid, char* buffer, int size) {
	if (f) { delete f; }
	f = new BBSS_BufferIn(buffer, size);
	Object* obj = nrn_gid2obj(gid);	
	gidobj(gid, obj);
	delete f;
	f = 0;
}

static void cb_gidobj(int gid, Object* obj) {
	bbss->gidobj(gid, obj);
}

void BBSaveState::gids() {
	if (debug) { sprintf(dbuf, "Enter gids()"); PDEBUG; }
	nrn_gidout_iter(&cb_gidobj);
	if (debug) { sprintf(dbuf, "Leave gids()"); PDEBUG; }
}

void BBSaveState::gidobj(int basegid) {
	int spgid;
	Object* obj;
	assert(base2spgid->find(basegid, spgid));
	obj = nrn_gid2obj(spgid);
	gidobj(spgid, obj);
}

void BBSaveState::gidobj(int gid, Object* obj) {
	char buf[256];
        int rgid = gid;
	if (debug) { sprintf(dbuf, "Enter gidobj(%d, %s)", gid, hoc_object_name(obj)); PDEBUG; }
	sprintf(buf, "begin cell");
	f->s(buf, 1);
	f->i(rgid);  //on reading, we promptly ignore rgid from now on, stick with gid
	int size = cellsize(obj);
	f->i(size);
	cell(obj);
	possible_presyn(gid);
	sprintf(buf, "end cell");
	f->s(buf, 1);
	if (debug) { sprintf(dbuf, "Leave gidobj(%d, %s)", gid, hoc_object_name(obj)); PDEBUG; }
}

int BBSaveState::cellsize(Object* c){
	if (debug) { sprintf(dbuf, "Enter cellsize(%s)", hoc_object_name(c)); PDEBUG; }
	int cnt = -1;
	if (f->type() == BBSS_IO::OUT) {
		BBSS_IO* sav = f;
		f = new BBSS_Cnt();
		cell(c);
		cnt = ((BBSS_Cnt*)f)->bytecnt();
		delete f;
		f = sav;
	}
	if (debug) { sprintf(dbuf, "Leave cellsize(%s)", hoc_object_name(c)); PDEBUG; }
	return cnt;
}

// Here is the major place where there is a symmetry break between writing
// and reading. That is because of the possibility of splitting where
// not only the pieces are distributed differently between saving and restoring
// processors but the pieces themselves (as well as the piece gids)
// are different. It is only the union of pieces that describes a complete cell.
// Symmetry exists again at the section level
// For writing the cell is defined as the existing set of pieces in the hoc
// cell Object. Here is cell we write enough prefix info about the Section
// to be able to determine if it exists before reading with section(sec);

void BBSaveState::cell(Object* c) {
	if (debug) { sprintf(dbuf, "Enter cell(%s)", hoc_object_name(c)); PDEBUG; }
	char buf[256];
	sprintf(buf, "%s", hoc_object_name(c));
	f->s(buf);
	if (!is_point_process(c)) { // must be cell object
	    if (f->type() != BBSS_IO::IN) { // writing, counting
		// from forall_section in cabcode.c
		// count, and iterate from first to last
		hoc_Item* qsec, *first, *last;
		qsec = c->secelm_;
		int cnt = 0;
		Section* sec;
		for (first = qsec; first->itemtype &&
		  hocSEC(first)->prop->dparam[6].obj == c; first = first->prev) {
			sec = hocSEC(first);
			if (sec->prop) {
				++cnt;
			}
		}
		first = first->next;
		last = qsec->next;
		f->i(cnt);
		for (qsec = first; qsec != last; qsec = qsec->next) {
			Section* sec = hocSEC(qsec);
			if (sec->prop) {
				// the section exists
				sprintf(buf, "begin section");
				f->s(buf);
				section_exist_info(sec);
				section(sec);
				sprintf(buf, "end section");
				f->s(buf);
			}
		}
	    }else{ // reading
		Section* sec;
		int i, cnt, indx, size;
		f->i(cnt);
		for (i=0; i < cnt; ++i) {
			sprintf(buf, "begin section");
			f->s(buf, 1);
			f->s(buf); // the section name
			f->i(indx); // section array index
			f->i(size);
			sec = nrn_section_exists(buf, indx, c);
			if (sec) {
				section(sec);
			}else{ // skip size bytes
				f->skip(size);
			}
			sprintf(buf, "end section");
			f->s(buf, 1);
		}
	    }
	}else{ // ARTIFICIAL_CELL
		Point_process* pnt = ob2pntproc(c);
		mech(pnt->prop);
	}
	if (debug) { sprintf(dbuf, "Leave cell(%s)", hoc_object_name(c)); PDEBUG; }
}

void BBSaveState::section_exist_info(Section* sec) {
	char buf[256];
	Symbol* sym = sec->prop->dparam[0].sym;
	sprintf(buf, "%s", sym->name);
	f->s(buf);
	int indx = sec->prop->dparam[5].i;
	f->i(indx);
	int size = sectionsize(sec);
	f->i(size, 1);
}

void BBSaveState::section(Section* sec) {
	if (debug) { sprintf(dbuf, "Enter section(%s)", sec->prop->dparam[0].sym->name); PDEBUG; }
	// section_exist_info(sec);
	seccontents(sec);
	if (debug) { sprintf(dbuf, "Leave section(%s)", sec->prop->dparam[0].sym->name); PDEBUG; }
}

int BBSaveState::sectionsize(Section* sec) {
	if (debug == 1) { sprintf(dbuf, "Enter sectionsize(%s)", sec->prop->dparam[0].sym->name); PDEBUG; }
	// should be same for both IN and OUT
	int cnt = -1;
	if (f->type() != BBSS_IO::CNT) {
		BBSS_IO* sav = f;
		f = new BBSS_Cnt();
		seccontents(sec);
		cnt = ((BBSS_Cnt*)f)->bytecnt();
		delete f;
		f = sav;
	}
	if (debug == 1) { sprintf(dbuf, "Leave sectionsize(%s)", sec->prop->dparam[0].sym->name); PDEBUG; }
	return cnt;
}

void BBSaveState::seccontents(Section* sec) {
	if (debug) { sprintf(dbuf, "Enter seccontents(%s)", sec->prop->dparam[0].sym->name); PDEBUG; }
	int i, nseg;
	char buf[100];
	sprintf(buf, "//contents");
	f->s(buf);
	nseg = sec->nnode-1;
	f->i(nseg, 1);
	for (i=0; i < nseg; ++i) {
		node(sec->pnode[i]);
	}
	node01(sec, sec->parentnode);
	node01(sec, sec->pnode[nseg]);
	if (debug) { sprintf(dbuf, "Leave seccontents(%s)", sec->prop->dparam[0].sym->name); PDEBUG; }
}

// all Point_process and mechanisms -- except IGNORE point process instances
void BBSaveState::node(Node* nd) {
	if (debug) { sprintf(dbuf, "Enter node(nd)"); PDEBUG; }
	int i;
	Prop* p;
	f->d(1, NODEV(nd));
	//count
	// On restore, new point processes may have been inserted in
	// the section and marked IGNORE. So we need to count only the
	// non-ignored.
	for (i=0, p = nd->prop; p; p = p->next) {
		if (p->type > 3) {
			if (memb_func[p->type].is_point) {
				if (!ignored(p)) {
					++i;
				}
			}else{ // density mechanism
				++i;
			}
		}
	}
	f->i(i, 1);
	for (p = nd->prop; p; p = p->next) {
		if (p->type > 3) {
			mech(p);
		}
	}
	if (debug) { sprintf(dbuf, "Leave node(nd)"); PDEBUG; }
}

// only Point_process that belong to Section
void BBSaveState::node01(Section* sec, Node* nd) {
	if (debug) { sprintf(dbuf, "Enter node01(sec, nd)"); PDEBUG; }
	int i;
	Prop* p;
	// It is not clear why the zero area node voltages need to be saved.
	// Without them, we get correct simulations after a restore for
	// whole cells but not for split cells.
	f->d(1, NODEV(nd));
	//count
	for (i=0, p = nd->prop; p; p = p->next) {
		if (memb_func[p->type].is_point) {
			Point_process* pp = (Point_process*)p->dparam[1]._pvoid;
			if (pp->sec == sec) {
				if (!ignored(p)) {
					++i;
				}
			}
		}
	}
	f->i(i, 1);
	for (p = nd->prop; p; p = p->next) {
		if (memb_func[p->type].is_point) {
			Point_process* pp = (Point_process*)p->dparam[1]._pvoid;
			if (pp->sec == sec) {
				mech(p);
			}
		}
	}
	if (debug) { sprintf(dbuf, "Leave node01(sec, nd)"); PDEBUG; }
}

void BBSaveState::mech(Prop* p) {
	if (debug) { sprintf(dbuf, "Enter mech(prop type %d)", p->type); PDEBUG; }
	int type = p->type;
	if (memb_func[type].is_point && ignored(p)) {
		return;
	}
	f->i(type, 1);
	char buf[100];
	sprintf(buf, "//%s", memb_func[type].sym->name);
	f->s(buf, 1);
	f->d(ssi[p->type].size, p->param + ssi[p->type].offset);
	Point_process* pp = 0;
	if (memb_func[p->type].is_point) {
		pp = (Point_process*)p->dparam[1]._pvoid;
		if (pnt_receive[p->type]) {
			// associated NetCon and queue SelfEvent
			// if the NetCon has a unique non-gid source (art cell)
			// that source is save/restored as well.
			netrecv_pp(pp);
		}
	}
	if (ssi[p->type].callback) { // model author dependent info
		// the POINT_PROCESS or SUFFIX has a bbsavestate function
		sprintf(buf, "callback");
		f->s(buf, 1);
		int narg = 2;
		double xdir = -1.0; // -1 size, 0 save, 1 restore
		double* xval = NULL; // returned for save, sent for restore.

		hoc_pushpx(&xdir);
		hoc_pushpx(xval);
		if (memb_func[p->type].is_point) {
			hoc_call_ob_proc(pp->ob, ssi[p->type].callback, narg);
			double d = hoc_xpop();
		}else{
			double d = nrn_call_mech_func(ssi[p->type].callback, narg, p, p->type);
		}
		int sz = int(xdir);
		xval = new double[sz];

		hoc_pushpx(&xdir);
		hoc_pushpx(xval);
		if (f->type() == BBSS_IO::IN) { // restore
			xdir = 1.;
			f->d(sz, xval);
			if (memb_func[p->type].is_point) {
				hoc_call_ob_proc(pp->ob, ssi[p->type].callback, narg);
				double d = hoc_xpop();
			}else{
				double d = nrn_call_mech_func(ssi[p->type].callback, narg, p, p->type);
			}
		}else{
			xdir = 0.;
			if (memb_func[p->type].is_point) {
				hoc_call_ob_proc(pp->ob, ssi[p->type].callback, narg);
				double d = hoc_xpop();
			}else{
				double d = nrn_call_mech_func(ssi[p->type].callback, narg, p, p->type);
			}
			f->d(sz, xval);
		}
		delete [] xval;
	}
	if (debug) { sprintf(dbuf, "Leave mech(prop type %d)", p->type); PDEBUG; }
}

void BBSaveState::netrecv_pp(Point_process* pp) {
	if (debug) { sprintf(dbuf, "Enter netrecv_pp(pp prop type %d)", pp->prop->type); PDEBUG; }
	char buf[1000];
	sprintf(buf, "%s", hoc_object_name(pp->ob));
	f->s(buf);

	int type = pp->prop->type;
	// associated NetCon, and queue SelfEvent
	DEList* dl, *dl1;
	if (!pp2de->find(pp, dl)) {dl = 0;}
	int cnt = 0;
	// dl has the NetCons first then the SelfEvents
	// NetCon
	for (cnt = 0, dl1 = dl; dl1 && dl1->de->type() == NetConType; dl1 = dl1->next) {
		++cnt;
	}
	f->i(cnt, 1);
sprintf(buf, "NetCon");
f->s(buf, 1);
	for (; dl && dl->de->type() == NetConType; dl = dl->next) {
		NetCon* nc = (NetCon*)dl->de;
		f->d(nc->cnt_, nc->weight_);
		if (f->type() != BBSS_IO::IN) { // writing, counting
			DblList* db = 0;
			int j = 0;
			if (nc2dblist && nc2dblist->find(nc, db)) {
				j = db->count();
				f->i(j);
				for (int i = 0; i < j; ++i) {
					double x = db->item(i);
					f->d(1, x);
				}
			}else{
				f->i(j);
			}		
			int has_stim = 0;
			if (nc->src_ && nc->src_->osrc_ && nc->src_->gid_ < 0) {
				// save the associated local stimulus
				has_stim = 1;
				f->i(has_stim, 1);
				Point_process* pp = ob2pntproc(nc->src_->osrc_);
				mech(pp->prop);
			}else{
				f->i(has_stim, 1);
			}
		}else{ // reading
			int j = 0;
			f->i(j);
			for (int i = 0; i < j; ++i) {
				double x;
				f->d(1, x);
				nrn_netcon_event(nc, x);
			}
			int has_stim = 0;
			f->i(has_stim);
			if (has_stim) {
			  assert((nc->src_ && nc->src_->osrc_ && nc->src_->gid_ < 0) == has_stim);
			  Point_process* pp = ob2pntproc(nc->src_->osrc_);
			  mech(pp->prop);
			}
		}
	}
	for (cnt = 0, dl1 = dl; dl1 && dl1->de->type() == SelfEventType; dl1 = dl1->next) {
		++cnt;
	}
	f->i(cnt);
	// SelfEvent existence depends on state and for reading the queue
	// is empty. So count and save is different from  restore
	if (f->type() != BBSS_IO::IN) {
		for (; dl && dl->de->type() == SelfEventType; dl = dl->next) {
			SEWrap* sew = (SEWrap*)dl->de;
sprintf(buf, "SelfEvent");
f->s(buf);
			f->d(1, sew->se->flag_);
			f->d(1, sew->tt);
			f->i(sew->ncindex);
			int moff = -1;
			if (sew->se->movable_) {
				moff  = (Datum*)(sew->se->movable_) - pp->prop->dparam;
			}
			f->i(moff);
		}		
	}else{ // restore
		for (int i=0; i < cnt; ++i) {
			int ncindex, moff;
			double flag, tt, *w;
f->s(buf);
			f->d(1, flag);
			f->d(1, tt);
			f->i(ncindex);
			f->i(moff);
			void** movable = 0;
			if(moff >= 0) {
				movable = &(pp->prop->dparam[moff]._pvoid);
			}
			if (ncindex == -1) {
				w = 0;
			}else{
				int j;
				for (j=0, dl1 = dl; j < ncindex; ++j, dl1 = dl1->next) {
					;
				}
				assert(dl1 && dl1->de->type() == NetConType);
				w = ((NetCon*)dl1->de)->weight_;
			}
			net_send(movable, w, pp, tt, flag);
		}
	}
	if (debug) { sprintf(dbuf, "Leave netrecv_pp(pp prop type %d)", pp->prop->type); PDEBUG; }
}

static int giddest_size;
static int* giddest;
static int* tsdest_cnts;
static double* tsdest;

// input scnt, sdipl ; output rcnt, rdispl
static void all2allv_helper(int* scnt, int* sdispl, int* rcnt, int* rdispl) {
	int i;
	int np = nrnmpi_numprocs;
	int* c = new int[np];
	rdispl[0] = 0;
	for (i=0; i < np; ++i) {
		c[i] = 1;
		rdispl[i+1] = rdispl[i] + c[i];
	}
	nrnmpi_int_alltoallv(scnt, c, rdispl, rcnt, c, rdispl);
	delete [] c;
	rdispl[0] = 0;
	for (i=0; i < np; ++i) {
		rdispl[i+1] = rdispl[i] + rcnt[i];
	}
}

static void all2allv_int2(int* scnt, int* sdispl, int* gidsrc, int* ndsrc) {
#if NRNMPI
	int i;
	int np = nrnmpi_numprocs;
	int* rcnt = new int[np];
	int* rdispl = new int[np+1];
	all2allv_helper(scnt, sdispl, rcnt, rdispl);

	giddest = 0;
	tsdest_cnts = 0;
	giddest_size = rdispl[np];
	if (giddest_size) {
		giddest = new int[giddest_size];
		tsdest_cnts = new int[giddest_size];
	}
	nrnmpi_int_alltoallv(gidsrc, scnt, sdispl, giddest, rcnt, rdispl);
	nrnmpi_int_alltoallv(ndsrc, scnt, sdispl, tsdest_cnts, rcnt, rdispl);

	delete [] rcnt;
	delete [] rdispl;
#else
	assert(0);
#endif
}

static void all2allv_dbl1(int* scnt, int* sdispl, double* tssrc) {
#if NRNMPI
	int i, size;
	int np = nrnmpi_numprocs;
	int* rcnt = new int[np];
	int* rdispl = new int[np+1];
	all2allv_helper(scnt, sdispl, rcnt, rdispl);

	tsdest = 0;
	size = rdispl[np];
	if (size) {
		tsdest = new double[size];
	}
	nrnmpi_dbl_alltoallv(tssrc, scnt, sdispl, tsdest, rcnt, rdispl);

	delete [] rcnt;
	delete [] rdispl;
#else
	assert(0);
#endif
}

static void scatteritems() {
	// since gid queue items with the same gid are scattered on
	// many processors, we send all the gid,tscnt pairs (and tslists
	// with undelivered NetCon counts)
	// to the round-robin host (we do not know the gid owner host yet).
	int i, gid, host;
	DblList* dl;
	src2send_cnt = 0;
	src2send = new Int2DblList(1000);
	TQueue* tq = net_cvode_instance_event_queue(nrn_threads);
	// if event on queue at t we will not be able to decide whether or
	// not it should be delivered during restore.
	// The assert was moved into mk_presyn_info since this function is
	// reused by bbss_queuecheck and the error in that context will
	// be analyzed there.
	// assert(tq->least_t() > nrn_threads->_t);
	callback_mode = 1;
	tq->forall_callback(tqcallback);
	// space inefficient but simple support analogous to pc.all2all
	int* gidsrc = 0;
	int* ndsrc = 0; // count for each DblList, parallel to gidsrc
	double* tssrc = 0; // all the doubles (and undelivered NetCon counts) in every DblList
	if (src2send_cnt) {
		int ndsrctotal = 0;
		gidsrc = new int[src2send_cnt];
		ndsrc = new int[src2send_cnt];
		NrnHashIterateKeyValue(Int2DblList, src2send, int, gid, DblList*, dl) {
			ndsrctotal += dl->count();
		}}}		
		tssrc = new double[ndsrctotal];
	}
	int* off = new int[nrnmpi_numprocs+1]; // offsets for gidsrc and ndsrc
	int* cnts = new int[nrnmpi_numprocs];  // dest host counts for gidsrc and ndsrc
	int* doff = new int[nrnmpi_numprocs+1]; // offsets for doubles
	int* dcnts = new int[nrnmpi_numprocs+1]; // dest counts of the doubles
	for (i=0; i < nrnmpi_numprocs; ++i) {
		cnts[i] = 0; dcnts[i] = 0;
	}
	// counts to send to each destination rank
	NrnHashIterateKeyValue(Int2DblList, src2send, int, gid, DblList*, dl) {
		int host = gid%nrnmpi_numprocs;
		++cnts[host];
		dcnts[host] += dl->count();
	}}}
	// offsets
	off[0] = 0; doff[0] = 0;
	for (i=0; i < nrnmpi_numprocs; ++i) {
		off[i+1] = off[i] + cnts[i];
		doff[i+1] = doff[i] + dcnts[i];
	}
	// what to send to each destination. Note dcnts and ndsrc are NOT the same.
	for (i=0; i < nrnmpi_numprocs; ++i) { cnts[i] = 0; dcnts[i] = 0; }
	NrnHashIterateKeyValue(Int2DblList, src2send, int, gid, DblList*, dl) {
		host = gid%nrnmpi_numprocs;
		gidsrc[off[host] + cnts[host]] = gid;
		ndsrc[off[host] + cnts[host]++] = dl->count();
		for (i=0; i < dl->count(); ++i) {
			tssrc[doff[host] + dcnts[host]++] = dl->item(i);
		}
	}}}
	NrnHashIterateKeyValue(Int2DblList, src2send, int, gid, DblList*, dl) {
		delete dl;
	}}}
	delete src2send;

	if (nrnmpi_numprocs > 1) {
		all2allv_int2(cnts, off, gidsrc, ndsrc);
		all2allv_dbl1(dcnts, doff, tssrc);
		if (gidsrc) {
			delete [] gidsrc;
			delete [] ndsrc;
			delete [] tssrc;
		}
	}else{
		giddest_size = cnts[0];
		giddest = gidsrc;
		tsdest_cnts = ndsrc;
		tsdest = tssrc;
	}
	delete [] cnts;
	delete [] off;
	delete [] dcnts;
	delete [] doff;
}

static void allgatherv_helper(int cnt, int* rcnt, int* rdspl) {
		nrnmpi_int_allgather(&cnt, rcnt, 1);
		rdspl[0] = 0;
		for (int i=0; i < nrnmpi_numprocs; ++i) {
			rdspl[i+1] = rdspl[i] + rcnt[i];
		}
}

static void spikes_on_correct_host(int cnt, int* g, int* dcnts, int tscnt, double* ts, Int2DblList* m) {
	// tscnt is the sum of the dcnts.
	// i.e. the send times and undelivered NetCon counts.
	int i, rsize, *rg=NULL, *rtscnts=NULL;
	double *rts=NULL;
	// not at all space efficient
	if (nrnmpi_numprocs > 1) {

		int* cnts = new int[nrnmpi_numprocs];
		int* dspl = new int[nrnmpi_numprocs+1];
		allgatherv_helper(cnt, cnts, dspl);
		rsize = dspl[nrnmpi_numprocs];
		if (rsize) {
			rg = new int[rsize];
			rtscnts = new int[rsize];
		}
		nrnmpi_int_allgatherv(g, rg, cnts, dspl);
		nrnmpi_int_allgatherv(dcnts, rtscnts, cnts, dspl);

		allgatherv_helper(tscnt, cnts, dspl);
		rts = new double[dspl[nrnmpi_numprocs]];
		nrnmpi_dbl_allgatherv(ts, rts, cnts, dspl);

		delete [] cnts;
		delete [] dspl;
	}else{
		rsize = cnt;
		rg = g;
		rtscnts = dcnts;
		rts = ts;
	}
	int tsoff = 0;
	for (i = 0; i < rsize; ++i) {
		if (nrn_gid_exists(rg[i])) {
			DblList* dl = new DblList(rtscnts[i]);
			(*m)[rg[i]] = dl;
			for (int j=0; j < rtscnts[i]; ++j) {
				dl->append(rts[tsoff + j]);			
			}
		}
		tsoff += rtscnts[i];
	}
	if (nrnmpi_numprocs > 1) {
        if(rg)
            delete [] rg;
        if(rtscnts)
            delete [] rtscnts;
        if(rts)
            delete [] rts;
	}
}

static void construct_presyn_queue() {
	// almost all the old mk_presyn_info factored into here since
	// a presyn_queue is optionally needed for check_queue()

	// note that undelivered netcon count is interleaved with tsend
	// in the DblList of the Int2DblList
	int gid, tscnt, i; double ts, tt;
	DblList* dl;
		if (presyn_queue) { del_presyn_info(); }
		nc2dblist = new NetCon2DblList(20);
		scatteritems();
		int cnt = giddest_size;
		Int2DblList* m = new Int2DblList(cnt+1);
		int mcnt = 0;
		int mdcnt = 0;
		int its = 0;
		for (i=0; i < cnt; ++i) {
			int gid = giddest[i];
			tscnt = tsdest_cnts[i];
			if (m->find(gid, dl)) {
				// in the days when there could only be one
				// initiation time for a gid.
				//assert( fabs(tt - ts) < 1e-12 );
			}else{
				dl = new DblList(2);
				(*m)[gid] = dl;
				++mcnt;
			}
			// add the initiation time(s) if they are unique ( > .1)
			// and also accumulate the undelivered netcon counts
			// otherwise assert if not identical ( > 1e-12)
			for (int k = 0; k < tscnt; k += 2) {
				double t1 = tsdest[its++];
				int inccnt = tsdest[its++];
				int add = 1;
				// can't hurt to test the ones just added
				// no thought given to efficiency
				// Actually, need to test to accumulate
				for (int j=0; j < dl->count(); j += 2) {
					double t2 = dl->item(j);
					double dt = fabs(dl->item(j) - t1);
					if (dt < 1e-12) {
						dl->item_ref(j+1) += inccnt;
						add = 0;
						break;
					}else if (dt < .1) {
						assert(0);
					}
				}
				if (add) {
					dl->append(t1);
					dl->append(inccnt);
					mdcnt += 2;
				}
			}
		}
		if (giddest) {
			delete [] giddest;
			delete [] tsdest_cnts;
			delete [] tsdest;
		}
	// each host now has a portion of the (gid, ts) spike pairs
	// (Actually (gid, list of (ts, undelivered NetCon count)) map.)
	// but the pairs are not on the hosts that own the gid.
	// The major thing that is accomplished so far is that the
	// up to thousands of Netcon events on the queues of thousands
	// of host are now a single PreSyn event on a single host.
	// We could save them all in separate area and read them all
	// and send only when a host owns the gid, but we wish
	// to keep the spike sent info with the cell info for the gid.
	// We next need to do something analogous to the allgather send
	// so each host can examine every spike and pick out the ones
	// which were sent from that host. That becomes the true
	// presyn_queue map.
		int* gidsrc = 0;
		int* tssrc_cnt = 0;
		double* tssrc = 0;
		if (mcnt) {
			gidsrc = new int[mcnt];
			tssrc_cnt = new int[mcnt];
			tssrc = new double[mdcnt];
		}
		mcnt = 0;
		mdcnt = 0;
		NrnHashIterateKeyValue(Int2DblList, m, int, gid, DblList*, dl) {
			gidsrc[mcnt] = gid;
			tssrc_cnt[mcnt] = dl->count();
			for (int i=0; i < dl->count(); ++i) {
				tssrc[mdcnt++] = dl->item(i);
			}
			++mcnt;
			delete dl;
		}}}
		delete m;
		presyn_queue = m = new Int2DblList(127);
		spikes_on_correct_host(mcnt, gidsrc, tssrc_cnt, mdcnt, tssrc, m);
		if (gidsrc) {
			delete [] gidsrc;
			delete [] tssrc_cnt;
			delete [] tssrc;
		}
}

void BBSaveState::mk_presyn_info() { //also the NetCon* to tdelivery map
	if (f->type() != BBSS_IO::IN) { //only when saving or counting
		// if event on queue at t we will not be able to decide
		// whether or not it should be delivered during restore.
		TQueue* tq = net_cvode_instance_event_queue(nrn_threads);

		TQItem* tqi = tq->least();
		int dtype = tqi?((DiscreteEvent*)(tqi->data_))->type():0;
		assert(tq->least_t() > nrn_threads->_t || dtype==NetParEventType);
		construct_presyn_queue();
	}
}

void BBSaveState::possible_presyn(int gid) {
	if (debug) { sprintf(dbuf, "Enter possible_presyn()"); PDEBUG; }
	char buf[100];
	int i; double ts;
	// first the presyn state associated with this gid
	if (nrn_gid_exists(gid) < 2) {
		if (f->type() == BBSS_IO::IN) {
			// if reading then skip what was written
			i = 0;
			f->i(i);
			if (i == 1) { // skip state
				int j;
				double x;
sprintf(buf, "PreSyn");
f->s(buf, 1);
				f->i(j);
				f->d(1, x);
			}
		}else{
			i = -1;
			f->i(i);
		}
	}else{
		PreSyn* ps = nrn_gid2presyn(gid);
		i = (ps->ssrc_ != 0 ? 1 : -1); // does it have state
		f->i(i, 1);
		int output_index = ps->output_index_;
		f->i(output_index);
		if (output_index >= 0 && i == 1) {
sprintf(buf, "PreSyn");
f->s(buf, 1);
			int j = (ps->flag_ ? 1 : 0);
			double th = ps->valthresh_;
			f->i(j);
			f->d(1, th );
			if( ps->output_index_ >= 0 ) {
				ps->flag_ = j;
				ps->valthresh_ = th;
			}
#if 0
			f->d(1, ps->valold_);
			f->d(1, ps->told_);
			// ps->qthresh_ handling unimplemented but would not be needed
			// unless cvode.condition_order(2) when cvode active.
#endif
		}
	}
	// next the possibility that a spike derived from this presyn
	// is on the queue.
	if (f->type() != BBSS_IO::IN) { //save or count
		DblList* dl;
		if (presyn_queue->find(gid, dl)) {
			f->i(gid);
			i = dl->count(); // ts and undelivered netcon counts
			f->i(i);
			for (i=0; i < dl->count(); i += 2) {
				ts = dl->item(i);
				f->d(1, ts);
				int unc =  dl->item(i+1);
				f->i(unc);
			}
		}else{
			i = -1;
			f->i(i);
		}
	}else{ // restore
		f->i(i); // gid
		if (i >= 0) {
			int cnt, unc;
			//assert (gid == i);
                   if( gid == i ) {
			f->i(cnt); // both the ts and undelivered netcon counts

			//while re-issuing the PreSyn send, we do not want
			//any spike recording to take place. So, what are
			//the current Vector sizes, if used, so we can put
			//them back. This presumes we are recording only
			//from PreSyn with an output_gid.
#if 1 // if set to 0 comment out asserts below
			// The only reason we save here is to allow a
			// assertion test when restoring the original
			// record vector size.
			int sz1 = -1; int sz2 = -1;
			PreSyn* ps = nrn_gid2presyn(gid);
			if (ps->tvec_) {
				sz1 = ps->tvec_->capacity();
			}
			if (ps->idvec_) {
				sz2 = ps->idvec_->capacity();
			}
#endif

#if QUEUECHECK
			// map from gid to unc for later checking
			if (!queuecheck_gid2unc) {
				queuecheck_gid2unc = new Int2DblList(1000);
			}
			DblList* dl = new DblList(cnt);
			(*queuecheck_gid2unc)[i] = dl;
#endif
			for (int j=0; j< cnt; j += 2) {
				f->d(1, ts);
				if (nrn_use_bin_queue_ == 1) {
					ts = binq_time(ts);
				}
				f->i(unc); // expected undelivered NetCon count
				nrn_fake_fire(gid, ts, 2); // only owned by this
#if QUEUECHECK
				dl->append(ts);
				dl->append(unc);
#endif
			}

			// restore spike record sizes.
			if (ps->tvec_) {
				int sz = ps->tvec_->capacity() - cnt/2;
				assert(sz == sz1);
				ps->tvec_->resize(sz);
			}
			if (ps->idvec_) {
				int sz = ps->idvec_->capacity() - cnt/2;
				assert(sz == sz2);
				ps->idvec_->resize(sz);
			}
                    } else {
                        //skip -> file has undelivered spikes, but this is not the cell that deals with them 
                        f->i(cnt);
                         for (int j=0; j< cnt; j += 2) {
                             f->d(1, ts);
                             f->i(unc);
                         }
                    }
		}
	}
	if (debug) { sprintf(dbuf, "Leave possible_presyn()"); PDEBUG; }
}

#if QUEUECHECK
static void bbss_queuecheck() {
	construct_presyn_queue();
	NrnHashIterateKeyValue(Int2DblList, queuecheck_gid2unc, int, gid, DblList*, dl) {
		DblList* dl2;
		if (presyn_queue->find(gid, dl2)) {
			if (dl->count() == dl2->count()) {
				for (int i=0; i < dl->count(); i += 2) {
if ((fabs(dl->item(i) - dl2->item(i)) > 1e-12) || dl->item(i+1) != dl2->item(i+1)) {
printf("error: gid=%d expect t=%g %d but queue contains t=%g %d  tdiff=%g\n",
gid, dl->item(i), int(dl->item(i+1)), dl2->item(i), int(dl2->item(i+1)), dl->item(i) - dl2->item(i));
}
				}
			}else{
printf("error: gid=%d distinct delivery times, expect %ld, actual %ld\n",
gid, dl->count()/2, dl2->count()/2);
			}
		}else{
printf("error: gid=%d expect spikes but none on queue\n", gid);
			for (int i=0; i < dl->count()-1; i += 2) {
				printf("   %g %d", dl->item(i), int(dl->item(i+1)));
			}
			printf("\n");
		}
	}}}	
	NrnHashIterateKeyValue(Int2DblList, presyn_queue, int, gid, DblList*, dl2) {
		DblList* dl;
		if (!presyn_queue->find(gid, dl)) {
printf("error: gid=%d expect no spikes but some on queue\n", gid);
			for (int i=0; i < dl2->count()-1; i += 2) {
				printf("   %g %d", dl->item(i), int(dl->item(i+1)));
			}
			printf("\n");
		}
	}}}	
#if 0
	printf("undelivered netcons\n");
	NrnHashIterateKeyValue(Int2DblList, queuecheck_gid2unc, int, gid, DblList*, dl) {
		printf("%d", gid);
		for (int i=0; i < dl->count()-1; i += 2) {
			printf("   %g %d", dl->item(i), int(dl->item(i+1)));
		}
		printf("\n");
	}}}	
#endif
#if 0
	printf("presyn_queue\n");
	NrnHashIterateKeyValue(Int2DblList, presyn_queue, int, gid, DblList*, dl) {
		printf("%d", gid);
		for (int i=0; i < dl->count()-1; i += 2) {
			printf("   %g %d", dl->item(i), int(dl->item(i+1)));
		}
		printf("\n");
	}}}
#endif
	//cleanup
	NrnHashIterateKeyValue(Int2DblList, queuecheck_gid2unc, int, gid, DblList*, dl) {
		delete dl;
	}}}
	delete queuecheck_gid2unc;
	queuecheck_gid2unc = 0;
	del_presyn_info();
}
#endif /*QUEUECHECK*/

