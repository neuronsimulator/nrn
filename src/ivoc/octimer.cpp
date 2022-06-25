#include <../../nrnconf.h>

#if HAVE_IV
#include <Dispatch/iohandler.h>
#include <Dispatch/dispatcher.h>

#include <stdio.h>
#include "oc2iv.h"
#include "objcmd.h"
#endif /* HAVE_IV */
#include "classreg.h"

#ifdef MINGW
#include <windows.h>
#endif

#if HAVE_IV
#if defined(MINGW)
class OcTimer {
#else
class OcTimer: public IOHandler {
#endif
  public:
    OcTimer(const char*);
    OcTimer(Object*);
    virtual ~OcTimer();

    virtual void timerExpired(long, long);
    void start();
    void stop();
    double seconds();
    void seconds(double);

  private:
    double seconds_;
    HocCommand* hc_;
#ifdef MINGW
    HANDLE wtimer_;
#endif
    bool stopped_;
};

#ifdef MINGW
static void CALLBACK callback(PVOID lpParameter, BOOLEAN TimerOrWaitFired) {
    ((OcTimer*) lpParameter)->timerExpired(0, 0);
}
#endif

#endif /* HAVE_IV */

static double t_seconds(void* v) {
#if HAVE_IV
    OcTimer* t = (OcTimer*) v;
    if (ifarg(1)) {
        t->seconds(chkarg(1, 1e-6, 1e6));
    }
    return double(t->seconds());
#else
    return 0.;
#endif /* HAVE_IV  */
}

static double t_start(void* v) {
#if HAVE_IV
    OcTimer* t = (OcTimer*) v;
    t->start();
    return 0.;
#else
    return 0.;
#endif /* HAVE_IV  */
}
static double t_stop(void* v) {
#if HAVE_IV
    OcTimer* t = (OcTimer*) v;
    t->stop();
    return 0.;
#else
    return 0.;
#endif /* HAVE_IV  */
}
static void* t_cons(Object*) {
#if HAVE_IV
    if (hoc_is_object_arg(1)) {
        return new OcTimer(*hoc_objgetarg(1));
    } else {
        return new OcTimer(gargstr(1));
    }
#else
    return nullptr;
#endif /* HAVE_IV */
}
static void t_destruct(void* v) {
#if HAVE_IV
    OcTimer* t = (OcTimer*) v;
    delete t;
#endif /* HAVE_IV */
}

Member_func t_members[] = {{"seconds", t_seconds}, {"start", t_start}, {"end", t_stop}, {0, 0}};

void OcTimer_reg() {
    class2oc("Timer", t_cons, t_destruct, t_members, NULL, NULL, NULL);
}
#if HAVE_IV
OcTimer::OcTimer(const char* cmd) {
    hc_ = new HocCommand(cmd);
    seconds_ = .5;
#ifdef MINGW
    wtimer_ = NULL;
#endif
    stopped_ = true;
}
OcTimer::OcTimer(Object* cmd) {
    hc_ = new HocCommand(cmd);
    seconds_ = .5;
#ifdef MINGW
    wtimer_ = NULL;
#endif
    stopped_ = true;
}
OcTimer::~OcTimer() {
    stop();
    delete hc_;
}
void OcTimer::start() {
#ifdef MINGW
    stopped_ = false;
    LARGE_INTEGER nsec100;
    nsec100.QuadPart = (long long) (-seconds_ * 10000000.);
    wtimer_ = CreateWaitableTimer(NULL, TRUE, NULL);
    while (stopped_ == false) {
        SetWaitableTimer(wtimer_, &nsec100, 0, NULL, NULL, 0);
        hc_->execute();
        WaitForSingleObject(wtimer_, INFINITE);
    }
    CloseHandle(wtimer_);
    wtimer_ = NULL;
#else
    long s = long(seconds_);
    long us = long((seconds_ - double(s)) * 1000000.);
    stopped_ = false;
    Dispatcher::instance().startTimer(s, us, this);
#endif
}
void OcTimer::stop() {
    stopped_ = true;
#ifndef MINGW
    Dispatcher::instance().stopTimer(this);
#endif
}
void OcTimer::timerExpired(long, long) {
#ifndef MINGW
    if (!stopped_) {
        this->start();
    }
#endif
    // want it to be part of interval just like on mac
    hc_->execute();
}
double OcTimer::seconds() {
    return seconds_;
}
void OcTimer::seconds(double sec) {
    seconds_ = sec;
}
#endif /* HAVE_IV */
