#include <../../nrnconf.h>
#include <nrnmpi.h>
#if NRNMPI  // to end of file
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "bbssrv2mpi.h"
#include "bbssrv.h"
#include "bbsimpl.h"
#include "hocdec.h"  //Printf

void nrnbbs_context_wait();

BBSDirectServer* BBSDirectServer::server_;

#include <map>
#include <set>

#define debug 0

#define MessageList     MpiMessageList
#define WorkItem        MpiWorkItem
#define WorkList        MpiWorkList
#define ReadyList       MpiReadyList
#define ResultList      MpiResultList
#define PendingList     MpiPendingList
#define LookingToDoList MpiLookingToDoList

class WorkItem {
  public:
    WorkItem(int id, bbsmpibuf* buf, int cid);
    virtual ~WorkItem();
    WorkItem* parent_;
    int id_;
    bbsmpibuf* buf_;
    int cid_;  // mpi host id
    bool todo_less_than(const WorkItem*) const;
};

struct ltstr {
    bool operator()(const char* s1, const char* s2) const {
        return strcmp(s1, s2) < 0;
    }
};

struct ltint {
    bool operator()(int i, int j) const {
        return i < j;
    }
};

struct ltWorkItem {
    bool operator()(const WorkItem* w1, const WorkItem* w2) const {
        return w1->todo_less_than(w2);
    }
};

static char* newstr(const char* s) {
    char* s1 = new char[strlen(s) + 1];
    strcpy(s1, s);
    return s1;
}

WorkItem::WorkItem(int id, bbsmpibuf* buf, int cid) {
#if debug == 2
    printf("WorkItem %d\n", id);
#endif
    id_ = id;
    buf_ = buf;
    cid_ = cid;
    parent_ = nullptr;
}

WorkItem::~WorkItem() {
#if debug
    printf("~WorkItem %d\n", id_);
#endif
}

bool WorkItem::todo_less_than(const WorkItem* w) const {
    WorkItem* w1 = (WorkItem*) this;
    WorkItem* w2 = (WorkItem*) w;
    while (w1->parent_ != w2->parent_) {
        if (w1->id_ < w2->id_) {
            w2 = w2->parent_;
        } else {
            w1 = w1->parent_;
        }
    }
#if debug
    printf("todo_less_than %d < %d return %d\n", this->id_, w->id_, w1->id_ < w2->id_);
#endif
    return w1->id_ < w2->id_;
}

class MessageList: public std::multimap<const char*, bbsmpibuf*, ltstr> {};
class PendingList: public std::multimap<const char*, const int, ltstr> {};
class WorkList: public std::map<int, const WorkItem*, ltint> {};
class LookingToDoList: public std::set<int, ltint> {};
class ReadyList: public std::set<const WorkItem*, ltWorkItem> {};
class ResultList: public std::multimap<int, const WorkItem*, ltint> {};

BBSDirectServer::BBSDirectServer() {
    messages_ = new MessageList();
    work_ = new WorkList();
    todo_ = new ReadyList();
    results_ = new ResultList();
    pending_ = new PendingList();
    looking_todo_ = new LookingToDoList();
    send_context_ = new LookingToDoList();
    next_id_ = FIRSTID;
    context_buf_ = nullptr;
    remaining_context_cnt_ = 0;
}

BBSDirectServer::~BBSDirectServer() {
    delete todo_;
    delete results_;
    delete looking_todo_;
    printf("~BBSLocalServer not deleting everything\n");
    // need to unref MessageValue in messages_ and delete WorkItem in work_
    delete pending_;
    delete messages_;
    delete work_;
    delete send_context_;
}

bool BBSDirectServer::look_take(const char* key, bbsmpibuf** recv) {
#if debug
    printf("DirectServer::look_take |%s|\n", key);
#endif
    bool b = false;
    nrnmpi_unref(*recv);
    *recv = nullptr;
    MessageList::iterator m = messages_->find(key);
    if (m != messages_->end()) {
        b = true;
        *recv = (*m).second;
        // printf("free %d\n", buf);
        char* s = (char*) ((*m).first);
        messages_->erase(m);
        delete[] s;
    }
#if debug
    printf("DirectServer::look_take |%s| recv=%p return %d\n", key, (*recv), b);
#endif
    return b;
}

bool BBSDirectServer::look(const char* key, bbsmpibuf** recv) {
#if debug
    printf("DirectServer::look |%s|\n", key);
#endif
    bool b = false;
    nrnmpi_unref(*recv);
    *recv = nullptr;
    MessageList::iterator m = messages_->find(key);
    if (m != messages_->end()) {
        b = true;
        *recv = (*m).second;
        if (*recv) {
            nrnmpi_ref(*recv);
        }
    }
#if debug
    printf("DirectServer::look |%s| recv=%p return %d\n", key, (*recv), b);
#endif
    return b;
}

void BBSDirectServer::put_pending(const char* key, int cid) {
#if debug
    printf("put_pending |%s| %d\n", key, cid);
#endif
    char* s = newstr(key);
    pending_->insert(std::pair<const char* const, const int>(s, cid));
}

bool BBSDirectServer::take_pending(const char* key, int* cid) {
    bool b = false;
    PendingList::iterator p = pending_->find(key);
    if (p != pending_->end()) {
        *cid = (*p).second;
#if debug
        printf("take_pending |%s| %d\n", key, *cid);
#endif
        char* s = (char*) ((*p).first);
        pending_->erase(p);
        delete[] s;
        b = true;
    }
    return b;
}

