#include <../../nrnconf.h>
#include "bbsconf.h"
#ifdef HAVE_PVM3_H	// to end of file
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pvm3.h>
#include <InterViews/resource.h>
#include "oc2iv.h"
#include "bbs.h"
#include "bbsrcli.h"
#include "bbssrv.h"
#include <nrnmpi.h>

#define debug 0

extern "C" {
	extern int nrn_global_argc;
	extern char** nrn_global_argv;
#if defined(HAVE_PKMESG)
	int pvm_pkmesg(int mid);
	int pvm_pkmesgbody(int);
#endif
};

#if defined(HAVE_STL)
#if defined(HAVE_SSTREAM) // the standard ...
#include <map>
#else
#include <pair.h>
#include <map.h>
#endif

struct ltint {
	bool operator() (int i, int j) const {
		return i < j;
	}
};

class KeepArgs : public map<int, int, ltint>{};

#endif

int BBSClient::sid_;

BBSClient::BBSClient() {
	BBSClient::start();
#if defined(HAVE_STL)
	keepargs_ = new KeepArgs();
#endif
}

BBSClient::~BBSClient() {
#if defined(HAVE_STL)
	delete keepargs_;
#endif
}

void BBSClient::perror(const char* s) {
	pvm_perror((char*)s);
}

int BBSClient::upkint() {
	int i;
	if (pvm_upkint(&i, 1, 1)) { perror("upkint"); }
	return i;
}

double BBSClient::upkdouble() {
	double x;
	if( pvm_upkdouble(&x, 1, 1)) { perror("upkdouble"); }
	return x;
}

void BBSClient::upkvec(int n, double* x) {
	if( pvm_upkdouble(x, n, 1)) { perror("upkvec"); }
}

char* BBSClient::upkstr() {
	int len;
	char* s;
	if( pvm_upkint(&len, 1, 1)) { perror("upkstr"); }
	s = new char[len+1];
	pvm_upkstr(s);
	return s;
}

char* BBSClient::upkpickle(size_t* n) {
	assert(0);
	return s;
}

void BBSClient::pkbegin() {
	if( pvm_initsend(PvmDataDefault) < 0 ) { perror("pkbegin"); }
}

void BBSClient::pkint(int i) {
	int ii = i;
	if( pvm_pkint(&ii, 1, 1)) { perror("pkint"); }
}

void BBSClient::pkdouble(double x) {
	double xx = x;
	if( pvm_pkdouble(&xx, 1, 1)) { perror("pkdouble"); }
}

void BBSClient::pkvec(int n, double* x) {
	if( pvm_pkdouble(x, n, 1)) { perror("pkvec"); }
}

void BBSClient::pkstr(const char* s) {
	int len = strlen(s);
	if( pvm_pkint(&len, 1, 1)) { perror("pkstr length"); }
	if( pvm_pkstr((char*)s)) { perror("pkstr string"); }
}

void BBSClient::pkpickle(const char* s, size_t n) {
	assert(0);
}

void BBSClient::post(const char* key) {
#if debug
printf("BBSClient::post |%s|\n", key);
fflush(stdout);
#endif
	int index, os;
	os = pvm_setsbuf(pvm_mkbuf(PvmDataDefault));
	pvm_pkstr((char*)key);
#if defined(HAVE_PKMESG)
	pvm_pkmesg(os);
	index = pvm_send(sid_, POST);
#else
	pvm_send(sid_, CRAY_POST);
	os = pvm_setsbuf(os);
	index = pvm_send(sid_, CRAY_POST);
#endif
	pvm_freebuf(os);
	if (index < 0) {perror("post");}
}

void BBSClient::post_todo(int parentid) {
#if debug
printf("BBSClient::post_todo for %d\n", parentid);
fflush(stdout);
#endif
	int index, os;
	os = pvm_setsbuf(pvm_mkbuf(PvmDataDefault));
	pvm_pkint(&parentid, 1, 1);
#if defined(HAVE_PKMESG)
	pvm_pkmesg(os);
	index = pvm_send(sid_, POST_TODO);
#else
	pvm_send(sid_, CRAY_POST_TODO);
	os = pvm_setsbuf(os);
	index = pvm_send(sid_, CRAY_POST_TODO);
#endif
	pvm_freebuf(os);
	if (index < 0) {perror("post_todo");}
}

void BBSClient::post_result(int id) {
#if debug
printf("BBSClient::post_result %d\n", id);
fflush(stdout);
#endif
	int index, os;
	os = pvm_setsbuf(pvm_mkbuf(PvmDataDefault));
	pvm_pkint(&id, 1, 1);
#if defined(HAVE_PKMESG)
	pvm_pkmesg(os);
	index = pvm_send(sid_, POST_RESULT);
#else
	pvm_send(sid_, CRAY_POST_RESULT);
	os = pvm_setsbuf(os);
	index = pvm_send(sid_, CRAY_POST_RESULT);
#endif
	pvm_freebuf(os);
	if (index < 0) {perror("post_result");}
}


