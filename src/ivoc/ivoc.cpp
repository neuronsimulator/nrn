#include <../../nrnconf.h>

#include <vector>
#include <ocnotify.h>
#include <stdio.h>
#include <stdlib.h>
#include <nrnmutdec.h>
#include "oc2iv.h"
#include "ocfunc.h"

extern Object** (*nrnpy_gui_helper_)(const char* name, Object* obj);
extern double (*nrnpy_object_to_double_)(Object*);

#if HAVE_IV
#include "utility.h"
#include "ivoc.h"
#endif

#include "bimap.hpp"

#if USE_PTHREAD
static MUTDEC;
#endif

using PF = void (*)(void*, int);
using FList = std::vector<PF>;

static FList* f_list;

static nrn::tool::bimap<void*, Observer*>* pvob;
static nrn::tool::bimap<double*, Observer*>* pdob;

// fast insert, find, and remove of (double*, Observer*) using either as
// a key. Use pair of multimap since there can be many observers of the
// same double. And perhaps one Observer is watching several double*. Also
// the double* being watched is removed when the array (pd*, size) it is
// a part of is freed. So the upper_bound property is needed

int nrn_err_dialog_active_;


void* (*nrnpy_save_thread)();
void (*nrnpy_restore_thread)(void*);

void nrn_notify_freed(PF pf) {
    if (!f_list) {
        f_list = new FList;
    }
    f_list->push_back(pf);
}

void nrn_notify_when_void_freed(void* p, Observer* ob) {
    MUTLOCK
    if (!pvob) {
        pvob = new nrn::tool::bimap<void*, Observer*>();
    }
    pvob->insert(p, ob);
    MUTUNLOCK
}

void nrn_notify_when_double_freed(double* p, Observer* ob) {
    MUTLOCK
    if (!pdob) {
        pdob = new nrn::tool::bimap<double*, Observer*>();
    }
    pdob->insert(p, ob);
    MUTUNLOCK
}

void nrn_notify_pointer_disconnect(Observer* ob) {
    MUTLOCK
    if (pvob) {
        pvob->obremove(ob);
    }
    if (pdob) {
        pdob->obremove(ob);
    }
    MUTUNLOCK
}

void notify_pointer_freed(void* pt) {
    if (pvob) {
        MUTLOCK
        void* pv;
        Observer* ob;
        while (pvob->find(pt, pv, ob)) {
            ob->update(NULL);
            pvob->remove(pv, ob);
        }
        MUTUNLOCK
    }
}
void notify_freed(void* p) {
    if (f_list) {
        for (PF f: *f_list) {
            f(p, 1);
        }
    }
    notify_pointer_freed(p);
}
void notify_freed_val_array(double* p, size_t size) {
    if (f_list) {
        for (PF f: *f_list) {
            f((void*) p, size);
        }
    }
    if (pdob) {
        double* pp;
        Observer* ob;
        while (pdob->find(p, size, pp, ob)) {
            // printf("notify_freed_val_array %d %ld\n", size, j);
            ob->update(NULL);
            pdob->remove(pp, ob);
        }
    }
}

char* cxx_char_alloc(size_t sz) {
    char* cp = new char[sz];
    return cp;
}


#ifndef MINGW  // actual implementation in ivocwin.cpp
void nrniv_bind_thread(void);
void nrniv_bind_thread() {
    hoc_pushx(1.);
    hoc_ret();
}
#endif

void nrn_err_dialog(const char* mes) {
#if HAVE_IV
    IFGUI
    if (nrn_err_dialog_active_ && !Session::instance()->done()) {
        char m[1024];
        sprintf(m, "%s (See terminal window)", mes);
        continue_dialog(m);
    }
    ENDGUI
#endif
}

#if HAVE_IV  // to end of file

#include <InterViews/event.h>
#include <InterViews/reqerr.h>
#include <InterViews/style.h>
#include <IV-look/kit.h>

#include "xmenu.h"

/*
 * Interface between oc and interviews.
 *
 * The normal command driven oc can be simultaneously event driven if
 * instead of blocking on a terminal read, run_til_stdin() is called.
 * This runs the interviews event loop until something is typed in the
 * window from which oc was run.
 */

