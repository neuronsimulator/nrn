#include <../../nrnconf.h>
#include "nrnmpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <InterViews/resource.h>
#include "oc2iv.h"
#include "bbs.h"
#include "bbslocal.h"
#if defined(NRNMPI)
#include "bbsdirect.h"
#include "bbsrcli.h"
#endif

#if defined(HAVE_TMS) && !NRNMPI
#include <time.h>
#include <sys/times.h>
#include <limits.h>
static struct tms tmsbuf, tms_start_;
static clock_t starttime;
#endif

extern int nrn_global_argc;
extern char** nrn_global_argv;

bool BBSImpl::is_master_ = false;
bool BBSImpl::started_ = false;
bool BBSImpl::done_ = false;
bool BBSImpl::master_works_ = true;

#undef debug
#define debug BBSImpl::debug_

int BBSImpl::debug_ = 0;
int BBSImpl::mytid_;

static int etaskcnt;
static double total_exec_time;
static double worker_take_time;

BBS::BBS() {
    init(-1);
}

BBS::BBS(int n) {
    init(n);
}

#if NRNMPI
void BBS::init(int) {
    if (nrnmpi_use == 0) {
        BBSImpl::is_master_ = true;
        impl_ = new BBSLocal();
        return;
    }
    if (!BBSImpl::started_) {
        BBSImpl::is_master_ = (nrnmpi_myid_bbs == 0) ? true : false;
        BBSImpl::master_works_ = true;
    }
    // Just as with PVM which stored buffers on the bulletin board
    // so we have the following files to store MPI_PACKED buffers
    // on the bulletin board. It would be possible to factor out
    // the pvm stuff and we may do that later but for now we
    // start with copies of the four files that worked for PVM
    // and convert to the nrnmpi functions implemented in
    // ../nrnmpi
    // The four files are
    // bbsclimpi.cpp - mpi remote client BBSClient from bbsrcli.cpp
    // bbsdirectmpi.cpp - mpi master client BBSDirect from bbsdirect.cpp
    // bbssrvmpi.cpp - mpi server portion to remote client from master bbs
    //		BBSDirectServer derived from bbssrv.cpp
    // bbslsrvmpi.cpp - mpi master bbs portion of BBSDirectServer
    // 		derived from bbslsrv2.cpp
    // We reuse the .h files of these.
    if (BBSImpl::is_master_) {
        impl_ = new BBSDirect();
    } else {
        impl_ = new BBSClient();
    }
}
#else   // !NRNMPI
void BBS::init(int) {
    if (!BBSImpl::started_) {
        BBSImpl::is_master_ = true;
        BBSImpl::master_works_ = true;
    }
    impl_ = new BBSLocal();
}
#endif  // !NRNMPI

BBSImpl::BBSImpl() {
    runworker_called_ = 0;
    wait_time_ = 0.;
    send_time_ = 0.;
    integ_time_ = 0.;
    working_id_ = 0;
    n_ = 0;
    pickle_ret_ = 0;
    pickle_ret_size_ = 0;
}

BBS::~BBS() {
    delete impl_;
}

BBSImpl::~BBSImpl() {
    if (pickle_ret_) {
        delete[] pickle_ret_;
    }
}

bool BBS::is_master() {
    return BBSImpl::is_master_;
}

int BBS::nhost() {
    return nrnmpi_numprocs;
}

int BBS::myid() {
    return nrnmpi_myid;
}

bool BBSImpl::is_master() {
    return is_master_;
}

double BBS::time() {
    return impl_->time();
}

double BBSImpl::time() {
#if NRNMPI
    return nrnmpi_wtime();
#else
#ifdef HAVE_TMS
    return double(times(&tmsbuf)) / 100.;
#else
    return 0.;
#endif
#endif
}

double BBS::wait_time() {
    return impl_->wait_time_;
}
double BBS::integ_time() {
    return impl_->integ_time_;
}
double BBS::send_time() {
    return impl_->send_time_;
}
void BBS::add_wait_time(double st) {
    impl_->wait_time_ += impl_->time() - st;
}

void BBS::perror(const char* s) {
    impl_->perror(s);
}

void BBSImpl::perror(const char*) {}

int BBS::upkint() {
    int i = impl_->upkint();
    if (debug) {
        printf("upkint %d\n", i);
    }
    return i;
}

double BBS::upkdouble() {
    double d = impl_->upkdouble();
    if (debug) {
        printf("upkdouble %g\n", d);
    }
    return d;
}

