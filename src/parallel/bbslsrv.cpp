#include <../../nrnconf.h>
#include <stdio.h>
#include <string.h>
#include "bbslsrv.h"
#include "oc_ansi.h"

#define INT    1
#define DOUBLE 2
#define STRING 3
#define VECTOR 4
#define PICKLE 5

#include <map>
#include <set>
#include <string>

// debug is 0 1 or 2
#define debug 0

class WorkItem {
  public:
    WorkItem(int id, MessageValue*);
    virtual ~WorkItem();
    WorkItem* parent_;
    int id_;
    MessageValue* val_;
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


WorkItem::WorkItem(int id, MessageValue* m) {
#if debug == 2
    printf("WorkItem %d\n", id);
#endif
    id_ = id;
    val_ = m;
    val_->ref();
    parent_ = nullptr;
}

WorkItem::~WorkItem() {
#if debug
    printf("~WorkItem %d\n", id_);
#endif
    val_->unref();
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

class MessageList: public std::multimap<const char*, const MessageValue*, ltstr> {};
class WorkList: public std::map<int, const WorkItem*, ltint> {};
class ReadyList: public std::set<WorkItem*, ltWorkItem> {};
class ResultList: public std::multimap<int, const WorkItem*, ltint> {};

MessageItem::MessageItem() {
    next_ = nullptr;
    type_ = 0;
}

MessageItem::~MessageItem() {
    switch (type_) {
    case STRING:
        delete[] u.s;
        break;
    case VECTOR:
        delete[] u.pd;
        break;
    case PICKLE:
        delete[] u.s;
        break;
    }
}

MessageValue::MessageValue() {
    first_ = nullptr;
    last_ = nullptr;
    unpack_ = nullptr;
}

MessageValue::~MessageValue() {
    MessageItem *mi, *next;
    for (mi = first_; mi; mi = next) {
        next = mi->next_;
        delete mi;
    }
}

MessageItem* MessageValue::link() {
    MessageItem* mi = new MessageItem();
    if (last_) {
        last_->next_ = mi;
    } else {
        first_ = mi;
        unpack_ = mi;
    }
    last_ = mi;
    return mi;
}

void MessageValue::init_unpack() {
    unpack_ = first_;
}

int MessageValue::pkint(int i) {
    MessageItem* m = link();
    m->type_ = INT;
    m->u.i = i;
    return 0;
}

int MessageValue::pkdouble(double x) {
    MessageItem* m = link();
    m->type_ = DOUBLE;
    m->u.d = x;
    return 0;
}

int MessageValue::pkvec(int n, double* x) {
    int i;
    MessageItem* m = link();
    m->type_ = VECTOR;
    m->u.pd = new double[n];
    for (i = 0; i < n; ++i) {
        m->u.pd[i] = x[i];
    }
    return 0;
}

int MessageValue::pkstr(const char* str) {
    MessageItem* m = link();
    m->type_ = STRING;
    m->u.s = new char[strlen(str) + 1];
    strcpy(m->u.s, str);
    return 0;
}

int MessageValue::pkpickle(const char* bytes, size_t n) {
    MessageItem* m = link();
    m->type_ = PICKLE;
    m->u.s = new char[n];
    m->size_ = n;
    memcpy(m->u.s, bytes, n);
    return 0;
}

int MessageValue::upkint(int* i) {
    if (!unpack_ || unpack_->type_ != INT) {
        return -1;
    }
    *i = unpack_->u.i;
    unpack_ = unpack_->next_;
    return 0;
}

int MessageValue::upkdouble(double* d) {
    if (!unpack_ || unpack_->type_ != DOUBLE) {
        return -1;
    }
    *d = unpack_->u.d;
    unpack_ = unpack_->next_;
    return 0;
}

int MessageValue::upkvec(int n, double* d) {
    int i;
    if (!unpack_ || unpack_->type_ != VECTOR) {
        return -1;
    }
    for (i = 0; i < n; ++i) {
        d[i] = unpack_->u.pd[i];
    }
    unpack_ = unpack_->next_;
    return 0;
}

int MessageValue::upkstr(char* s) {
    if (!unpack_ || unpack_->type_ != STRING) {
        return -1;
    }
    strcpy(s, unpack_->u.s);
    unpack_ = unpack_->next_;
    return 0;
}

int MessageValue::upkpickle(char* s, size_t* n) {
    if (!unpack_ || unpack_->type_ != PICKLE) {
        return -1;
    }
    *n = unpack_->size_;
    memcpy(s, unpack_->u.s, *n);
    unpack_ = unpack_->next_;
    return 0;
}

BBSLocalServer::BBSLocalServer() {
    messages_ = new MessageList();
    work_ = new WorkList();
    todo_ = new ReadyList();
    results_ = new ResultList();
    next_id_ = 1;
}

BBSLocalServer::~BBSLocalServer() {
    delete todo_;
    delete results_;

    printf("~BBSLocalServer not deleting everything\n");
    // need to unref MessageValue in messages_ and delete WorkItem in work_
    delete messages_;
    delete work_;
}

bool BBSLocalServer::look_take(const char* key, MessageValue** val) {
    MessageList::iterator m = messages_->find(key);
    if (m != messages_->end()) {
        *val = (MessageValue*) ((*m).second);
        char* s = (char*) ((*m).first);
        messages_->erase(m);
        delete[] s;
#if debug
        printf("srvr_look_take |%s|\n", key);
#endif
        return true;
    }
#if debug
    printf("fail srvr_look_take |%s|\n", key);
#endif
    return false;
}

bool BBSLocalServer::look(const char* key, MessageValue** val) {
    MessageList::iterator m = messages_->find(key);
    if (m != messages_->end()) {
        *val = (MessageValue*) ((*m).second);
        Resource::ref(*val);
#if debug
        printf("srvr_look true |%s|\n", key);
#endif
        return true;
    } else {
        val = nullptr;
    }
#if debug
    printf("srvr_look false |%s|\n", key);
#endif
    return false;
}

void BBSLocalServer::post(const char* key, MessageValue* val) {
    MessageList::iterator m = messages_->insert(
        std::pair<const char* const, const MessageValue*>(newstr(key), val));
    Resource::ref(val);
#if debug
    printf("srvr_post |%s|\n", key);
#endif
}

void BBSLocalServer::post_todo(int parentid, MessageValue* val) {
    WorkItem* w = new WorkItem(next_id_++, val);
    WorkList::iterator p = work_->find(parentid);
    if (p != work_->end()) {
        w->parent_ = (WorkItem*) ((*p).second);
    }
    work_->insert(std::pair<const int, const WorkItem*>(w->id_, w));
    todo_->insert(w);
#if debug
    printf("srvr_post_todo id=%d pid=%d\n", w->id_, parentid);
#endif
}

void BBSLocalServer::post_result(int id, MessageValue* val) {
    WorkList::iterator i = work_->find(id);
    WorkItem* w = (WorkItem*) ((*i).second);
    val->ref();
    w->val_->unref();
    w->val_ = val;
    results_->insert(std::pair<const int, const WorkItem*>(w->parent_ ? w->parent_->id_ : 0, w));
#if debug
    printf("srvr_post_done id=%d pid=%d\n", id, w->parent_ ? w->parent_->id_ : 0);
#endif
}

int BBSLocalServer::look_take_todo(MessageValue** m) {
    ReadyList::iterator i = todo_->begin();
    if (i != todo_->end()) {
        WorkItem* w = (*i);
        todo_->erase(i);
        *m = w->val_;
#if debug
        printf("srvr look_take_todo %d\n", w->id_);
#endif
        w->val_->ref();
        return w->id_;
    } else {
#if debug
        printf("srvr look_take_todo failed\n");
#endif
        return 0;
    }
}

int BBSLocalServer::look_take_result(int pid, MessageValue** m) {
    ResultList::iterator i = results_->find(pid);
    if (i != results_->end()) {
        WorkItem* w = (WorkItem*) ((*i).second);
        results_->erase(i);
        *m = w->val_;
        w->val_->ref();
        int id = w->id_;
        WorkList::iterator j = work_->find(id);
        work_->erase(j);
#if debug
        printf("srvr look_take_result %d for parent %d\n", w->id_, pid);
#endif
        delete w;
        return id;
    } else {
#if debug
        printf("srvr look_take_result failed for parent %d\n", pid);
#endif
        return 0;
    }
}
