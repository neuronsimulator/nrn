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

#include <string>

static char* newstr(const char* s) {
    char* s1 = new char[strlen(s) + 1];
    strcpy(s1, s);
    return s1;
}

WorkItem::WorkItem(int id, MessageValue* m)
    : id_(id), val_(m)
{
    val_->ref();
}

WorkItem::~WorkItem() {
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
    return w1->id_ < w2->id_;
}

void MessageValue::pkint(int i) {
    unpack_.emplace(i);
}

void MessageValue::pkdouble(double x) {
    unpack_.emplace(x);
}

void MessageValue::pkvec(int n, double* x) {
    unpack_.emplace(std::vector<double>(x, x + n));
}

void MessageValue::pkstr(const char* str) {
    unpack_.emplace(std::string(str));
}

void MessageValue::pkpickle(const char* bytes, size_t n) {
    unpack_.emplace(std::string(bytes, n));
}

int MessageValue::upkint(int* i) {
    const MessageItem& mi = unpack_.front();
    if (const auto* val = std::get_if<int>(&mi)) {
        *i = *val;
        unpack_.pop();
        return 0;
    }
    return -1;
}

int MessageValue::upkdouble(double* d) {
    const MessageItem& mi = unpack_.front();
    if (const auto* val = std::get_if<double>(&mi)) {
        *d = *val;
        unpack_.pop();
        return 0;
    }
    return -1;
}

int MessageValue::upkvec(int n, double* d) {
    const MessageItem& mi = unpack_.front();
    if (const auto* val = std::get_if<std::vector<double>>(&mi)) {
        for (size_t i = 0; i < n; ++i) {
            d[i] = (*val)[i];
        }
        unpack_.pop();
        return 0;
    }
    return -1;
}

int MessageValue::upkstr(char* s) {
    const MessageItem& mi = unpack_.front();
    if (const auto* val = std::get_if<std::string>(&mi)) {
        strcpy(s, (*val).c_str());
        unpack_.pop();
        return 0;
    }
    return -1;
}

int MessageValue::upkpickle(char* s, size_t* n) {
    const MessageItem& mi = unpack_.front();
    if (const auto* val = std::get_if<std::string>(&mi)) {
        *n = val->size();
        memcpy(s, (*val).c_str(), *n);
        unpack_.pop();
        return 0;
    }
    return -1;
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
        return true;
    }
    return false;
}

bool BBSLocalServer::look(const char* key, MessageValue** val) {
    MessageList::iterator m = messages_->find(key);
    if (m != messages_->end()) {
        *val = (MessageValue*) ((*m).second);
        Resource::ref(*val);
        return true;
    } else {
        val = nullptr;
    }
    return false;
}

void BBSLocalServer::post(const char* key, MessageValue* val) {
    MessageList::iterator m = messages_->insert(
        std::pair<const char* const, const MessageValue*>(newstr(key), val));
    Resource::ref(val);
}

void BBSLocalServer::post_todo(int parentid, MessageValue* val) {
    WorkItem* w = new WorkItem(next_id_++, val);
    WorkList::iterator p = work_->find(parentid);
    if (p != work_->end()) {
        w->parent_ = (WorkItem*) ((*p).second);
    }
    work_->insert(std::pair<const int, const WorkItem*>(w->id_, w));
    todo_->insert(w);
}

void BBSLocalServer::post_result(int id, MessageValue* val) {
    WorkList::iterator i = work_->find(id);
    WorkItem* w = (WorkItem*) ((*i).second);
    val->ref();
    w->val_->unref();
    w->val_ = val;
    results_->insert(std::pair<const int, const WorkItem*>(w->parent_ ? w->parent_->id_ : 0, w));
}

int BBSLocalServer::look_take_todo(MessageValue** m) {
    ReadyList::iterator i = todo_->begin();
    if (i != todo_->end()) {
        WorkItem* w = (*i);
        todo_->erase(i);
        *m = w->val_;
        w->val_->ref();
        return w->id_;
    } else {
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
        delete w;
        return id;
    } else {
        return 0;
    }
}
