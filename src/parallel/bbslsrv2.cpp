#include <../../nrnconf.h>
#include "bbsconf.h"
#ifdef HAVE_PVM3_H	// to end of file
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "bbslsrv2.h"
#include "bbssrv.h"

#if defined(HAVE_PKMESG)
extern "C" { int pvm_pkmesgbody(int); };
#endif

BBSDirectServer* BBSDirectServer::server_;

#if defined(HAVE_STL)
#if defined(HAVE_SSTREAM) // the standard ...
#include <map>
#include <set>
#else
#include <pair.h>
#include <multimap.h>
#include <map.h>
#include <set.h>
#include <multiset.h>
#endif

#define debug 0

#define MessageList PvmMessageList
#define WorkItem PvmWorkItem
#define WorkList PvmWorkList
#define ReadyList PvmReadyList
#define ResultList PvmResultList
#define PendingList PvmPendingList
#define LookingToDoList PvmLookingToDoList

class WorkItem {
public:
	WorkItem(int id, int bufid, int cid);
	virtual ~WorkItem();
	WorkItem* parent_;
	int id_;
	int bufid_;
	int cid_; // pvm host id
	bool todo_less_than(const WorkItem*)const;
};

struct ltstr {
	bool operator() (const char* s1, const char* s2) const {
		return strcmp(s1, s2) < 0;
	}
};

struct ltint {
	bool operator() (int i, int j) const {
		return i < j;
	}
};

struct ltWorkItem {
	bool operator() (const WorkItem* w1, const WorkItem* w2) const {
		return w1->todo_less_than(w2);
	}
};

static char* newstr(const char* s) {
	char* s1 = new char[strlen(s) + 1];
	strcpy(s1, s);
	return s1;
}

WorkItem::WorkItem(int id, int bufid, int cid) {
#if debug == 2
printf("WorkItem %d\n", id);
#endif
	id_ = id;
	bufid_ = bufid;
	cid_ = cid;
	parent_ = nil;
}

WorkItem::~WorkItem() {
#if debug
printf("~WorkItem %d\n", id_);
#endif
}

bool WorkItem::todo_less_than(const WorkItem* w) const {
	WorkItem* w1 = (WorkItem*)this;
	WorkItem* w2 = (WorkItem*)w;
	while (w1->parent_ != w2->parent_) {
		if (w1->id_ < w2->id_) {
			w2 = w2->parent_;
		}else{
			w1 = w1->parent_;
		}
	}
#if debug
printf("todo_less_than %d < %d return %d\n", this->id_, w->id_, w1->id_ < w2->id_);
#endif
	return w1->id_ < w2->id_;
}

class MessageList : public multimap <const char*, const int, ltstr>{};
class PendingList : public multimap <const char*, const int, ltstr>{};
class WorkList : public map <int, const WorkItem*, ltint>{};
class LookingToDoList : public set <int, ltint>{};
class ReadyList : public set<const WorkItem*, ltWorkItem>{};
class ResultList: public multimap<int, const WorkItem*, ltint>{};
#else
class MessageList {};
class PendingList {};
class WorkList {};
class LookingToDoList {};
class ReadyList {};
class ResultList {};
#endif

BBSDirectServer::BBSDirectServer(){
#if defined(HAVE_STL)
	messages_ = new MessageList();
	work_ = new WorkList();
	todo_ = new ReadyList();
	results_ = new ResultList();
	pending_ = new PendingList();
	looking_todo_ = new LookingToDoList();
	send_context_ = new LookingToDoList();
	next_id_ = FIRSTID;
	context_buf_ = 0;
	remaining_context_cnt_ = 0;
#endif
}

BBSDirectServer::~BBSDirectServer(){
#if defined(HAVE_STL)
	delete todo_;
	delete results_;
	delete looking_todo_;
printf("~BBSLocalServer not deleting everything\n");
// need to unref MessageValue in messages_ and delete WorkItem in work_
	delete pending_;
	delete messages_;
	delete work_;
	delete send_context_;
#endif
}

bool BBSDirectServer::look_take(const char* key) {
	bool b = false;
#if defined(HAVE_STL)
	MessageList::iterator m = messages_->find(key);
	if (m != messages_->end()) {
		b = true;
		int buf = (*m).second;
		buf = pvm_setrbuf(buf);
//printf("free %d\n", buf);
		pvm_freebuf(buf);
		char* s = (char*)((*m).first);
		messages_->erase(m);
		delete [] s;
	}
#if debug
	if (b) {
		printf("DirectServer::look_take |%s| %d\n", key, pvm_getrbuf());
	}else{
		printf("DirectServer::look_take |%s| fail\n", key);
	}
#endif
#endif
	return b;
}

bool BBSDirectServer::look(const char* key) {
	bool b = false;
#if defined(HAVE_STL)
	MessageList::iterator m = messages_->find(key);
	if (m != messages_->end()) {
		b = true;
		int buf  = (*m).second;
		pvm_initsend(PvmDataDefault);
		pvm_pkmesgbody(buf);
		buf = pvm_getrbuf();
		pvm_setrbuf(pvm_setsbuf(pvm_mkbuf(PvmDataDefault)));
		pvm_freebuf(buf);
	}
//	printf("DirectServer::look %d |%s|\n", b, key);
#endif
	return b;
}

void BBSDirectServer::put_pending(const char* key, int cid) {
#if defined(HAVE_STL)
#if debug
printf("put_pending |%s| %d\n", key, cid);
#endif
	char* s = newstr(key);
	pending_->insert(pair<const char* const, const int>(s, cid));
#endif
}

