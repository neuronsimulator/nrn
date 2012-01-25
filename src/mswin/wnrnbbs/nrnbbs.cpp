#include <OS/string.h>
#include <OS/list.h>
//#include <OS/table.h>
#include <stdio.h>
#include <string.h>
#include "nrnbbs.h"
//#include "/nrn/src/winio/debug.h"

extern "C" {
void nrnbbs_server_post(const char* name, const char* value);
void nrnbbs_server_take(const char* name, char* value);
void nrnbbs_server_admin_post(const char* name, const char* value);
void nrnbbs_server_admin_request(const char* name, char* value);
void nrnbbs_show_postings();
void HandleOutput ( const char *);
}

class NrnMessage {
public:
	NrnMessage(const char* name, const char* value);
	virtual ~NrnMessage();
	const String& message();
	const char* name();
	bool equal(const char*);
	const char* value();
private:
	CopyString s_;
	CopyString value_;
};

declarePtrList(NrnMessageList, NrnMessage)
implementPtrList(NrnMessageList, NrnMessage)

class NrnBBSImpl {
public:
	NrnBBSImpl();
	virtual ~NrnBBSImpl();
	long lookup(const char*);
public:
	NrnMessageList ml_;
};

static NrnBBSImpl* bi_;

bool nrnbbs_connect() {
	if (!bi_) {
		bi_ = new NrnBBSImpl();
	}
   return true;
}
void nrnbbs_disconnect() {
	if (bi_) {
		delete bi_;
		bi_ = nil;
	}
}

void nrnbbs_server_post(const char* name, const char* value) {
	NrnMessage* m;
	m = new NrnMessage(name, value);
	bi_->ml_.append(m);
char buf[256];
sprintf(buf, "%d post |%s| |%s|", bi_->ml_.count(), name, value);
HandleOutput(buf);
}

void nrnbbs_show_postings() {
	long i;
	if (bi_->ml_.count() == 0) {
		HandleOutput("NrnBBS is empty");
	}
	for (i = 0; i < bi_->ml_.count(); ++i) {
		char buf[256];
		NrnMessage* m = bi_->ml_.item(i);
		sprintf(buf, "%d |%s| |%s|", i+1, m->name(), m->value());
		HandleOutput(buf);
	}
}

void nrnbbs_server_take(const char* name, char* value) {
	long i;
	i = bi_->lookup(name+1);
	if (i >= 0) {
		sprintf(value, "1%s", bi_->ml_.item(i)->value());
		if (name[0] == '0') { // only look if 1, don't take
			bi_->ml_.remove(i);
char buf[256];
sprintf(buf, "%d take |%s| |%s|", bi_->ml_.count(), name, value);
HandleOutput(buf);
		}
#if 1
		 else{
char buf[256];
sprintf(buf, "%d look |%s| |%s|", bi_->ml_.count(), name, value);
HandleOutput(buf);
		}
#endif
	}else{
		sprintf(value, "0");
	}
}

void nrnbbs_server_admin_post(const char* name, const char* value) {
	NrnMessage* m;
	m = new NrnMessage(name, value);
	bi_->ml_.append(m);
//DebugMessage("%d post |%s| |%s|\n", bi_->ml_.count(), name, value);
}

void nrnbbs_server_admin_request(const char* name, char* value) {
	long i;
	i = bi_->lookup(name+1);
	if (i >= 0) {
		sprintf(value, "1%s", bi_->ml_.item(i)->value());
		if (name[0] == '0') { // only look if 1, don't take
			bi_->ml_.remove(i);
//DebugMessage("%d take |%s| |%s|\n", bi_->ml_.count(), name, value);
		}
	}else{
		sprintf(value, "0");
	}
}

NrnBBSImpl::NrnBBSImpl() { }
NrnBBSImpl::~NrnBBSImpl() {
	long i, cnt = ml_.count();
	for (i = 0; i < cnt; ++i) {
		delete ml_.item(i);
	}
}

long NrnBBSImpl::lookup(const char* name) {
	long i, cnt;
	cnt = ml_.count();
	for (i = 0; i < cnt; ++i) {
		if (ml_.item(i)->equal(name)) {
			return i;
		}
	}
	return -1;
}

NrnMessage::NrnMessage(const char* name, const char* value){
	s_ = name;
	value_ = value;
}

NrnMessage::~NrnMessage() {
}

bool NrnMessage::equal(const char* name) {
	if (strcmp(name, s_.string()) == 0){
		return true;
	}else{
		return false;
	}
}

const char*  NrnMessage::value() {
	return value_.string();
}

const char*  NrnMessage::name() {
	return s_.string();
}
