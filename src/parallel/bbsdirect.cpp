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
#include "bbsdirect.h"
#include "bbslsrv2.h"
#include "bbssrv.h"
#include "nrnmpi.h"

extern "C" {
	extern int nrn_global_argc;
	extern char** nrn_global_argv;
	extern int bbs_nhost_;
	void bbs_sig_set();
#if defined(HAVE_PKMESG)
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


static char* rel_working_dir();

static int* cids;
static int ncids;

BBSDirect::BBSDirect() {
	if (!BBSDirectServer::server_) {
		BBSDirectServer::server_ = new BBSDirectServer();
	}
	BBSDirect::start();
#if defined(HAVE_STL)
	keepargs_ = new KeepArgs();
#endif
}

BBSDirect::~BBSDirect() {
#if defined(HAVE_STL)
	delete keepargs_;
#endif
}

void BBSDirect::check_pvm() {
	BBSImpl:use_pvm_ = pvm_parent() != PvmSysErr;
}

void BBSDirect::perror(const char* s) {
	pvm_perror((char*)s);
}

void BBSDirect::context() {
	BBSDirectServer::server_->context(ncids, cids);
}

int BBSDirect::upkint() {
	int i;
//	printf("upkint from %d\n", pvm_getrbuf());
	if (pvm_upkint(&i, 1, 1)) { perror("upkint"); }
//	printf("upkint returning %d\n", i);
	return i;
}

double BBSDirect::upkdouble() {
	double x;
//	printf("upkdouble from %d\n", pvm_getrbuf());
	if( pvm_upkdouble(&x, 1, 1)) { perror("upkdouble"); }
//	printf("upkdouble returning %g\n", x);
	return x;
}

void BBSDirect::upkvec(int n, double* x) {
//	printf("upkvec from %d\n", pvm_getrbuf());
	if( pvm_upkdouble(x, n, 1)) { perror("upkvec"); }
}

char* BBSDirect::upkstr() {
	int len;
	char* s;
//	printf("upkstr from %d\n", pvm_getrbuf());
	if( pvm_upkint(&len, 1, 1)) { perror("upkstr"); }
	s = new char[len+1];
	pvm_upkstr(s);
//	printf("upkstr returning |%s|\n", s);
	return s;
}

char* BBSDirect::upkpickle(size_t* n) {
	char* s = 0;
	assert(0);
	return s;
}

void BBSDirect::pkbegin() {
	if( pvm_initsend(PvmDataDefault) < 0 ) { perror("pkbegin"); }
//	printf("pkbegin into sbuf %d\n", pvm_getsbuf());
}

void BBSDirect::pkint(int i) {
	int ii = i;
//	printf("pkint %d into %d\n", i, pvm_getsbuf());
	if( pvm_pkint(&ii, 1, 1)) { perror("pkint"); }
}

void BBSDirect::pkdouble(double x) {
	double xx = x;
//	printf("pkdouble %g into %d\n", x, pvm_getsbuf());
	if( pvm_pkdouble(&xx, 1, 1)) { perror("pkdouble"); }
}

void BBSDirect::pkvec(int n, double* x) {
	if( pvm_pkdouble(x, n, 1)) { perror("pkvec"); }
}

void BBSDirect::pkstr(const char* s) {
	int len = strlen(s);
//	printf("pkstr |%s| into %d\n", s,  pvm_getsbuf());
	if( pvm_pkint(&len, 1, 1)) { perror("pkstr length"); }
	if( pvm_pkstr((char*)s)) { perror("pkstr string"); }
}

void BBSDirect::pkpickle(const char* s, size_t n) {
	assert(0);
}

void BBSDirect::post(const char* key) {
//	printf("post |%s| sbuf %d\n", key, pvm_getsbuf());
	BBSDirectServer::server_->post(key);
	BBSDirectServer::handle();
}

void BBSDirect::post_todo(int parentid) {
//	printf("post_todo for %d sbuf %d\n", parentid, pvm_getsbuf());
	BBSDirectServer::server_->post_todo(parentid, mytid_);
	BBSDirectServer::handle();
}

void BBSDirect::post_result(int id) {
//	printf("post_result %d sbuf %d\n", id, pvm_getsbuf());
	BBSDirectServer::server_->post_result(id);
	BBSDirectServer::handle();
}

int BBSDirect::look_take_todo() {
	BBSDirectServer::handle();
	int id = BBSDirectServer::server_->look_take_todo();
//	printf("look_take_todo rbuf %d\n", pvm_getrbuf());
	return id;
}

int BBSDirect::take_todo() {
	int id = BBSDirectServer::server_->look_take_todo();
	if (id == 0) {
		printf("BBSDirect::take_todo blocking\n");
		assert(0);
	}
//	printf("take_todo rbuf %d\n", pvm_getrbuf());
	return id;
}

int BBSDirect::look_take_result(int pid) {
	BBSDirectServer::handle();
	int id = BBSDirectServer::server_->look_take_result(pid);
//	printf("look_take_result %d rbuf %d\n", pid, pvm_getrbuf());
	return id;
}

void BBSDirect::save_args(int userid) {
#if defined(HAVE_STL)
	int bufid = pvm_setsbuf(pvm_mkbuf(PvmDataDefault));
	pvm_pkmesgbody(bufid);
	keepargs_->insert(
		pair<const int, int>(userid, bufid)
	);
	
#endif
	BBSDirectServer::server_->post_todo(working_id_, mytid_);
	BBSDirectServer::handle();
}

void BBSDirect::return_args(int userid) {
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

bool BBSDirect::look_take(const char* key) {
	BBSDirectServer::handle();
	bool b = BBSDirectServer::server_->look_take(key);
#if 0
	if (b) {
		printf("look_take |%s| true rbuf %d\n", key, pvm_getrbuf());
	}else{
		printf("look_take |%s| false rbuf %d\n", key, pvm_getrbuf());
	}
#endif
	return b;
}

bool BBSDirect::look(const char* key) {
	BBSDirectServer::handle();
	bool b = BBSDirectServer::server_->look(key);
	printf("look |%s| %d rbug %d\n", key, b, pvm_getrbuf());
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
	int i;
	if (done_) {
		return;
	}
	BBSImpl::done();
	done_ = true;
printf("done: ncids=%d\n", ncids);
	for (i=0; i < ncids; ++i) {
		pvm_initsend(PvmDataDefault);
		pvm_send(cids[i], QUIT);		
//printf("kill %d\n", cids[i]);
//		pvm_kill(cids[i]);
	}
	if (pvm_exit()) {perror("BBSDirect::done");}
	BBSDirectServer::server_->done();
}

void BBSDirect::start() {
	char* client = 0;
	int tid, host_mytid;
	int i, n, ncpu, nncpu;
	struct pvmhostinfo* hostp;
	if (started_) { return; }
	BBSImpl::start();
	mytid_ = pvm_mytid();
	nrnmpi_myid = 0;
	if (mytid_ < 0) { perror("start"); }
	host_mytid = pvm_tidtohost(mytid_);
	tid = pvm_parent();
	if (tid == PvmSysErr) {
		perror("start");
	}else if (tid == PvmNoParent) {
		is_master_ = true;
		pvm_catchout(stdout);
		pvm_setopt(PvmRoute, PvmRouteDirect);
		pvm_config(&n, NULL, &hostp);
		nncpu = 0;
		for (i=0; i < n; ++i) {
			ncpu = hostp[i].hi_speed;
			if (ncpu%1000) {
				hoc_warning(hostp[i].hi_name,
" speed in pvm configuration file is not a multiple of 1000. Assuming 1000.");
				ncpu = 1000;
			}
			nncpu += ncpu/1000;
		}
		nrnmpi_numprocs = nncpu;
		ncids = 0;
	}else{ // a worker, impossible
		assert(false);
	}
	if (nrnmpi_numprocs > 1 && tid == PvmNoParent) {
		char ** sargv;
	// args are workingdirectory specialOrNrniv -bbs_nhost nhost args
		sargv = new char*[nrn_global_argc + 4];
		for (i=1; i < nrn_global_argc; ++i) {
			sargv[i+3] = nrn_global_argv[i];
		}
		sargv[nrn_global_argc + 3] = 0;
		sargv[0] = rel_working_dir();
//printf("sargv[0]=|%s|\n", sargv[0]);
		sargv[1] = nrn_global_argv[0];
		sargv[2] = "-bbs_nhost";
		sargv[3] = new char[10];
		sprintf(sargv[3], "%d", nrnmpi_numprocs);
		cids = new int[nrnmpi_numprocs-1];
if (nrn_global_argv[nrn_global_argc] != 0) {
	printf("argv not null terminated\n");
	exit(1);
}

		BBSDirectServer::server_->start();
		bbs_sig_set();
		bbs_handle();

		//spawn according to number of cpu's (but master has one less)
//printf("%d total number of cpus on %d machines\n", nncpu, n);
		int icid = 0;
		bool first = true;
	    while (icid < nrnmpi_numprocs - 1) {
		for (i=0; i < n; ++i) {
			ncpu = hostp[i].hi_speed;
			if (ncpu%1000) {
				ncpu = 1000;
			}
			ncpu /= 1000;
//printf("%d cpu for machine %d (%s)\n", ncpu, i, hostp[i].hi_name);
			if (first && hostp[i].hi_tid == host_mytid) {
				// spawn one fewer on master first time through
				--ncpu;
			}
			if (icid + ncpu >= nrnmpi_numprocs) {
				ncpu = nrnmpi_numprocs - icid - 1;
			}
//printf("before spawn %d processes (icid=%d) for machine %d (%s)\n", ncpu, icid, i, hostp[i].hi_name);
			if (ncpu) {
				ncids = pvm_spawn("bbswork.sh", sargv,
					PvmTaskHost, hostp[i].hi_name, ncpu, cids + icid);
				if (ncids != ncpu) {
fprintf(stderr, "Tried to spawn %d tasks, only %d succeeded on %s\n", ncpu, ncids, hostp[i].hi_name);
hoc_execerror("Could not spawn all the requested tasks for", hostp[i].hi_name);
				}
//printf("spawned %d for %s with cids starting at %d\n", ncpu, hostp[i].hi_name, icid);
				icid += ncpu;
			}
			if (icid >= nrnmpi_numprocs) {
				break;
			}
		}
		first = false;
	    }
		ncids = icid;
printf("spawned %d more %s on %d cpus on %d machines\n", ncids, nrn_global_argv[0], nncpu, n);
		delete [] sargv[3];
		delete [] sargv;
	}
}

static char* rel_working_dir() {
	// return relative to $HOME if possible, otherwise absolute
//	char* wd = new char[1024];
//	getwd(wd);
	char* wd = getenv("PWD");
	char* home = getenv("HOME");
	int nh = strlen(home);
	if (strncmp(wd, home, nh) == 0) {
		if (wd[nh] == '/') {++nh;}
#if 1
		return wd + nh;
#else
		// since then mem will be freed elsewhere
		char* rel = new char[strlen(wd) - nh + 1];
		strcpy(rel, wd+nh);
		delete [] wd;
		return rel;
#endif
	}
	return wd;
}
#endif //HAVE_PVM3_H