bool BBSDirectServer::take_pending(const char* key, int* cid) {
	bool b = false;
#if defined(HAVE_STL)
	PendingList::iterator p = pending_->find(key);
	if (p != pending_->end()) {
		*cid = (*p).second;
//printf("take_pending |%s| %d\n", key, *cid);
		char* s =  (char*)((*p).first);
		pending_->erase(p);
		delete [] s;
		b = true;
	}
#endif
	return b;
}

void BBSDirectServer::post(const char* key) {
#if defined(HAVE_STL)
	int cid;
	int bufid = pvm_getsbuf();
//	printf("DirectServer::post |%s| bufid=%d\n", key, bufid);
	if (take_pending(key, &cid)) {
		pvm_send(cid, TAKE);
		pvm_freebuf(bufid);
	}else{
		MessageList::iterator m = messages_->insert(
		  pair<const char* const, const int>(newstr(key), bufid)
		);
	}
	pvm_setsbuf(pvm_mkbuf(PvmDataDefault));
#endif
}

void BBSDirectServer::add_looking_todo(int cid) {
#if defined(HAVE_STL)
	looking_todo_->insert(cid);
#endif
}

void BBSDirectServer::post_todo(int pid, int cid){
#if defined(HAVE_STL)
	int bufid = pvm_getsbuf();
	WorkItem* w = new WorkItem(next_id_++, bufid, cid);
	WorkList::iterator p = work_->find(pid);
	if (p != work_->end()) {
		w->parent_ = (WorkItem*)((*p).second);
	}
	work_->insert(pair<const int, const WorkItem*>(w->id_, w));
	LookingToDoList::iterator i = looking_todo_->begin();
	if (i != looking_todo_->end()) {
		cid = (*i);
		looking_todo_->erase(i);
		// the send buffer is correct
		pvm_send(cid, w->id_ + 1);
		w->bufid_ = 0;
		pvm_freebuf(pvm_getsbuf());
	}else{
		todo_->insert(w);
	}
	pvm_setsbuf(pvm_mkbuf(PvmDataDefault));
#endif
}

void  BBSDirectServer::context(int ncids, int* cids) {
#if defined(HAVE_STL)
	int cid, j;
//printf("ncids=%d\n", ncids);
	if (remaining_context_cnt_ > 0) {
		printf("some workers did not receive previous context\n");
//		send_context_->clear();
		send_context_->erase(send_context_->begin(), send_context_->end());
		pvm_freebuf(context_buf_);
	}
	remaining_context_cnt_ = ncids;
	for (j = 0; j < ncids; ++j) {
		send_context_->insert(cids[j]);
	}
	LookingToDoList::iterator i = looking_todo_->begin();
	while (i != looking_todo_->end()) {
		cid = (*i);
		looking_todo_->erase(i);
//printf("sending context to already waiting %x\n", cid);
		pvm_send(cid, CONTEXT+1);
		--ncids;
		i = send_context_->find(cid);
		send_context_->erase(i);
		--remaining_context_cnt_;
		i = looking_todo_->begin();
	}
	if (remaining_context_cnt_ > 0) {
		context_buf_ = pvm_setsbuf(pvm_mkbuf(PvmDataDefault));
	}else{
		pvm_freebuf(pvm_setsbuf(pvm_mkbuf(PvmDataDefault)));
	}
#endif
}

bool  BBSDirectServer::send_context(int cid) {
#if defined(HAVE_STL)
	LookingToDoList::iterator i = send_context_->find(cid);
	if (i != send_context_->end()) {
		send_context_->erase(i);
		int oldbuf = pvm_setsbuf(context_buf_);
//printf("sending context to %x\n", cid);
		pvm_send(cid, CONTEXT+1);
		pvm_setsbuf(oldbuf);
		if(--remaining_context_cnt_ <= 0) {
			pvm_freebuf(context_buf_);
		}
		return true;
	}
#endif
	return false;
}

void BBSDirectServer::post_result(int id){
#if defined(HAVE_STL)
	WorkList::iterator i = work_->find(id);
	WorkItem* w = (WorkItem*)((*i).second);
// deal with following when thinking about error recovery
// at this time it was freed as a receive buffer after the take_todo
//	pvm_freebuf(w->bufid_);
	w->bufid_ = pvm_setsbuf(pvm_mkbuf(PvmDataDefault));
	results_->insert(pair<const int, const WorkItem*>(w->parent_ ? w->parent_->id_ : 0, w));
#endif
}

int BBSDirectServer::look_take_todo() {
#if defined(HAVE_STL)
	ReadyList::iterator i = todo_->begin();
	if (i != todo_->end()) {
		WorkItem* w = (WorkItem*)(*i);
		todo_->erase(i);
		int oldbuf = pvm_setrbuf(w->bufid_);
		pvm_freebuf(oldbuf);
		w->bufid_ = 0;
		return w->id_;
	}else{
		return 0;
	}
#else
return 0;
#endif
}

int BBSDirectServer::look_take_result(int pid) {
#if defined(HAVE_STL)
	ResultList::iterator i = results_->find(pid);
	if (i != results_->end()) {
		WorkItem* w = (WorkItem*)((*i).second);
		results_->erase(i);
		int oldbuf = pvm_setrbuf(w->bufid_);
		pvm_freebuf(oldbuf);
		int id = w->id_;
		WorkList::iterator j = work_->find(id);
		work_->erase(j);
		delete w;
		return id;
	}else{
		return 0;
	}	
#else
return 0;
#endif
}

#endif //HAVE_PVM3_H
