: The spikeout pairs (t, gid) resulting from a parallel network simulation
: can become the stimulus for any single cpu subnet as long as the gid's are
: consistent.
: Note: hoc need not retain references to the tvec and gidvec vectors
: as Info makes a copy of those, double for tvec, int for gidvec.

NEURON {
    ARTIFICIAL_CELL PatternStim
    THREADSAFE
    RANGE fake_output
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
/* Changed Info definition to correspond to that of CoreNEURON pattern.mod
   in order to reduce memory requirements of separate copies of what
   used to here be an IvocVect of doubles for gidvec but in CoreNEURON was
   an int*. Also it obviates the copying of index back and forth.
   The user differnce here is that both tvec and gidvec are copies (double
   and int) of the passed in Vector. That is bad, but the user does not
   have to retain a reference to those Vectors so that memory can be
   potentially freed. On the other hand, it is now a simple matter to
   implement reading the Info directly from a file.
*/
struct Info { /* same as in CoreNEURON */
    int size;
    double* tvec;
    int* gidvec;
    int index;
};
Info*& as_info(double*& vptr) { return reinterpret_cast<Info*&>(vptr); }
ENDVERBATIM


CONSTRUCTOR {
VERBATIM {
    auto& info = as_info(_p_ptr);
    assert(!info);
    info = new Info{};
    info->size = 0;
    info->tvec = nullptr;
    info->gidvec = nullptr;
    info->index = 0;
}
ENDVERBATIM
}

DESTRUCTOR {
VERBATIM {
    auto& info = as_info(_p_ptr);
    if (info->size > 0) {
        delete[] info->tvec;
        delete[] info->gidvec;
    }
    delete info;
    info = nullptr;
}
ENDVERBATIM
}

FUNCTION initps() {
VERBATIM {
    auto& info = as_info(_p_ptr);
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
    auto& info = as_info(_p_ptr);
    int size = info->size;
    double* tvec = info->tvec;
    int* gidvec = info->gidvec;
    int fake_out = fake_output ? 1 : 0;
    for (int i=0; info->index < size; ++i) {
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

PROCEDURE play() {
VERBATIM {
    auto& info = as_info(_p_ptr);
    if (info->size > 0) {
        delete[] info->tvec;
        delete[] info->gidvec;
        info->tvec = nullptr;
        info->gidvec = nullptr;
        info->size = 0;
    }

    if (ifarg(1)) {
        int _i;
        auto* tvec = vector_arg(1);
        auto* gidvec = vector_arg(2);
        int size = vector_capacity(tvec);
        double* tdata = vector_vec(tvec);
        double* giddata = vector_vec(gidvec);

        info->size = size;
        info->tvec = new double[size];
        info->gidvec = new int[size];
        for (_i = 0; _i < size; ++_i) {
            info->tvec[_i] = tdata[_i];
            info->gidvec[_i] = (int)giddata[_i];
        }
    }
}
ENDVERBATIM
}
        
VERBATIM

static void bbcore_write(double* x, int* d, int* xx, int *offset, _threadargsproto_){}
static void bbcore_read(double* x, int* d, int* xx, int* offset, _threadargsproto_){}

Info* nrn_patternstim_info_ref(Datum* _ppvar) {
    // CoreNEURON PatternStim will use this Info*
    return as_info(_p_ptr);
}
ENDVERBATIM
