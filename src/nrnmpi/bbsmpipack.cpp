#include <../../nrnconf.h>
#include <stdio.h>
#include <stdlib.h>

/* do not want the redef in the dynamic load case */
#include <nrnmpiuse.h>

#if NRNMPI_DYNAMICLOAD
#include <nrnmpi_dynam.h>
#endif

#include <nrnmpi.h>

#if NRNMPI
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <mpi.h>
#include <nrnmpidec.h>
#include <nrnmpi_impl.h>
#include <hocdec.h>

#define nrn_mpi_assert(arg) nrn_assert(arg == MPI_SUCCESS)

#define nrnmpidebugleak 0
#define debug           0

extern MPI_Comm nrn_bbs_comm;

#if nrnmpidebugleak
static int nrnmpi_bufcnt_;
#endif

/* we want to find the key easily and I am assuming that all MPI
 implementations allow one to start unpacking from a particular position.
 Therefore we give space for the key position at the beginning and
 an int always takes up the same space at position 0.
 Now I regret not forcing the key to come first at the user level.
*/

#define my_MPI_INT    0
#define my_MPI_DOUBLE 1
#define my_MPI_CHAR   2
#define my_MPI_PACKED 3
#define my_MPI_PICKLE 4

static MPI_Datatype mytypes[] = {MPI_INT, MPI_DOUBLE, MPI_CHAR, MPI_PACKED, MPI_CHAR};

static void unpack(void* buf, int count, int my_datatype, bbsmpibuf* r, const char* errmes) {
    int type[2];
    assert(r && r->buf);
#if debug
    printf("%d unpack upkpos=%d pkposition=%d keypos=%d size=%d\n",
           nrnmpi_myid_bbs,
           r->upkpos,
           r->pkposition,
           r->keypos,
           r->size);
#endif
    assert(r->upkpos >= 0 && r->size >= r->upkpos);
    nrn_mpi_assert(MPI_Unpack(r->buf, r->size, &r->upkpos, type, 2, MPI_INT, nrn_bbs_comm));
#if debug
    printf("%d unpack r=%p size=%d upkpos=%d type[0]=%d datatype=%d  type[1]=%d  count=%d\n",
           nrnmpi_myid_bbs,
           r,
           r->size,
           r->upkpos,
           type[0],
           my_datatype,
           type[1],
           count);
#endif
    if (type[0] != my_datatype || type[1] != count) {
        printf("%d unpack size=%d upkpos=%d type[0]=%d   datatype=%d  type[1]=%d  count=%d\n",
               nrnmpi_myid_bbs,
               r->size,
               r->upkpos,
               type[0],
               my_datatype,
               type[1],
               count);
    }
    assert(type[0] == my_datatype);
    assert(type[1] == count);
    nrn_mpi_assert(
        MPI_Unpack(r->buf, r->size, &r->upkpos, buf, count, mytypes[my_datatype], nrn_bbs_comm));
}

void nrnmpi_upkbegin(bbsmpibuf* r) {
    int type;
    int p;
#if debug
    printf("%d nrnmpi_upkbegin %p (preunpack upkpos=%d keypos=%d)\n",
           nrnmpi_myid_bbs,
           r,
           r->upkpos,
           r->keypos);
#endif
    assert(r && r->buf && r->size > 0);
    if (nrnmpi_myid_bbs == -1) {
        hoc_execerror("subworld process with nhost > 0 cannot use", "the bulletin board");
    }
    r->upkpos = 0;
    nrn_mpi_assert(MPI_Unpack(r->buf, r->size, &r->upkpos, &p, 1, MPI_INT, nrn_bbs_comm));
    if (p > r->size) {
        printf("\n %d nrnmpi_upkbegin keypos=%d size=%d\n", nrnmpi_myid_bbs, p, r->size);
    }
    assert(p <= r->size);
    nrn_mpi_assert(MPI_Unpack(r->buf, r->size, &p, &type, 1, MPI_INT, nrn_bbs_comm));
#if debug
    printf("%d nrnmpi_upkbegin type=%d keypos=%d\n", nrnmpi_myid_bbs, type, p);
#endif
    assert(type == 0);
    r->keypos = p;
}

char* nrnmpi_getkey(bbsmpibuf* r) {
    char* s;
    int type;
    type = r->upkpos;
    r->upkpos = r->keypos;
#if debug
    printf("%d nrnmpi_getkey %p keypos=%d\n", nrnmpi_myid_bbs, r, r->keypos);
#endif
    s = nrnmpi_upkstr(r);
    assert(r->pkposition == 0 || r->pkposition == r->upkpos);
    r->pkposition = r->upkpos;
    r->upkpos = type;
#if debug
    printf("getkey return %s\n", s);
#endif
    return s;
}

