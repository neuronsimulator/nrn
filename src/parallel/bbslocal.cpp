#include <../../nrnconf.h>
#include <InterViews/resource.h>
#include "oc2iv.h"
#include "bbslocal.h"
#include "bbslsrv.h"
#include <nrnmpi.h>

#include <map>
#include <set>
#include <string>

struct ltint {
    bool operator()(int i, int j) const {
        return i < j;
    }
};

class KeepArgs: public std::map<int, const MessageValue*, ltint> {};

static MessageValue* posting_;
static MessageValue* taking_;
static BBSLocalServer* server_;

BBSLocal::BBSLocal() {
    if (!server_) {
        server_ = new BBSLocalServer();
        posting_ = nullptr;
        taking_ = nullptr;
    }
    start();
    keepargs_ = new KeepArgs();
}

BBSLocal::~BBSLocal() {
    // need to unref anything in keepargs_;
    delete keepargs_;
}

void BBSLocal::context() {}

void BBSLocal::perror(const char* s) {
    hoc_execerror("BBSLocal error in ", s);
}

int BBSLocal::upkint() {
    int i{};
    if (!taking_ || taking_->upkint(&i))
        perror("upkint");
    return i;
}

double BBSLocal::upkdouble() {
    double x{};
    if (!taking_ || taking_->upkdouble(&x)) {
        perror("upkdouble");
    }
    return x;
}

void BBSLocal::upkvec(int n, double* x) {
    if (!taking_ || taking_->upkvec(n, x)) {
        perror("upkdouble");
    }
}

char* BBSLocal::upkstr() {
    int len{};
    char* s;
    if (!taking_ || taking_->upkint(&len)) {
        perror("upkstr length");
    }
    s = new char[len + 1];
    if (taking_->upkstr(s)) {
        perror("upkstr string");
    }
    return s;
}

char* BBSLocal::upkpickle(size_t* n) {
    int len{};
    char* s;
    if (!taking_ || taking_->upkint(&len)) {
        perror("upkpickle length");
    }
    s = new char[len];
    if (taking_->upkpickle(s, n)) {
        perror("upkpickle data");
    }
    assert(*n == len);
    return s;
}

void BBSLocal::pkbegin() {
    Resource::unref(posting_);
    posting_ = new MessageValue();
    posting_->ref();
}

void BBSLocal::pkint(int i) {
    if (!posting_ || posting_->pkint(i)) {
        perror("pkint");
    }
}

void BBSLocal::pkdouble(double x) {
    if (!posting_ || posting_->pkdouble(x)) {
        perror("pkdouble");
    }
}

void BBSLocal::pkvec(int n, double* x) {
    if (!posting_ || posting_->pkvec(n, x)) {
        perror("pkdouble");
    }
}

void BBSLocal::pkstr(const char* s) {
    if (!posting_ || posting_->pkint(strlen(s))) {
        perror("pkstr length");
    }
    if (!posting_ || posting_->pkstr(s)) {
        perror("pkstr string");
    }
}

void BBSLocal::pkpickle(const char* s, size_t n) {
    if (!posting_ || posting_->pkint(n)) {
        perror("pkpickle size");
    }
    if (!posting_ || posting_->pkpickle(s, n)) {
        perror("pkpickle data");
    }
}

void BBSLocal::post(const char* key) {
    server_->post(key, posting_);
    Resource::unref(posting_);
    posting_ = nullptr;
}

bool BBSLocal::look_take(const char* key) {
    Resource::unref(taking_);
    taking_ = nullptr;
    bool b = server_->look_take(key, &taking_);
    return b;
}

bool BBSLocal::look(const char* key) {
    Resource::unref(taking_);
    taking_ = nullptr;
    bool b = server_->look(key, &taking_);
    return b;
}

void BBSLocal::take(const char* key) {  // blocking
    int id;
    for (;;) {
        Resource::unref(taking_);
        taking_ = nullptr;
        if (server_->look_take(key, &taking_)) {
            return;
        } else if ((id = server_->look_take_todo(&taking_)) != 0) {
            execute(id);
        } else {
            perror("take blocking");
        }
    }
}

void BBSLocal::post_todo(int parentid) {
    server_->post_todo(parentid, posting_);
    Resource::unref(posting_);
    posting_ = nullptr;
}

void BBSLocal::post_result(int id) {
    server_->post_result(id, posting_);
    Resource::unref(posting_);
    posting_ = nullptr;
}

int BBSLocal::look_take_result(int pid) {
    Resource::unref(taking_);
    taking_ = nullptr;
    int id = server_->look_take_result(pid, &taking_);
    return id;
}

int BBSLocal::look_take_todo() {
    Resource::unref(taking_);
    taking_ = nullptr;
    int id = server_->look_take_todo(&taking_);
    return id;
}

int BBSLocal::take_todo() {
    Resource::unref(taking_);
    taking_ = nullptr;
    int id = look_take_todo();
    if (id == 0) {
        perror("take_todo blocking");
    }
    return id;
}

void BBSLocal::save_args(int userid) {
    server_->post_todo(working_id_, posting_);
    keepargs_->insert(std::pair<const int, const MessageValue*>(userid, posting_));
    posting_ = nullptr;
}

void BBSLocal::return_args(int userid) {
    KeepArgs::iterator i = keepargs_->find(userid);
    assert(i != keepargs_->end());
    Resource::unref(taking_);
    taking_ = (MessageValue*) ((*i).second);
    keepargs_->erase(i);
    taking_->init_unpack();
    BBSImpl::return_args(userid);
}

void BBSLocal::done() {
    BBSImpl::done();
}

void BBSLocal::start() {
    if (started_) {
        return;
    }
    BBSImpl::start();
    mytid_ = 1;
    is_master_ = true;
}
