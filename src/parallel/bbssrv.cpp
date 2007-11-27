#include <../../nrnconf.h>
#include "bbsconf.h"
#ifdef HAVE_PVM3_H	// to end of file
#include <stdio.h>
#include <unistd.h>
#include "bbslsrv2.h"
#include "bbssrv.h"
#include "nrnmpi.h"

#define debug 0

#if defined(HAVE_SIGPOLL)
#include <stropts.h>
#include <errno.h>
#include <signal.h>
//#include <sys/types.h>
//#include <sys/conf.h>

#define POLLDELAY 20

extern "C" {
	void bbs_sig_handler(int);
	int bbs_sig_cnt_;
	int bbs_handled_cnt_;
	int bbs_sig_handle_cnt_;
	int pvm_getfds(int** fds);
}
#endif

#define POLLCNT 300
extern "C" {
	extern int bbs_poll_;
#if defined(HAVE_PKMESG)
	int pvm_upkmesg();
#endif
	void bbs_handle();
	void bbs_sig_set();
	extern double hoc_cross_x_;
}

static int bbs_poll_cnt_;
static int bbs_msg_cnt_;
static int clientid_;

void bbs_handle() {
	if (BBSDirectServer::server_) {
		bbs_poll_ = POLLCNT;
	}else{
		bbs_poll_ = -1;
		return;
	}
#if defined(HAVE_SIGPOLL)
	sighold(SIGPOLL);
	if (bbs_poll_cnt_ < 2*bbs_sig_cnt_) {
		bbs_poll_ = POLLDELAY;
	}
	sigrelse(SIGPOLL);
#endif
	++bbs_poll_cnt_;
	BBSDirectServer::handle();
}

void bbs_sig_set() {
#if defined(HAVE_SIGPOLL)
	if (nrnmpi_numprocs < 2) {return;}
//#define mask (S_RDNORM | S_RDBAND | S_HIPRI | S_MSG)
//#define mask (S_INPUT)
#define mask (S_RDNORM)
	int nfds, i, *fds, arg, e;
	nfds = pvm_getfds(&fds);
	for (i = 0; i < nfds; ++i) {
		e = ioctl(fds[i], I_GETSIG, &arg);
		if (e < 0) {
			if(errno != EINVAL) {
				perror("bbs_sig_set I_GETSIG");
			}
			errno = 0;
		}
		arg = 0;
		if ((arg & mask) == 0) {
//printf("set signal for %d to %lx cnt=%d\n", fds[i], arg|mask, bbs_sig_cnt_);
			if (ioctl(fds[i], I_SETSIG, arg|mask) < 0) {
				perror("bbs_sig_set I_SETSIG");
				errno = 0;
			}
		}
	}
	if (sigset(SIGPOLL, bbs_sig_handler) == SIG_ERR) {
		perror("bbs_sig_set SIGPOLL handler");
	}
#endif
}

#if defined(HAVE_SIGPOLL)
void bbs_sig_handler(int) {
	++bbs_sig_cnt_;
	bbs_poll_ = 1;
//printf("bbs_sig_handler %g %d\n", hoc_cross_x_, bbs_sig_cnt_);
}
#endif


void BBSDirectServer::start() {
	if (nrnmpi_numprocs > 1) {
		bbs_poll_ = POLLCNT;
	}
}
void BBSDirectServer::done() {
	printf("bbs_msg_cnt_=%d bbs_poll_cnt_=%d bbs_poll_=%d\n", 
		bbs_msg_cnt_, bbs_poll_cnt_,
		((bbs_poll_ < 0) ? -bbs_poll_ : bbs_poll_));
#if defined(HAVE_SIGPOLL)
	printf("bbs_sig_cnt_=%d\n", bbs_sig_cnt_);
#endif
}

void BBSDirectServer::handle() {
		if (pvm_probe(-1, -1) > 0) {
		      	int r, s;
		      	r = pvm_setrbuf(pvm_mkbuf(PvmDataDefault));
		      	s = pvm_setsbuf(pvm_mkbuf(PvmDataDefault));
			do {
				handle1();
			} while (pvm_probe(-1, -1) > 0);
			r = pvm_setrbuf(r);
			s = pvm_setsbuf(s);
			pvm_freebuf(r);
			pvm_freebuf(s);
		}
}