int nrnmpi_getid(bbsmpibuf* r) {
    int i, type;
    type = r->upkpos;
    r->upkpos = r->keypos;
#if debug
    printf("%d nrnmpi_getid %p keypos=%d\n", nrnmpi_myid_bbs, r, r->keypos);
#endif
    i = nrnmpi_upkint(r);
    r->upkpos = type;
#if debug
    printf("getid return %d\n", i);
#endif
    return i;
}

int nrnmpi_upkint(bbsmpibuf* r) {
    int i;
    unpack(&i, 1, my_MPI_INT, r, "upkint");
    return i;
}

double nrnmpi_upkdouble(bbsmpibuf* r) {
    double x;
    unpack(&x, 1, my_MPI_DOUBLE, r, "upkdouble");
    return x;
}

void nrnmpi_upkvec(int n, double* x, bbsmpibuf* r) {
    unpack(x, n, my_MPI_DOUBLE, r, "upkvec");
}

#if NRNMPI_DYNAMICLOAD
/* for some unknown reason mpiexec -n 2 python3 test0.py gives
load_nrnmpi: /home/hines/neuron/nrndynam/x86_64/lib/libnrnmpi.so: undefined symbol: cxx_char_alloc
So fill this in explicitly in nrnmpi_dynam.cpp
*/
char* (*p_cxx_char_alloc)(int len);
#endif

char* nrnmpi_upkstr(bbsmpibuf* r) {
    int len;
    char* s;
    unpack(&len, 1, my_MPI_INT, r, "upkstr length");
#if NRNMPI_DYNAMICLOAD
    s = (*p_cxx_char_alloc)(len + 1); /* will be delete not free */
#else
    s = cxx_char_alloc(len + 1); /* will be delete not free */
#endif
    unpack(s, len, my_MPI_CHAR, r, "upkstr string");
    s[len] = '\0';
    return s;
}

char* nrnmpi_upkpickle(size_t* size, bbsmpibuf* r) {
    int len;
    char* s;
    unpack(&len, 1, my_MPI_INT, r, "upkpickle length");
    *size = len;
#if NRNMPI_DYNAMICLOAD
    s = (*p_cxx_char_alloc)(len + 1); /* will be delete not free */
#else
    s = cxx_char_alloc(len + 1); /* will be delete, not free */
#endif
    unpack(s, len, my_MPI_PICKLE, r, "upkpickle data");
    return s;
}

static void resize(bbsmpibuf* r, int size) {
    int newsize;
    if (r->size < size) {
        newsize = (size / 64) * 64 + 128;
        r->buf = static_cast<char*>(hoc_Erealloc(r->buf, newsize));
        hoc_malchk();
        r->size = newsize;
    }
}

void nrnmpi_pkbegin(bbsmpibuf* r) {
    int type;
    if (nrnmpi_myid_bbs == -1) {
        hoc_execerror("subworld process with nhost > 0 cannot use", "the bulletin board");
    }
    r->pkposition = 0;
    type = 0;
#if debug
    printf(
        "%d nrnmpi_pkbegin %p size=%d pkposition=%d\n", nrnmpi_myid_bbs, r, r->size, r->pkposition);
#endif
    nrn_mpi_assert(MPI_Pack(&type, 1, MPI_INT, r->buf, r->size, &r->pkposition, nrn_bbs_comm));
}

void nrnmpi_enddata(bbsmpibuf* r) {
    int p, type, isize, oldsize;
    p = r->pkposition;
    type = 0;
#if debug
    printf("%d nrnmpi_enddata %p size=%d pkposition=%d\n", nrnmpi_myid_bbs, r, r->size, p);
#endif
    nrn_mpi_assert(MPI_Pack_size(1, MPI_INT, nrn_bbs_comm, &isize));
    oldsize = r->size;
    resize(r, r->pkposition + isize);
#if debug
    if (oldsize < r->pkposition + isize) {
        printf("%d %p need %d more. end up with total of %d\n", nrnmpi_myid_bbs, r, isize, r->size);
    }
#endif
    nrn_mpi_assert(MPI_Pack(&type, 1, MPI_INT, r->buf, r->size, &r->pkposition, nrn_bbs_comm));
#if debug
    printf("%d nrnmpi_enddata buf=%p size=%d pkposition=%d\n",
           nrnmpi_myid_bbs,
           r->buf,
           r->size,
           r->pkposition);
#endif
    nrn_mpi_assert(MPI_Pack(&p, 1, MPI_INT, r->buf, r->size, &type, nrn_bbs_comm));
#if debug
    printf("%d after nrnmpi_enddata, %d was packed at beginning and 0 was packed before %d\n",
           nrnmpi_myid_bbs,
           p,
           r->pkposition);
#endif
}

