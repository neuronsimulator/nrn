: The spikeout pairs (t, gid) resulting from a parallel network simulation
: can become the stimulus for any single cpu subnet as long as the gid's are
: consistent.
: Note: hoc must retain references to the tvec and gidvec vectors
: to prevent the Info from going out of existence

NEURON {
	ARTIFICIAL_CELL PatternStim
	RANGE fake_output
	THREADSAFE
	BBCOREPOINTER ptr
}

PARAMETER {
	fake_output = 0
}

ASSIGNED {
	ptr
}

INITIAL {
	if (initps() > 0) { net_send(0, 1) }
}

NET_RECEIVE (w) {LOCAL nst
	if (flag == 1) {
		nst = sendgroup()
		if (nst >= t) {net_send(nst - t, 1)}
	}
}

VERBATIM

extern void nrn_fake_fire(int gid, double spiketime, int fake_out);

typedef struct {
	int size;
	double* tvec;
	int* gidvec;
	int index;
} Info;

#define INFOCAST Info** ip = (Info**)(&(_p_ptr))

ENDVERBATIM


VERBATIM
Info* mkinfo(_threadargsproto_) {
	INFOCAST;
	Info* info = (Info*)hoc_Emalloc(sizeof(Info)); hoc_malchk();
	info->size = 0;
	info->tvec = (double*)0;
	info->gidvec = (int*)0;
	info->index = 0;
	return info;
}
ENDVERBATIM

FUNCTION initps() {
VERBATIM {
	INFOCAST; Info* info = *ip;
	info->index = 0;
	if (info && info->tvec) {
		_linitps = 1.;
	}else{
		_linitps = 0.;
	}
}
ENDVERBATIM
}

FUNCTION sendgroup() {
VERBATIM {
	INFOCAST; Info* info = *ip;
	int size = info->size;
	int fake_out;
	double* tvec = info->tvec;
	int* gidvec = info->gidvec;
	int i;
	fake_out = fake_output ? 1 : 0;
	for (i=0; info->index < size; ++i) {
		/* only if the gid is NOT on this machine */
		nrn_fake_fire(gidvec[info->index], tvec[info->index], fake_out);
		++info->index;
		if (i > 100 && t < tvec[info->index]) { break; }
	}
	if (info->index >= size) {
		_lsendgroup = t - 1.;
	}else{
		_lsendgroup = tvec[info->index];
	}
}
ENDVERBATIM
}

VERBATIM
static void bbcore_write(double* x, int* d, int* xx, int *offset, _threadargsproto_){}
static void bbcore_read(double* x, int* d, int* xx, int* offset, _threadargsproto_){}

void pattern_stim_setup_helper(int size, double* tv, double* gv, _threadargsproto_) {
	INFOCAST;
	Info* info = mkinfo(_threadargs_);
	*ip = info;
	info->size = size;
	info->tvec = tv;
	info->gidvec = gv;
}

ENDVERBATIM