extern void hoc_main1_init(const char* pname, const char** env);
extern int hoc_oc(const char*);
extern int hoc_interviews;
extern Symbol* hoc_parse_expr(const char*, Symlist**);
extern double hoc_run_expr(Symbol*);
extern int hoc_execerror_messages;
extern void hoc_ret();
extern void hoc_pushx(double);
extern FILE* hoc_fin;
extern void ivoc_cleanup();
extern "C" void nrn_shape_update();
extern int bbs_poll_;
extern void bbs_handle();

int run_til_stdin();
void single_event_run();
void hoc_notify_iv();

extern int hoc_print_first_instance;
void ivoc_style();

// because NEURON can no longer maintain its own copy of dialogs.cpp
// we communicate with the InterViews version through a callback.
extern bool (*IVDialog_setAcceptInput)(bool);
bool setAcceptInputCallback(bool);
bool setAcceptInputCallback(bool b) {
    Oc oc;
    return oc.setAcceptInput(b);
}

void ivoc_style() {
    TRY_GUI_REDIRECT_DOUBLE("ivoc_style", NULL);
    IFGUI
    if (Session::instance()) {
        Style* s = Session::instance()->style();
        s->remove_attribute(gargstr(1));
        s->attribute(gargstr(1), gargstr(2), -5);
    }
#if 0
String s;
if (WidgetKit::instance()->style()->find_attribute(gargstr(1)+1, s)) {
	printf("ivoc_style %s: %s\n", gargstr(1), s.string());
}else{
	printf("couldn't find %s\n", gargstr(1));
}
#endif
    ENDGUI
    hoc_ret();
    hoc_pushx(1.);
}

#if !defined(MINGW) && !defined(MAC)
/*static*/ class ReqErr1: public ReqErr {
  public:
    ReqErr1();
    virtual void Error();
    virtual int count() {
        return count_;
    }

  private:
    int count_;
    int r_;
};

ReqErr1::ReqErr1() {
    count_ = 0;
    r_ = 0;
}

void ReqErr1::Error() {
    if (!count_ || code != r_) {
        if (!r_) {
            r_ = code;
        }
        fprintf(stderr, "X Error of failed request: %s\n", message);
        if (r_ == code) {
            fprintf(stderr, "Further messages for error code %d will not be shown\n", r_);
        }
    }
    ++count_;
}

static ReqErr1* reqerr1;
#endif

#if MAC
static HandleStdin* hsd_;
#endif

#ifdef MINGW
static HandleStdin* hsd_;
void winio_key_press() {
    hsd_->inputReady(1);
}

#endif

Oc::Oc() {
    MUTLOCK
    ++refcnt_;
    MUTUNLOCK
}

bool Oc::helpmode_;

Oc::Oc(Session* s, const char* pname, const char** env) {
    if (session_)
        return;
    refcnt_++;
    session_ = s;
    IVDialog_setAcceptInput = setAcceptInputCallback;
    notify_change_ = new Observable();
    if (s) {
        helpmode_ = false;
#if !defined(WIN32) && !defined(MAC)
        reqerr1 = new ReqErr1;
        reqerr1->Install();
#endif
#if defined(MINGW) || defined(MAC)
        hsd_ = handleStdin_ = new HandleStdin;
#else
        handleStdin_ = new HandleStdin;
        Dispatcher::instance().link(0, Dispatcher::ReadMask, handleStdin_);
        Dispatcher::instance().link(0, Dispatcher::ExceptMask, handleStdin_);
#endif
        hoc_interviews = 1;
#if MAC
        hoc_print_first_instance = 0;
#endif
        String str;
        if (session_->style()->find_attribute("first_instance_message", str)) {
            if (str == "on") {
                hoc_print_first_instance = 1;
            } else {
                hoc_print_first_instance = 0;
            }
        }
    }
    MUTCONSTRUCT(1)
    hoc_main1_init(pname, env);
}

Oc::~Oc() {
    MUTLOCK
    if (--refcnt_ == 0) {
#if !defined(MINGW) && !defined(MAC)
        if (reqerr1 && reqerr1->count()) {
            fprintf(stderr, "total X Errors: %d\n", reqerr1->count());
        }
#endif
    }
    MUTUNLOCK
}