static void pack(void* inbuf, int incount, int my_datatype, bbsmpibuf* r, const char* e) {
    int type[2];
    int dsize, isize, oldsize;
#if debug
    printf("%d pack %p count=%d type=%d outbuf-%p pkposition=%d %s\n",
           nrnmpi_myid_bbs,
           r,
           incount,
           my_datatype,
           r->buf,
           r->pkposition,
           e);
#endif
    nrn_mpi_assert(MPI_Pack_size(incount, mytypes[my_datatype], nrn_bbs_comm, &dsize));
    nrn_mpi_assert(MPI_Pack_size(2, MPI_INT, nrn_bbs_comm, &isize));
    oldsize = r->size;
    resize(r, r->pkposition + dsize + isize);
#if debug
    if (oldsize < r->pkposition + dsize + isize) {
        printf("%d %p need %d more. end up with total of %d\n",
               nrnmpi_myid_bbs,
               r,
               dsize + isize,
               r->size);
    }
#endif
    type[0] = my_datatype;
    type[1] = incount;
    nrn_mpi_assert(MPI_Pack(type, 2, MPI_INT, r->buf, r->size, &r->pkposition, nrn_bbs_comm));
    nrn_mpi_assert(MPI_Pack(
        inbuf, incount, mytypes[my_datatype], r->buf, r->size, &r->pkposition, nrn_bbs_comm));
#if debug
    printf("%d pack done pkposition=%d\n", nrnmpi_myid_bbs, r->pkposition);
#endif
}

void nrnmpi_pkint(int i, bbsmpibuf* r) {
    int ii;
    ii = i;
    pack(&ii, 1, my_MPI_INT, r, "pkint");
}

void nrnmpi_pkdouble(double x, bbsmpibuf* r) {
    double xx;
    xx = x;
    pack(&xx, 1, my_MPI_DOUBLE, r, "pkdouble");
}

void nrnmpi_pkvec(int n, double* x, bbsmpibuf* r) {
    pack(x, n, my_MPI_DOUBLE, r, "pkvec");
}

void nrnmpi_pkstr(const char* s, bbsmpibuf* r) {
    int len;
    len = strlen(s);
    pack(&len, 1, my_MPI_INT, r, "pkstr length");
    pack((char*) s, len, my_MPI_CHAR, r, "pkstr string");
}

void nrnmpi_pkpickle(const char* s, size_t size, bbsmpibuf* r) {
    int len = size;
    pack(&len, 1, my_MPI_INT, r, "pkpickle length");
    pack((char*) s, len, my_MPI_PICKLE, r, "pkpickle data");
}

void nrnmpi_bbssend(int dest, int tag, bbsmpibuf* r) {
#if debug
    printf("%d nrnmpi_bbssend %p dest=%d tag=%d size=%d\n",
           nrnmpi_myid_bbs,
           r,
           dest,
           tag,
           (r) ? r->size : 0);
#endif

    /* Some MPI implementations limit tags to be less than full MPI_INT domain
       so pack tag if > 20 in second slot (now reserved) of buffer and use 20 as the tag.
    */
    if (tag > 20) {
        int save_position = r->pkposition;
        int save_upkpos = r->upkpos;
        nrnmpi_upkbegin(r);
        nrnmpi_upkint(r);
        r->pkposition = r->upkpos;
        nrnmpi_pkint(tag, r);
        r->pkposition = save_position;
        r->upkpos = save_upkpos;
        tag = 20;
    }

    if (r) {
        assert(r->buf && r->keypos <= r->size);
        nrn_mpi_assert(MPI_Send(r->buf, r->size, MPI_PACKED, dest, tag, nrn_bbs_comm));
    } else {
        nrn_mpi_assert(MPI_Send(nullptr, 0, MPI_PACKED, dest, tag, nrn_bbs_comm));
    }
    errno = 0;
#if debug
    printf("%d return from send\n", nrnmpi_myid_bbs);
#endif
}

