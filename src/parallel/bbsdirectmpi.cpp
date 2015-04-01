#include <../../nrnconf.h>
#include <nrnmpi.h>
#include "bbsconf.h"
#ifdef NRNMPI	// to end of file
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <InterViews/resource.h>
#include "oc2iv.h"
#include "bbs.h"
#include "bbsdirect.h"
#include "bbssrv2mpi.h"
#include "bbssrv.h"

extern "C" {
extern void nrnmpi_int_broadcast(int*, int, int);
}

#if defined(HAVE_STL)
#if defined(HAVE_SSTREAM) // the standard ...
#include <map>
#else
#include <pair.h>
#include <map.h>
#endif

#define debug 0

struct ltint {
	bool operator() (int i, int j) const {
		return i < j;
	}
};

class KeepArgs : public map<int, bbsmpibuf*, ltint>{};

#endif


BBSDirect::BBSDirect() {
	if (!BBSDirectServer::server_) {
		BBSDirectServer::server_ = new BBSDirectServer();
	}
	sendbuf_ = nil;
	recvbuf_ = nil;
	BBSDirect::start();
#if defined(HAVE_STL)
	keepargs_ = new KeepArgs();
#endif
}

BBSDirect::~BBSDirect() {
	nrnmpi_unref(sendbuf_);
	nrnmpi_unref(recvbuf_);
#if defined(HAVE_STL)
	delete keepargs_;
#endif
}

void BBSDirect::perror(const char* s) {
	printf("BBSDirect error %s\n", s);
}

void BBSDirect::context() {
	BBSDirectServer::handle();
	nrnmpi_enddata(sendbuf_);
	BBSDirectServer::server_->context(sendbuf_);
	nrnmpi_unref(sendbuf_);
	sendbuf_ = nil;
}

int BBSDirect::upkint() {
	int i;
	i = nrnmpi_upkint(recvbuf_);
#if debug
	printf("upkint returning %d\n", i);
#endif
	return i;
}

double BBSDirect::upkdouble() {
	double x;
	x = nrnmpi_upkdouble(recvbuf_);
#if debug
	printf("upkdouble returning %g\n", x);
#endif
	return x;
}

void BBSDirect::upkvec(int n, double* x) {
	nrnmpi_upkvec(n, x, recvbuf_);
}

char* BBSDirect::upkstr() {
	char* s;
	s = nrnmpi_upkstr(recvbuf_);
#if debug
	printf("upkstr returning |%s|\n", s);
#endif
	return s;
}

char* BBSDirect::upkpickle(size_t* n) {
	char* s;
	s = nrnmpi_upkpickle(n, recvbuf_);
#if debug
	printf("upkpickle returning %d bytes\n", *n);
#endif
	return s;
}

void BBSDirect::pkbegin() {
#if debug
printf("%d BBSDirect::pkbegin\n", nrnmpi_myid_bbs);
#endif
	nrnmpi_unref(sendbuf_);
	sendbuf_ = nrnmpi_newbuf(100);
	nrnmpi_ref(sendbuf_);
	nrnmpi_pkbegin(sendbuf_);
}

void BBSDirect::pkint(int i) {
#if debug
printf("%d BBSDirect::pkint %d\n", nrnmpi_myid_bbs, i);
#endif
	nrnmpi_pkint(i, sendbuf_);
}

void BBSDirect::pkdouble(double x) {
#if debug
printf("%d BBSDirect::pkdouble\n", nrnmpi_myid_bbs, x);
#endif
	nrnmpi_pkdouble(x, sendbuf_);
}

void BBSDirect::pkvec(int n, double* x) {
#if debug
printf("%d BBSDirect::pkvec n=%d\n", nrnmpi_myid_bbs, n);
#endif
	nrnmpi_pkvec(n, x, sendbuf_);
}

void BBSDirect::pkstr(const char* s) {
#if debug
printf("%d BBSDirect::pkstr %s\n", nrnmpi_myid_bbs, s);
#endif
	nrnmpi_pkstr(s, sendbuf_);
}

void BBSDirect::pkpickle(const char* s, size_t n) {
#if debug
printf("%d BBSDirect::pkpickle %d bytes\n", nrnmpi_myid_bbs, n);
#endif
	nrnmpi_pkpickle(s, n, sendbuf_);
}

void BBSDirect::post(const char* key) {
#if debug
	printf("%d BBSDirect::post |%s|\n", nrnmpi_myid_bbs, key);
#endif
	nrnmpi_enddata(sendbuf_);
	nrnmpi_pkstr(key, sendbuf_);
	BBSDirectServer::server_->post(key, sendbuf_);
	nrnmpi_unref(sendbuf_);
	sendbuf_ = nil;
	BBSDirectServer::handle();
}

void BBSDirect::post_todo(int parentid) {
#if debug
	printf("%d BBSDirect::post_todo for %d\n", nrnmpi_myid_bbs, parentid);
#endif
	nrnmpi_enddata(sendbuf_);
	nrnmpi_pkint(parentid, sendbuf_);
	BBSDirectServer::server_->post_todo(parentid, nrnmpi_myid_bbs, sendbuf_);
	nrnmpi_unref(sendbuf_);
	sendbuf_ = nil;
	BBSDirectServer::handle();
}

