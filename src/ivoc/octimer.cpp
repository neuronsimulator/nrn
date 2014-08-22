#include <../../nrnconf.h>

#if HAVE_IV
#include <Dispatch/iohandler.h>
#include <Dispatch/dispatcher.h>

#include <stdio.h>
#include "oc2iv.h"
#include "objcmd.h"
#endif  /* HAVE_IV */
#include "classreg.h"
#if HAVE_IV
#if carbon
class OcTimer {
#else
class OcTimer : public IOHandler {
#endif
public:
	OcTimer(const char*);
	virtual ~OcTimer();

	virtual void timerExpired(long, long);
	void start();
	void stop();
	double seconds();
	void seconds(double);
private:
	double seconds_;
	HocCommand* hc_;
#if carbon
	EventLoopTimerRef timer_;
#else
	bool stopped_;
#endif
};

#if carbon
static void timer_proc(EventLoopTimerRef, void* v) {
	((OcTimer*)v)->timerExpired(0,0);
}
#endif

#endif  /* HAVE_IV */

static double t_seconds(void* v) {
#if HAVE_IV
	OcTimer* t = (OcTimer*)v;
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
	OcTimer* t = (OcTimer*)v;
	t->start();
	return 0.;
#else 
	return 0.;
#endif /* HAVE_IV  */
}
static double t_stop(void* v) {
#if HAVE_IV
	OcTimer* t = (OcTimer*)v;
	t->stop();
	return 0.;
#else 
	return 0.;
#endif /* HAVE_IV  */
}
static void* t_cons(Object*) {
#if HAVE_IV
	return new OcTimer(gargstr(1));
#else 
	return (void*)0;
#endif /* HAVE_IV */
}
static void t_destruct(void* v) {
#if HAVE_IV
	OcTimer* t = (OcTimer*)v;
	delete t;
#endif  /* HAVE_IV */
}

Member_func t_members[] = {
	"seconds", t_seconds,
	"start", t_start,
	"end", t_stop,
	0, 0
};

void OcTimer_reg() {
	class2oc("Timer", t_cons, t_destruct, t_members, NULL, NULL, NULL);
}
#if HAVE_IV
OcTimer::OcTimer(const char* cmd) {
	hc_ = new HocCommand(cmd);
	seconds_ = .5;
#if carbon
	timer_ = 0;
#else
	stopped_ = true;
#endif
}
OcTimer::~OcTimer() {
	stop();
	delete hc_;
}
void OcTimer::start() {
#if carbon
	if (timer_) { return; }
	InstallEventLoopTimer(GetMainEventLoop(), seconds_, seconds_, timer_proc,
		(void*)this, &timer_);
#else
#ifdef MINGW
#else
	long s = long(seconds_);
	long us = long((seconds_ - double(s))*1000000.);
	stopped_ = false;
	Dispatcher::instance().startTimer(s, us, this);
#endif
#endif
}
void OcTimer::stop() {
#if carbon
	if (timer_) {
		RemoveEventLoopTimer(timer_);
		timer_ = 0;
	}
#else
#ifdef MINGW
#else
	stopped_ = true;
	Dispatcher::instance().stopTimer(this);
#endif
#endif
}
void OcTimer::timerExpired(long, long) {
#if carbon
#else
#ifdef MINGW
#else
	if (!stopped_) {
		this->start();
	}
#endif
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