int nrnmpi_bbsrecv(int source, bbsmpibuf* r) {
    MPI_Status status;
    int size;
    if (source == -1) {
        source = MPI_ANY_SOURCE;
    }
#if debug
    printf("%d nrnmpi_bbsrecv %p\n", nrnmpi_myid_bbs, r);
#endif
    nrn_mpi_assert(MPI_Probe(source, MPI_ANY_TAG, nrn_bbs_comm, &status));
    nrn_mpi_assert(MPI_Get_count(&status, MPI_PACKED, &size));
#if debug
    printf("%d nrnmpi_bbsrecv probe size=%d source=%d tag=%d\n",
           nrnmpi_myid_bbs,
           size,
           status.MPI_SOURCE,
           status.MPI_TAG);
#endif
    resize(r, size);
    nrn_mpi_assert(
        MPI_Recv(r->buf, r->size, MPI_PACKED, source, MPI_ANY_TAG, nrn_bbs_comm, &status));
    errno = 0;
    /* Some MPI implementations limit tags to be less than full MPI_INT domain
       In the past we allowed  TODO mesages to have tags > 20 (FIRSTID of src/parallel/bbssrv.h)
       To fix the bug we no longer send or receive such tags and instead
       copy the tag into the second pkint of the r->buf and send with
       a tag of 20.
    */
    if (status.MPI_TAG == 20) {
        int tag;
        int save_upkpos = r->upkpos; /* possibly not needed */
        nrnmpi_upkbegin(r);
        nrnmpi_upkint(r);
        tag = nrnmpi_upkint(r);
        r->upkpos = save_upkpos;
        return tag;
    }
    return status.MPI_TAG;
}

int nrnmpi_bbssendrecv(int dest, int tag, bbsmpibuf* s, bbsmpibuf* r) {
    int size, itag, source;
    int msgtag;
    MPI_Status status;
#if debug
    printf("%d nrnmpi_bbssendrecv dest=%d tag=%d\n", nrnmpi_myid_bbs, dest, tag);
#endif
    if (!nrnmpi_iprobe(&size, &itag, &source) || source != dest) {
#if debug
        printf("%d nrnmpi_bbssendrecv nothing available so send\n", nrnmpi_myid_bbs);
#endif
        nrnmpi_bbssend(dest, tag, s);
    }
    return nrnmpi_bbsrecv(dest, r);
}

int nrnmpi_iprobe(int* size, int* tag, int* source) {
    int flag = 0;
    MPI_Status status;
    nrn_mpi_assert(MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, nrn_bbs_comm, &flag, &status));
    if (flag) {
        if (source)
            *source = status.MPI_SOURCE;
        if (tag)
            *tag = status.MPI_TAG;
        if (size)
            nrn_mpi_assert(MPI_Get_count(&status, MPI_PACKED, size));
    }
    return flag;
}

void nrnmpi_probe(int* size, int* tag, int* source) {
    int flag = 0;
    MPI_Status status;
    nrn_mpi_assert(MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, nrn_bbs_comm, &status));
    if (source)
        *source = status.MPI_SOURCE;
    if (tag)
        *tag = status.MPI_TAG;
    if (size)
        nrn_mpi_assert(MPI_Get_count(&status, MPI_PACKED, size));
}

bbsmpibuf* nrnmpi_newbuf(int size) {
    bbsmpibuf* buf;
    buf = (bbsmpibuf*) hoc_Emalloc(sizeof(bbsmpibuf));
    hoc_malchk();
#if debug
    printf("%d nrnmpi_newbuf %p\n", nrnmpi_myid_bbs, buf);
#endif
    buf->buf = (char*) 0;
    if (size > 0) {
        buf->buf = static_cast<char*>(hoc_Emalloc(size * sizeof(char)));
        hoc_malchk();
    }
    buf->size = size;
    buf->pkposition = 0;
    buf->upkpos = 0;
    buf->keypos = 0;
    buf->refcount = 0;
#if nrnmpidebugleak
    ++nrnmpi_bufcnt_;
#endif
    return buf;
}

void nrnmpi_copy(bbsmpibuf* dest, bbsmpibuf* src) {
    int i;
    resize(dest, src->size);
    for (i = 0; i < src->size; ++i) {
        dest->buf[i] = src->buf[i];
    }
    dest->pkposition = src->pkposition;
    dest->upkpos = src->upkpos;
    dest->keypos = src->keypos;
}

static void nrnmpi_free(bbsmpibuf* buf) {
#if debug
    printf("%d nrnmpi_free %p\n", nrnmpi_myid_bbs, buf);
#endif
    if (buf->buf) {
        free(buf->buf);
    }
    free(buf);
#if nrnmpidebugleak
    --nrnmpi_bufcnt_;
#endif
}

void nrnmpi_ref(bbsmpibuf* buf) {
    assert(buf);
    buf->refcount += 1;
}

void nrnmpi_unref(bbsmpibuf* buf) {
    if (buf) {
        --buf->refcount;
        if (buf->refcount <= 0) {
            nrnmpi_free(buf);
        }
    }
}

#if nrnmpidebugleak
void nrnmpi_checkbufleak() {
    if (nrnmpi_bufcnt_ > 0) {
        printf("%d nrnmpi_bufcnt=%d\n", nrnmpi_myid_bbs, nrnmpi_bufcnt_);
    }
}
#endif

#endif /*NRNMPI*/
