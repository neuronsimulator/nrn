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
    : id_(id)
    , val_(m) {
    val_->ref();
}

WorkItem::~WorkItem() {
    val_->unref();
}

bool WorkItem::todo_less_than(const WorkItem* w) const {
    auto* w1 = const_cast<WorkItem*>(this);
    auto* w2 = const_cast<WorkItem*>(w);
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

bool BBSLocalServer::look_take(const char* key, MessageValue** val) {
    auto m = messages_.find(key);
    if (m != messages_.end()) {
        *val = const_cast<MessageValue*>((*m).second);
        char* s = (char*) ((*m).first);
        messages_.erase(m);
        delete[] s;
        return true;
    }
    return false;
}

bool BBSLocalServer::look(const char* key, MessageValue** val) {
    if (auto m = messages_.find(key); m != messages_.end()) {
        *val = const_cast<MessageValue*>((*m).second);
        Resource::ref(*val);
        return true;
    }
    val = nullptr;
    return false;
}

void BBSLocalServer::post(const char* key, MessageValue* val) {
    auto m = messages_.emplace(newstr(key), val);
    Resource::ref(val);
}

void BBSLocalServer::post_todo(int parentid, MessageValue* val) {
    WorkItem* w = new WorkItem(next_id_++, val);
    if (auto p = work_.find(parentid); p != work_.end()) {
        w->parent_ = const_cast<WorkItem*>(p->second);
    }
    work_.emplace(w->id_, w);
    todo_.insert(w);
}

void BBSLocalServer::post_result(int id, MessageValue* val) {
    auto i = work_.find(id);
    WorkItem* w = const_cast<WorkItem*>(i->second);
    val->ref();
    w->val_->unref();
    w->val_ = val;
    results_.emplace(w->parent_ ? w->parent_->id_ : 0, w);
}

int BBSLocalServer::look_take_todo(MessageValue** m) {
    if (todo_.empty()) {
        return 0;
    }

    auto it = todo_.begin();
    WorkItem* w = *it;
    todo_.erase(it);
    *m = w->val_;
    w->val_->ref();
    return w->id_;
}

int BBSLocalServer::look_take_result(int pid, MessageValue** m) {
    auto i = results_.find(pid);
    if (i != results_.end()) {
        WorkItem* w = const_cast<WorkItem*>(i->second);
        results_.erase(i);
        *m = w->val_;
        w->val_->ref();
        int id = w->id_;
        auto j = work_.find(id);
        work_.erase(j);
        delete w;
        return id;
    } else {
        return 0;
    }
}