Session* Oc::getSession() {
    return session_;
}

int Oc::run(int argc, const char** argv) {
    return hoc_main1(argc, argv, 0);
}

int Oc::run(const char* buf, bool show_err_mes) {
    hoc_execerror_messages = show_err_mes;
    return hoc_oc(buf);
}

Symbol* Oc::parseExpr(const char* expr, Symlist** ps) {
    return hoc_parse_expr(expr, ps);
}

double Oc::runExpr(Symbol* sym) {
    return hoc_run_expr(sym);
}

const char* Oc::name(Symbol* sym) {
    return sym->name;
}

bool Oc::setAcceptInput(bool b) {
    bool old = handleStdin_->acceptInput_;
    handleStdin_->acceptInput_ = b;
    return old;
}

void Oc::notify_freed(PF pf) {
    nrn_notify_freed(pf);
}

void Oc::notify_when_freed(void* p, Observer* ob) {
    nrn_notify_when_void_freed(p, ob);
}

void Oc::notify_when_freed(double* p, Observer* ob) {
    nrn_notify_when_double_freed(p, ob);
}

void Oc::notify_pointer_disconnect(Observer* ob) {
    nrn_notify_pointer_disconnect(ob);
}

HandleStdin::HandleStdin() {
    stdinSeen_ = false;
    acceptInput_ = true;
}

int HandleStdin::inputReady(int fd) {
    stdinSeen_ = 1;
    if (fd) {
        ;
    }
    if (acceptInput_) {
        Oc::getSession()->quit();
    }
    return 0;
}

int HandleStdin::exceptionRaised(int fd) {
    hoc_interviews = 0;
    if (fd) {
        ;
    }
    stdinSeen_ = 1;
    Oc::getSession()->quit();
    return 0;
}

void ivoc_cleanup() {}

int run_til_stdin() {
    Session* session = Oc::getSession();
#if defined(WIN32) || MAC
    Oc oc;
    oc.notify();
#endif
#ifndef WIN32
    Oc::setStdinSeen(false);
#endif
    session->run();
    WinDismiss::dismiss_defer();  // in case window was dismissed
#if MAC
    extern Boolean IVOCGoodLine;
    if (IVOCGoodLine) {
        return 1;
    } else {
        return 0;
    }
#endif
#ifdef WIN32
    return 0;
#else
    return Oc::getStdinSeen();  // MAC should not reach this point
#endif
}

void single_event_run() {
    Resource::flush();
    Session* session = Oc::getSession();
    Event e;
    // actually run till no more events
    Oc::setAcceptInput(false);
#if MAC
    extern bool read_if_pending(Event&);
    while (!session->done() && read_if_pending(e)) {
        e.handle();
    }
#else
    bool dsav = session->done();
    session->unquit();
    while (session->pending() && !session->done()) {
        session->read(e);
        e.handle();
    }
    if (dsav) {
        session->quit();
    }
#endif
    Oc::setAcceptInput(true);
    ;
    HocPanel::keep_updated();
#if MAC
    Session::instance()->screen_update();
#endif
    WinDismiss::dismiss_defer();  // in case window was dismissed
}

#ifdef MINGW
extern void nrniv_bind_call(void);
#endif

void hoc_notify_iv() {
    IFGUI
#ifdef MINGW
    if (!nrn_is_gui_thread()) {
        // allow gui thread to run
        nrnpy_pass();
        hoc_pushx(0.);
        hoc_ret();
        return;
    }
    nrniv_bind_call();
#endif
    Resource::flush();
    Oc oc;
    oc.notify();
    single_event_run();
    ENDGUI
    hoc_pushx(1.);
    hoc_ret();
}

Observable* Oc::notify_change_;

void Oc::notify() {  // merely a possible change
    nrn_shape_update();
    notifyHocValue();
    notify_change_->notify();
    if (bbs_poll_ > 0)
        bbs_handle();
    WinDismiss::dismiss_defer();  // in case window was dismissed
}
void Oc::notify_attach(Observer* o) {
    notify_change_->attach(o);
}
void Oc::notify_detach(Observer* o) {
    notify_change_->detach(o);
}
#endif