void BBSDirect::post_result(int id) {
#if debug
	printf("%d BBSDirect::post_result %d\n", nrnmpi_myid_bbs, id);
#endif
	nrnmpi_enddata(sendbuf_);
	nrnmpi_pkint(id, sendbuf_);
	BBSDirectServer::server_->post_result(id, sendbuf_);
	nrnmpi_unref(sendbuf_);
	sendbuf_ = nil;
	BBSDirectServer::handle();
}

int BBSDirect::look_take_todo() {
	BBSDirectServer::handle();
	int id = BBSDirectServer::server_->look_take_todo(&recvbuf_);
	if (id) {
		nrnmpi_upkbegin(recvbuf_);
	
#if debug
printf("%d look_take_todo getid=%d\n", nrnmpi_getid(recvbuf_));
#endif
	}
#if debug
printf("%d BBSDirect::look_take_todo id=%d\n", nrnmpi_myid_bbs, id);
#endif
	return id;
}

int BBSDirect::take_todo() {
	int id = BBSDirectServer::server_->look_take_todo(&recvbuf_);
	if (id == 0) {
		printf("BBSDirect::take_todo blocking\n");
		assert(0);
	}
#if debug
	printf("%d BBSDirect::take_todo id=%d\n", nrnmpi_myid_bbs, id);
#endif
	return id;
}

int BBSDirect::look_take_result(int pid) {
	BBSDirectServer::handle();
	int id = BBSDirectServer::server_->look_take_result(pid, &recvbuf_);
#if debug
	printf("%d BBSDirect::look_take_result id=%d pid=%d\n", nrnmpi_myid_bbs, id, pid);
#endif
	if (id) {
		nrnmpi_upkbegin(recvbuf_);
	}
#if debug
printf("%d look_take_result return id=%d\n", nrnmpi_myid_bbs, id);
#endif
	return id;
}

int BBSDirect::master_take_result(int pid) {
	assert(is_master());
	assert(nrnmpi_numprocs_bbs > 1);
	for (;;) {
		int id = look_take_result(pid);
		if (id) {
			return id;
		}
		// wait for a message (no MPI_Iprobe)
		BBSDirectServer::handle_block();
	}
}

void BBSDirect::save_args(int userid) {
#if defined(HAVE_STL)
	nrnmpi_ref(sendbuf_);
	keepargs_->insert(
		pair<const int, bbsmpibuf*>(userid, sendbuf_)
	);
	
#endif
	post_todo(working_id_);
}

void BBSDirect::return_args(int userid) {
#if defined(HAVE_STL)
	KeepArgs::iterator i = keepargs_->find(userid);
	nrnmpi_unref(recvbuf_);
	recvbuf_ = nil;
	if (i != keepargs_->end()) {
		recvbuf_ = (*i).second;
		keepargs_->erase(i);
		nrnmpi_upkbegin(recvbuf_);
		BBSImpl::return_args(userid);
	}
#endif
}

bool BBSDirect::look_take(const char* key) {
	BBSDirectServer::handle();
	bool b = BBSDirectServer::server_->look_take(key, &recvbuf_);
	if (b) {
		nrnmpi_upkbegin(recvbuf_);
	}
#if debug
	if (b) {
		printf("look_take |%s| true\n", key);
	}else{
		printf("look_take |%s| false\n", key);
	}
#endif
	return b;
}

bool BBSDirect::look(const char* key) {
	BBSDirectServer::handle();
	bool b = BBSDirectServer::server_->look(key, &recvbuf_);
	if (b) {
		nrnmpi_upkbegin(recvbuf_);
	}
#if debug
	printf("look |%s| %d\n", key, b);
#endif
	return b;
}

void BBSDirect::take(const char* key) { // blocking
	int id;
	double st = time();
	for (;;) {
		if (look_take(key)) {
			wait_time_ += time() - st;
			return;
		} else if ((id = look_take_todo()) != 0) {
			wait_time_ += time() - st;
			execute(id);
			st = time();
		}else{
			// perhaps should do something meaningful here
			// like check whether to quit or not
		}
	}
}
	
void BBSDirect::done() {
//printf("%d bbsdirect::done\n", nrnmpi_myid_world);
	int i;
	if (done_) {
		return;
	}
	if (nrnmpi_numprocs > 1 && nrnmpi_numprocs_bbs < nrnmpi_numprocs_world) {
		int info[2]; info[0] = -2; info[1] = -1;
//printf("%d broadcast %d %d\n", nrnmpi_myid_world, info[0], info[1]);
		nrnmpi_int_broadcast(info, 2, 0);
	}
	BBSImpl::done();
	done_ = true;
	nrnmpi_unref(sendbuf_);
	sendbuf_ = nrnmpi_newbuf(20);
#if debug
printf("done: numprocs_bbs=%d\n", nrnmpi_numprocs_bbs);
#endif
	for (i=1; i < nrnmpi_numprocs_bbs; ++i) {
		nrnmpi_bbssend(i, QUIT, sendbuf_);
//printf("kill %d\n", i);
	}
	BBSDirectServer::server_->done();
}

void BBSDirect::start() {
	if (started_) { return; }
	BBSImpl::start();
	is_master_ = true;
	BBSDirectServer::server_->start();
	bbs_handle();
}

#endif //NRNMPI
