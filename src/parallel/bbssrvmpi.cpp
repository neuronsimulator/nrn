#include <../../nrnconf.h>
#include <nrnmpi.h>
#ifdef NRNMPI  // to end of file
#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "bbssrv2mpi.h"
#include "bbssrv.h"

#define debug 0

#define POLLCNT 300
extern int bbs_poll_;
void bbs_handle();
extern double hoc_cross_x_;

static int bbs_poll_cnt_;
static int bbs_msg_cnt_;

void bbs_handle() {
    if (BBSDirectServer::server_) {
        bbs_poll_ = POLLCNT;
    } else {
        bbs_poll_ = -1;
        return;
    }
    ++bbs_poll_cnt_;
    BBSDirectServer::handle();
}

void BBSDirectServer::start() {
    if (nrnmpi_numprocs_bbs > 1) {
        bbs_poll_ = POLLCNT;
    }
}
void BBSDirectServer::done() {
    return;
    printf("bbs_msg_cnt_=%d bbs_poll_cnt_=%d bbs_poll_=%d\n",
           bbs_msg_cnt_,
           bbs_poll_cnt_,
           ((bbs_poll_ < 0) ? -bbs_poll_ : bbs_poll_));
}

void BBSDirectServer::handle_block() {
    int size;
    int tag;
    int source;
    nrnmpi_probe(&size, &tag, &source);
    handle1(size, tag, source);
}

void BBSDirectServer::handle() {
    int size;
    int tag;
    int source;
    if (nrnmpi_iprobe(&size, &tag, &source) != 0) {
        do {
            handle1(size, tag, source);
        } while (nrnmpi_iprobe(&size, &tag, &source) != 0);
    }
}

void BBSDirectServer::handle1(int size, int tag, int cid) {
    bbsmpibuf* recv;
    bbsmpibuf* send;
    char* key;
    int index;
    send = nullptr;
    recv = nrnmpi_newbuf(size);
    nrnmpi_ref(recv);
    tag = nrnmpi_bbsrecv(cid, recv);
    ++bbs_msg_cnt_;
    if (size > 0) {
        nrnmpi_upkbegin(recv);
    }
    switch (tag) {
    case POST_TODO:
        index = nrnmpi_getid(recv);  // the parent index
#if debug
        printf("handle POST_TODO from %x when cross=%g\n", cid, hoc_cross_x_);
#endif
        BBSDirectServer::server_->post_todo(index, cid, recv);
        break;
    case POST_RESULT:
        index = nrnmpi_getid(recv);
#if debug
        printf("handle POST_RESULT %d from %x when cross=%g\n", index, cid, hoc_cross_x_);
#endif
        BBSDirectServer::server_->post_result(index, recv);
        break;
    case POST:
        key = nrnmpi_getkey(recv);
#if debug
        printf("handle POST %s from %x when cross=%g\n", key, cid, hoc_cross_x_);
#endif
        BBSDirectServer::server_->post(key, recv);
        break;
    case LOOK:
        key = nrnmpi_getkey(recv);
#if debug
        printf("handle LOOK %s from %x when cross=%g\n", key, cid, hoc_cross_x_);
#endif
        if (BBSDirectServer::server_->look(key, &send)) {
            nrnmpi_bbssend(cid, LOOK_YES, send);
            nrnmpi_unref(send);
        } else {
            nrnmpi_bbssend(cid, LOOK_NO, nullptr);
        }
        break;
    case LOOK_TAKE:
        key = nrnmpi_getkey(recv);
#if debug
        printf("handle LOOK_TAKE %s from %x when cross=%g\n", key, cid, hoc_cross_x_);
#endif
        if (BBSDirectServer::server_->look_take(key, &send)) {
#if debug
            printf("handle sending back something\n");
#endif
            nrnmpi_bbssend(cid, LOOK_TAKE_YES, send);
            nrnmpi_unref(send);
        } else {
            nrnmpi_bbssend(cid, LOOK_TAKE_NO, nullptr);
        }
        break;
    case TAKE:
        key = nrnmpi_getkey(recv);
#if debug
        printf("handle TAKE %s from %x when cross=%g\n", key, cid, hoc_cross_x_);
#endif
        if (BBSDirectServer::server_->look_take(key, &send)) {
#if debug
            printf("handle sending back something\n");
#endif
            nrnmpi_bbssend(cid, TAKE, send);
            nrnmpi_unref(send);
        } else {
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
        index = BBSDirectServer::server_->look_take_todo(&send);
#if debug
        printf("handle sending back id=%d\n", index);
#endif
        nrnmpi_bbssend(cid, index + 1, send);
        if (index) {
            nrnmpi_unref(send);
        }
        break;
    case LOOK_TAKE_RESULT:
        index = nrnmpi_getid(recv);
#if debug
        printf("handle LOOK_TAKE_RESULT for %x pid=%d\n", cid, index);
#endif
        index = BBSDirectServer::server_->look_take_result(index, &send);
#if debug
        printf("handle sending back id=%d\n", index);
#endif
        nrnmpi_bbssend(cid, index + 1, send);
        if (index) {
            nrnmpi_unref(send);
        }
        break;
    case TAKE_TODO:
#if debug
        printf("handle TAKE_TODO for %x\n", cid);
#endif
        if (server_->remaining_context_cnt_ > 0 && server_->send_context(cid)) {
#if debug
            printf("handle sent back a context\n");
#endif
            break;
        }
        index = BBSDirectServer::server_->look_take_todo(&send);
        if (index) {
#if debug
            printf("handle sending back id=%d\n", index);
#endif
            nrnmpi_bbssend(cid, index + 1, send);
            nrnmpi_unref(send);
        } else {
#if debug
            printf("handle add_looking_todo\n");
#endif
            BBSDirectServer::server_->add_looking_todo(cid);
        }
        break;
    case HELLO:
#if debug
        printf("handle HELLO from %x when cross=%g\n", cid, hoc_cross_x_);
#endif
        nrnmpi_pkbegin(recv);
        nrnmpi_enddata(recv);
        nrnmpi_bbssend(cid, HELLO, recv);
        break;
    default:
        printf("unknown message\n");
        break;
    }
    nrnmpi_unref(recv);
}
#else
void bbs_handle() {}
#endif  // NRNMPI