int BBSClient::get(const char* key, int type) {
#if debug
printf("BBSClient::get |%s| type=%d\n", key, type);
fflush(stdout);
#endif
	int bufid, nbyte, msgtag, tid, index;
	pvm_initsend(PvmDataDefault);
	pvm_pkstr((char*)key);
	return get(type);
}

int BBSClient::get(int key, int type) {
#if debug
printf("BBSClient::get %d type=%d\n", key, type);
fflush(stdout);
#endif
	pvm_initsend(PvmDataDefault);
	pvm_pkint(&key, 1, 1);
	return get(type)-1; // sent id+1 so cannot be mistaken for QUIT
}

int BBSClient::get(int type) { // blocking
fflush(stdout);
fflush(stderr);
	double st = time();	
	int bufid, nbyte, msgtag, tid, index;
	index = pvm_send(sid_, type);
	bufid = pvm_recv(sid_, -1);
	pvm_bufinfo(bufid, &nbyte, &msgtag, &tid);
	wait_time_ += time() - st;
	if (bufid < 0) { perror("take"); }
#if debug
printf("BBSClient::get return msgtag=%d\n", msgtag);
fflush(stdout);
#endif
	if (msgtag == QUIT) {
		done();
	}
	return msgtag;
}
	
bool BBSClient::look_take(const char* key) {
	int type = get(key, LOOK_TAKE);
	bool b = (type == LOOK_TAKE_YES);
	return b;
}

bool BBSClient::look(const char* key) {
	int type = get(key, LOOK);
	bool b = (type == LOOK_YES);
	return b;
}

void BBSClient::take(const char* key) { // blocking
	int bufid;
	get(key, TAKE);	
}
	
int BBSClient::look_take_todo() {
	int type = get(0, LOOK_TAKE_TODO);
	return type;
}

int BBSClient::take_todo() {
	int type;
	char* rs;
	size_t n;
	while((type = get(0, TAKE_TODO)) == CONTEXT) {
		upkint(); // throw away userid
#if debug
printf("execute context\n");
fflush(stdout);
#endif
		rs = execute_helper(&n, -1);
		if (rs) { delete [] rs; }
	}
	return type;
}

int BBSClient::look_take_result(int pid) {
	int type = get(pid, LOOK_TAKE_RESULT);
	return type;
}

void BBSClient::save_args(int userid) {
#if defined(HAVE_PKMESG)
#if defined(HAVE_STL)
	int bufid = pvm_setsbuf(pvm_mkbuf(PvmDataDefault));
	pvm_pkmesgbody(bufid);
	keepargs_->insert(
		pair<const int, int>(userid, bufid)
	);
	
#endif
	post_todo(working_id_);
#else
	int index, os;
	os = pvm_setsbuf(pvm_mkbuf(PvmDataDefault));
	pvm_pkint(&working_id_, 1, 1);
	pvm_send(sid_, CRAY_POST_TODO);
	os = pvm_setsbuf(os);
	index = pvm_send(sid_, CRAY_POST_TODO);
	os = pvm_setsbuf(os);
#if defined(HAVE_STL)
	keepargs_->insert(
		pair<const int, int>(userid, os)
	);
	
#endif
#endif
}

void BBSClient::return_args(int userid) {
#if defined(HAVE_STL)
	KeepArgs::iterator i = keepargs_->find(userid);
	if (i != keepargs_->end()) {
		int bufid = (*i).second;
		keepargs_->erase(i);
		pvm_freebuf(pvm_setrbuf(bufid));
		BBSImpl::return_args(userid);
	}
#endif
}

void BBSClient::done() {
#if debug
printf("BBSClient::done\n");
fflush(stdout);
#endif
	BBSImpl::done();
	if (pvm_exit()) {perror("BBSClient::done");}
	exit(0);
}

void BBSClient::start() {
	char* client = 0;
	int tid;
	int n;
	if (started_) { return; }
#if debug
printf("BBSClient start\n");
fflush(stdout);
#endif
	BBSImpl::start();
	mytid_ = pvm_mytid();
	if (mytid_ < 0) { perror("start"); }
	tid = pvm_parent();
	sid_ = tid;
	if (tid == PvmSysErr) {
		perror("start");
	}
	assert(tid > 0);
	{ // a worker
		is_master_ = false;
		pvm_initsend(PvmDataDefault);
		nrnmpi_myid = get(HELLO);
		assert(nrnmpi_myid > 0);
		return;
	}
}

#endif //HAVE_PVM_H