void BBS::upkvec(int n, double* px) {
    impl_->upkvec(n, px);
    if (debug) {
        printf("upkvec %d\n", n);
    }
}

char* BBS::upkstr() {
    char* s = impl_->upkstr();
    if (debug) {
        printf("upkstr |%s|\n", s);
    }
    return s;
}

char* BBS::upkpickle(size_t* n) {
    char* s = impl_->upkpickle(n);
    if (debug) {
        printf("upkpickle %lu |%s|\n", *n, s);
    }
    return s;
}

void BBS::pkbegin() {
    if (debug) {
        printf("pkbegin\n");
    }
    impl_->pkbegin();
}

void BBS::pkint(int i) {
    if (debug) {
        printf("pkint %d\n", i);
    }
    impl_->pkint(i);
}

void BBS::pkdouble(double x) {
    if (debug) {
        printf("pkdouble %g\n", x);
    }
    impl_->pkdouble(x);
}

void BBS::pkvec(int n, double* px) {
    if (debug) {
        printf("pkdouble %d\n", n);
    }
    impl_->pkvec(n, px);
}

void BBS::pkstr(const char* s) {
    if (debug) {
        printf("pkstr |%s|\n", s);
    }
    impl_->pkstr(s);
}

void BBS::pkpickle(const char* s, size_t n) {
    if (debug) {
        printf("pkpickle %lu |%s|\n", n, s);
    }
    impl_->pkpickle(s, n);
}

#if 0
// for now all todo messages are of the three item form
// tid
// gid
// id
// "stmt"
// the latter should set hoc_ac_ if the return value is important
// right now every arg must be literal, we'll handle variables later.
// eg the following should work.
// n = 1000  x = 0  for i=1,n { x += i }  hoc_ac_ = x
// although it may makes sense to send the result directly to the
// tid for now we just put it back onto the mailbox in the form
// message: "result tid gid" with two items, i.e. id, hoc_ac_
// Modified 11/30/09. To allow a return of a (pickled) PythonObject
// for the case when the execution is for a Python Callable the return
// message result now has three items, i.e. id, rtype, hoc_ac or pickled
// PyObject. rtype = 0 means hoc_ac, rtype = 1 means pickled PyObject.
// execute_helper returns the pickle string or (char*)0.
#endif
#if 0
// the todo management has been considerably modified to support
// a priority queue style for the order in which tasks are executed.
// Now the server manages the task id. Given a task id, it knows the
// parent task id.
// The working_id is always non-trivial on worker machines, and is
// only 0 on the master when dealing with a submission at the top level
// post_todo(working_id_) message is the statement, here the working_id is
//	the parent of the future stmt task.
// take_todo: message is the statement. return is the id of this task
// post_result(id) message is return value.
// the result will be retrieved relative to the submitting working_id_
// look_take_result(working_id_) message is the return value. the return
// value of this call is the id of the task that computed the return.
#endif

// BBSImpl::execute_helper() in ocbbs.cpp

void BBSImpl::execute(int id) {  // assumes a "_todo" message in receive buffer
    ++etaskcnt;
    double st, et;
    int userid;
    char* rs;
    char* s;
    size_t n;
    int i;
    int save_id = working_id_;
    int save_n = n_;
    working_id_ = id;
    n_ = 0;
    st = time();
    if (debug_) {
        printf("execute begin %g: working_id_=%d\n", st, working_id_);
    }
    userid = upkint();
    int wid = upkint();
    hoc_ac_ = double(id);
    rs = execute_helper(&n, id);  // builds and execute hoc statement
    et = time() - st;
    total_exec_time += et;
    if (debug) {
        printf("execute end elapsed %g: working_id_=%d hoc_ac_=%g\n", et, working_id_, hoc_ac_);
    }
    pkbegin();
    pkint(userid);
    pkint(wid);
    pkint(rs ? 1 : 0);
    if (!rs) {
        pkdouble(hoc_ac_);
    } else {
        pkpickle(rs, n);
        delete[] rs;
    }
    working_id_ = save_id;
    n_ = save_n;
    post_result(id);
}

int BBS::submit(int userid) {
    return impl_->submit(userid);
}

int BBSImpl::submit(int userid) {
    // userid was the first item packed
    ++n_;
    if (debug) {
        printf("submit n_= %d for working_id=%d userid=%d\n", n_, working_id_, userid);
    }
    if (userid < 0) {
        save_args(userid);
    } else {
        post_todo(working_id_);
    }
    return userid;
}