void BBSDirectServer::handle1() {
	char key[512];
	int bufid, size, cid, msgtag;
	int index;
	bufid = pvm_recv(-1, -1);
	++bbs_msg_cnt_;
	pvm_bufinfo(bufid, &size, &msgtag, &cid);
	switch (msgtag) {
	case POST_TODO:
		pvm_upkint(&index, 1, 1); // the parent index
#if debug
printf("handle POST_TODO from %x when cross=%g\n", cid, hoc_cross_x_);
#endif
		pvm_freebuf(pvm_setsbuf(pvm_upkmesg()));
		BBSDirectServer::server_->post_todo(index, cid);
		break;
	case POST_RESULT:
		pvm_upkint(&index, 1, 1);
#if debug
printf("handle POST_RESULT %d from %x when cross=%g\n", index, cid, hoc_cross_x_);
#endif
		pvm_freebuf(pvm_setsbuf(pvm_upkmesg()));
		BBSDirectServer::server_->post_result(index);
		break;
	case POST:
		pvm_upkstr(key);
#if debug
printf("handle POST %s from %x when cross=%g\n", key, cid, hoc_cross_x_);
#endif
		pvm_freebuf(pvm_setsbuf(pvm_upkmesg()));
		BBSDirectServer::server_->post(key);
		break;
	case CRAY_POST_TODO:
		pvm_upkint(&index, 1, 1); // the parent index
		pvm_recv(cid, -1);
#if debug
printf("handle CRAY_POST_TODO from %x when cross=%g\n", cid, hoc_cross_x_);
#endif
		BBSDirectServer::server_->post_todo(index, cid);
		break;
	case CRAY_POST_RESULT:
		pvm_upkint(&index, 1, 1);
		pvm_recv(cid, -1);
#if debug
printf("handle CRAY_POST_RESULT %d from %x when cross=%g\n", index, cid, hoc_cross_x_);
#endif
		BBSDirectServer::server_->post_result(index);
		break;
	case CRAY_POST:
		pvm_upkstr(key);
		pvm_recv(cid, -1);
#if debug
printf("handle CRAY_POST %s from %x when cross=%g\n", key, cid, hoc_cross_x_);
#endif
		BBSDirectServer::server_->post(key);
		break;
	case LOOK:
		pvm_upkstr(key);
#if debug
printf("handle LOOK %s from %x when cross=%g\n", key, cid, hoc_cross_x_);
#endif
		if (BBSDirectServer::server_->look(key)) {
			pvm_setrbuf(pvm_setsbuf(pvm_getrbuf()));
			pvm_send(cid, LOOK_YES);
		}else{
			pvm_initsend(PvmDataDefault);
			pvm_send(cid, LOOK_NO);
		}
		break;
	case LOOK_TAKE:
		pvm_upkstr(key);
#if debug
printf("handle LOOK_TAKE %s from %x when cross=%g\n", key, cid, hoc_cross_x_);
#endif
		if (BBSDirectServer::server_->look_take(key)) {
			pvm_setrbuf(pvm_setsbuf(pvm_getrbuf()));
#if debug
printf("handle sending back %d\n", pvm_getsbuf());
#endif
			pvm_send(cid, LOOK_TAKE_YES);
		}else{
			pvm_initsend(PvmDataDefault);
			pvm_send(cid, LOOK_TAKE_NO);
		}
		break;
	case TAKE:
		pvm_upkstr(key);
#if debug
printf("handle TAKE %s from %x when cross=%g\n", key, cid, hoc_cross_x_);
#endif
		if (BBSDirectServer::server_->look_take(key)) {
			pvm_setrbuf(pvm_setsbuf(pvm_getrbuf()));
#if debug
printf("handle sending back %d\n", pvm_getsbuf());
#endif
			pvm_send(cid, TAKE);
		}else{
#if debug
printf("handle put_pending %s for %d\n", key, cid);
#endif
			BBSDirectServer::server_->put_pending(key, cid);
		}
		break;
	case LOOK_TAKE_TODO:
#if debug
printf("handle LOOK_TAKE_TODO for cid=%x\n", cid);
#endif
		index = BBSDirectServer::server_->look_take_todo();
		if (index) {
			pvm_setrbuf(pvm_setsbuf(pvm_getrbuf()));
		}else{
			pvm_initsend(PvmDataDefault);
		}
#if debug
printf("handle sending back id=%d\n", index);
#endif
		pvm_send(cid, index+1);
		break;
	case LOOK_TAKE_RESULT:
		pvm_upkint(&index, 1, 1);
#if debug
printf("handle LOOK_TAKE_RESULT for %x pid=%d\n", cid, index);
#endif
		index = BBSDirectServer::server_->look_take_result(index);
		if (index) {
			pvm_setrbuf(pvm_setsbuf(pvm_getrbuf()));
		}else{
			pvm_initsend(PvmDataDefault);
		}
#if debug
printf("handle sending back id=%d\n", index);
#endif
		pvm_send(cid, index+1);
		break;
	case TAKE_TODO:
#if debug
printf("handle TAKE_TODO for %x\n", cid);
#endif
		if (server_->remaining_context_cnt_ > 0
		    && server_->send_context(cid)) {
#if debug
printf("handle sent back a context\n");
#endif
			break;
		}
		index = BBSDirectServer::server_->look_take_todo();
		if (index) {
			pvm_setrbuf(pvm_setsbuf(pvm_getrbuf()));
#if debug
printf("handle sending back id=%d\n", index);
#endif
			pvm_send(cid, index+1);		
		}else{
#if debug
printf("handle add_looking_todo\n");
#endif
			BBSDirectServer::server_->add_looking_todo(cid);
		}
		break;
	case HELLO:
printf("handle HELLO from %x when cross=%g\n", cid, hoc_cross_x_);
		pvm_initsend(PvmDataDefault);
		pvm_send(cid, ++clientid_);
#if defined(HAVE_SIGPOLL)
		bbs_sig_set();
#endif
		break;
	default:
printf("unknown message\n");
		break;
	}
}
#else
extern "C" {
void bbs_handle(){}
}
#endif //HAVE_PVM3_H

