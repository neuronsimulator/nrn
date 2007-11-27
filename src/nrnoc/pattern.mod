: The spikeout pairs (t, gid) resulting from a parallel network simulation
: can become the stimulus for any single cpu subnet as long as the gid's are
: consistent.
: Note: hoc must retain references to the tvec and gidvec vectors
: to prevent the Info from going out of existence

NEURON {
	ARTIFICIAL_CELL PatternStim
	RANGE fake_output
	POINTER ptr
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

extern int ifarg(int iarg);
extern double* vector_vec(void* vv);
extern int vector_capacity(void* vv);
extern void* vector_arg(int iarg);
extern void nrn_fake_fire(int gid, double spiketime, int fake_out);

typedef struct {
	void* tvec;
	void* gidvec;
	int index;
} Info;

#define INFOCAST Info** ip = (Info**)(&(_p_ptr))

ENDVERBATIM


CONSTRUCTOR {
VERBATIM {
	INFOCAST;
	Info* info = (Info*)hoc_Emalloc(sizeof(Info)); hoc_malchk();
	*ip = info;
	info->tvec = (void*)0;
	info->gidvec = (void*)0;
	info->index = 0;
}
ENDVERBATIM
}

DESTRUCTOR {
VERBATIM {
	INFOCAST; Info* info = *ip;
	free(info);
}
ENDVERBATIM
}

FUNCTION initps() {
VERBATIM {
	INFOCAST; Info* info = *ip;
	info->index = 0;
	if (info->tvec) {
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
	int size = vector_capacity(info->tvec);
	int fake_out;
	double* tvec = vector_vec(info->tvec);
	double* gidvec = vector_vec(info->gidvec);
	int i;
	fake_out = fake_output ? 1 : 0;
	for (i=0; info->index < size; ++i) {
		/* only if the gid is NOT on this machine */
		nrn_fake_fire((int)gidvec[info->index], tvec[info->index], fake_out);
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

PROCEDURE play() {
VERBATIM {
	INFOCAST; Info* info = *ip;
	if (ifarg(1)) {
		info->tvec = vector_arg(1);
		info->gidvec = vector_arg(2);
	}else{
		info->tvec = (void*)0;
		info->gidvec = (void*)0;
	}
}
ENDVERBATIM
}
        