void BBS::context() {
    impl_->context();
}

void BBSImpl::context() {
    printf("can't execute BBS::context on a worker\n");
    exit(1);
}

bool BBS::working(int& id, double& x, int& userid) {
    return impl_->working(id, x, userid);
}

void BBS::master_works(int flag) {
    if (impl_->is_master() && nrnmpi_numprocs_bbs > 1) {
        impl_->master_works_ = flag ? true : false;
    }
}

int BBSImpl::master_take_result(int pid) {
    assert(0);
    return 0;
}

bool BBSImpl::working(int& id, double& x, int& userid) {
    int cnt = 0;
    int rtype;
    double t;
    if (n_ <= 0) {
        if (debug) {
            printf("working n_=%d: return false\n", n_);
        }
        return false;
    }
    if (debug) {
        t = time();
    }
    for (;;) {
        ++cnt;
        if (master_works_) {
            id = look_take_result(working_id_);
        } else {
            id = master_take_result(working_id_);
        }
        if (id != 0) {
            userid = upkint();
            int wid = upkint();
            rtype = upkint();
            if (rtype == 0) {
                x = upkdouble();
            } else {
                assert(rtype == 1);
                x = 0.0;
                if (pickle_ret_) {
                    delete[] pickle_ret_;
                }
                pickle_ret_ = upkpickle(&pickle_ret_size_);
            }
            --n_;
            if (debug) {
                printf("working n_=%d: after %d try elapsed %g sec got result for %d id=%d x=%g\n",
                       n_,
                       cnt,
                       time() - t,
                       working_id_,
                       id,
                       x);
            }
            if (userid < 0) {
                return_args(userid);
            }
            return true;
        } else if ((id = look_take_todo()) != 0) {
            if (debug) {
                printf("working: no result for %d but did get _todo id=%d\n", working_id_, id);
            }
            execute(id);
        }
    }
};

void BBS::worker() {
    impl_->runworker_called_ = 1;
    impl_->worker();
}

void BBSImpl::worker() {
    // forever request and execute commands
    double st, et;
    int id;
    if (debug) {
        printf("%d BBS::worker is_master=%d nrnmpi_myid = %d\n",
               nrnmpi_myid_world,
               is_master(),
               nrnmpi_myid);
    }
    if (!is_master()) {
        if (nrnmpi_myid_bbs == -1) {  // wait for message from
            for (;;) {                // the proper nrnmpi_myid == 0
                subworld_worker_execute();
            }
        }
        for (;;) {
            st = time();
            id = take_todo();
            et = time() - st;
            worker_take_time += et;
            execute(id);
        }
    }
}

void BBS::post(const char* key) {
    if (debug) {
        printf("post: |%s|\n", key);
    }
    impl_->post(key);
}

bool BBS::look_take(const char* key) {
    bool b = impl_->look_take(key);
    if (debug) {
        printf("look_take |%s| return %d\n", key, b);
    }
    return b;
}

bool BBS::look(const char* key) {
    bool b = impl_->look(key);
    if (debug) {
        printf("look |%s| return %d\n", key, b);
    }
    return b;
}

void BBS::take(const char* key) {  // blocking
    double t;
    if (debug) {
        t = time();
        printf("begin take |%s| at %g\n", key, t);
    }
    impl_->take(key);
    if (debug) {
        printf("end take |%s| elapsed %g from %g\n", key, time() - t, t);
    }
}

void BBS::done() {
    if (impl_->runworker_called_) {
        impl_->done();
    }
}

void BBSImpl::done() {
    if (done_) {
        return;
    }
    done_ = true;
#ifdef HAVE_TMS
    clock_t elapsed = times(&tmsbuf) - starttime;
    printf("%d tasks in %g seconds. %g seconds waiting for tasks\n",
           etaskcnt,
           total_exec_time,
           worker_take_time);
    printf("user=%g sys=%g elapsed=%g %g%%\n",
           (double) (tmsbuf.tms_utime - tms_start_.tms_utime) / 100,
           (double) (tmsbuf.tms_stime - tms_start_.tms_stime) / 100,
           (double) (elapsed) / 100,
           100. * (double) (tmsbuf.tms_utime - tms_start_.tms_utime) / (double) elapsed);
#endif
}

void BBSImpl::start() {
    if (started_) {
        return;
    }
    started_ = 1;
#ifdef HAVE_TMS
    starttime = times(&tms_start_);
#endif
}