void BBSDirectServer::post(const char* key, bbsmpibuf* send) {
    int cid;
#if debug
    printf("DirectServer::post |%s| send=%p\n", key, send);
#endif
    if (take_pending(key, &cid)) {
        nrnmpi_bbssend(cid, TAKE, send);
    } else {
        MessageList::iterator m = messages_->insert(
            std::pair<const char* const, bbsmpibuf*>(newstr(key), send));
        nrnmpi_ref(send);
    }
}

void BBSDirectServer::add_looking_todo(int cid) {
    looking_todo_->insert(cid);
}

void BBSDirectServer::post_todo(int pid, int cid, bbsmpibuf* send) {
#if debug
    printf("BBSDirectServer::post_todo pid=%d cid=%d send=%p\n", pid, cid, send);
#endif
    WorkItem* w = new WorkItem(next_id_++, send, cid);
    nrnmpi_ref(send);
    WorkList::iterator p = work_->find(pid);
    if (p != work_->end()) {
        w->parent_ = (WorkItem*) ((*p).second);
    }
    work_->insert(std::pair<const int, const WorkItem*>(w->id_, w));
#if debug
    printf("work insert %d\n", w->id_);
#endif
    LookingToDoList::iterator i = looking_todo_->begin();
    if (i != looking_todo_->end()) {
        cid = (*i);
        looking_todo_->erase(i);
        // the send buffer is correct
        nrnmpi_bbssend(cid, w->id_ + 1, send);
    } else {
#if debug
        printf("todo insert\n");
#endif
        todo_->insert(w);
    }
}

void BBSDirectServer::context(bbsmpibuf* send) {
    int cid, j;
#if debug
    printf("numprocs_bbs=%d\n", nrnmpi_numprocs_bbs);
#endif
    // Previous context may complete after allowing more activity
    for (j = 0; j < 1000; ++j) {
        if (remaining_context_cnt_ == 0) {
            break;
        }
        handle();
    }
    if (remaining_context_cnt_ > 0) {
        Printf("some workers did not receive previous context\n");
        send_context_->erase(send_context_->begin(), send_context_->end());
        nrnmpi_unref(context_buf_);
        context_buf_ = nullptr;
    }
    remaining_context_cnt_ = nrnmpi_numprocs_bbs - 1;
    for (j = 1; j < nrnmpi_numprocs_bbs; ++j) {
        send_context_->insert(j);
    }
    LookingToDoList::iterator i = looking_todo_->begin();
    while (i != looking_todo_->end()) {
        cid = (*i);
        looking_todo_->erase(i);
#if debug
        printf("sending context to already waiting %d\n", cid);
#endif
        nrnmpi_bbssend(cid, CONTEXT + 1, send);
        i = send_context_->find(cid);
        send_context_->erase(i);
        --remaining_context_cnt_;
        i = looking_todo_->begin();
    }
    if (remaining_context_cnt_ > 0) {
        context_buf_ = send;
        nrnmpi_ref(context_buf_);
        handle();
    }
}

void nrnbbs_context_wait() {
    if (BBSImpl::is_master_) {
        BBSDirectServer::server_->context_wait();
    }
}

void BBSDirectServer::context_wait() {
    // printf("context_wait enter %d\n", remaining_context_cnt_);
    while (remaining_context_cnt_) {
        handle();
    }
    // printf("context_wait exit %d\n", remaining_context_cnt_);
}

bool BBSDirectServer::send_context(int cid) {
    LookingToDoList::iterator i = send_context_->find(cid);
    if (i != send_context_->end()) {
        send_context_->erase(i);
#if debug
        printf("sending context to %d\n", cid);
#endif
        nrnmpi_bbssend(cid, CONTEXT + 1, context_buf_);
        if (--remaining_context_cnt_ <= 0) {
            nrnmpi_unref(context_buf_);
            context_buf_ = nullptr;
        }
        return true;
    }
    return false;
}

void BBSDirectServer::post_result(int id, bbsmpibuf* send) {
#if debug
    printf("DirectServer::post_result id=%d send=%p\n", id, send);
#endif
    WorkList::iterator i = work_->find(id);
    WorkItem* w = (WorkItem*) ((*i).second);
    nrnmpi_ref(send);
    nrnmpi_unref(w->buf_);
    w->buf_ = send;
    results_->insert(std::pair<const int, const WorkItem*>(w->parent_ ? w->parent_->id_ : 0, w));
}

int BBSDirectServer::look_take_todo(bbsmpibuf** recv) {
#if debug
    printf("DirectServer::look_take_todo\n");
#endif
    nrnmpi_unref(*recv);
    *recv = nullptr;
    ReadyList::iterator i = todo_->begin();
    if (i != todo_->end()) {
        WorkItem* w = (WorkItem*) (*i);
        todo_->erase(i);
        *recv = w->buf_;
#if debug
        printf("DirectServer::look_take_todo recv %p with keypos=%d return %d\n",
               *recv,
               (*recv)->keypos,
               w->id_);
#endif
        w->buf_ = 0;
        return w->id_;
    } else {
        return 0;
    }
}

int BBSDirectServer::look_take_result(int pid, bbsmpibuf** recv) {
#if debug
    printf("DirectServer::look_take_result pid=%d\n", pid);
#endif
    nrnmpi_unref(*recv);
    *recv = nullptr;
    ResultList::iterator i = results_->find(pid);
    if (i != results_->end()) {
        WorkItem* w = (WorkItem*) ((*i).second);
        results_->erase(i);
        *recv = w->buf_;
        int id = w->id_;
        WorkList::iterator j = work_->find(id);
        work_->erase(j);
        delete w;
#if debug
        printf("DirectServer::look_take_result recv=%p return %d\n", *recv, id);
#endif
        return id;
    } else {
        return 0;
    }
}

#endif  // NRNMPI
