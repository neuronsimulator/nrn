#include <../../nrnconf.h>

#if HAVE_IV
#include <IV-Win/MWlib.h>
#if defined(_MSC_VER)
#undef min
#undef max
#undef near
#define near mynear

#else
#undef near
#endif

#undef MOVE
#undef DELETE
#undef IGNORE
#define MOVE mlhmove
#define DELETE mlhdelete
#include <Dispatch/dispatcher.h>
#include <InterViews/window.h>
#include <IV-Win/window.h>
#include <InterViews/display.h>
#include <InterViews/cursor.h>
#include <IV-Win/canvas.h>
#include "apwindow.h"
#include "ivoc.h"
#include "rubband.h"
#include "symdir.h"
#include "oc2iv.h"
#undef max
#undef min
#include "graph.h"

#if defined(CYGWIN)
// the link step needs liboc after libivoc but liboc refers to some
// things in libivoc that wouldn't normally be linked because nothing
// refers to them while libivoc is linking. So force them to link here
extern "C" {
int ivoc_list_look(Object*, Object*, char*, int);
void hoc_class_registration();
}
static void dummy() {
	ivoc_list_look(NULL, NULL, NULL, 0);
	hoc_class_registration();
}
#endif

int iv_mswin_to_pixel(int);
int iv_pixel_to_mswin(int);

void pwmimpl_redraw(Window* w) {
	w->rep()->WMpaint(0, 0);
}

void ivoc_bring_to_top(Window* w) {
	BringWindowToTop(w->rep()->msWindow());
}

void* mswin_setclip(Canvas* c, int x0, int y0, int x1, int y1) {
	HRGN clip = CreateRectRgn(x0, y0, x1, y1);
	SelectClipRgn(((MWcanvas*)c)->hdcOf(), clip);
	return clip;
}

void mswin_delete_object(void* v) {
	DeleteObject((HRGN)v);	
}

//---------------------------------------------------------

extern "C" {
void ivoc_win32_cleanup() {
	Oc::cleanup();
}
}

void Oc::cleanup() {
	if (help_cursor_) {
		delete help_cursor_;
	}
}

void PrintableWindow::hide() {
	if (is_mapped()) {
		HWND hwnd = Window::rep()->msWindow();
//printf("hide %p\n", this);
      ShowWindow(hwnd, SW_HIDE);
	}
}

void PrintableWindow::xmove(int x, int y) {
	HWND hwnd = Window::rep()->msWindow();
	//int width = canvas()->pwidth();
	//int height = canvas()->pheight();
	RECT r;
	GetWindowRect(hwnd, &r);
	MoveWindow(hwnd,
		iv_pixel_to_mswin(x), iv_pixel_to_mswin(y),
		 r.right - r.left, r.bottom - r.top, TRUE);
}
int PrintableWindow::xleft() const {
	WindowRep& w = *Window::rep();
	if (w.bound()) {
		HWND hwnd = w.msWindow();
		RECT winRect;
		GetWindowRect(hwnd, &winRect);
		return iv_mswin_to_pixel(winRect.left);
	}else{
		return 0;
	}
}
int PrintableWindow::xtop() const {
	WindowRep& w = *Window::rep();
	if (w.bound()) {
		HWND hwnd = w.msWindow();
		RECT winRect;
		GetWindowRect(hwnd, &winRect);
		return iv_mswin_to_pixel(winRect.top);
	}else{
		return 0;
	}
}

void PrintableWindow::xplace(int x, int y) {
	WindowRep& wr = *Window::rep();
	if (wr.bound()) {
		xmove(x, y);
	}else{
		xplace_ = true;
		xleft_ = x;
		xtop_ = y;
	}
}
void PrintableWindow::default_geometry() {
	 DismissableWindow::default_geometry();
	 if (xplace_) {
		pplace(iv_pixel_to_mswin(xleft_),
			display()->pheight() - iv_pixel_to_mswin(xtop_)
			- canvas()->to_pixels(height(),Dimension_Y));
	 }
}
#if 0
Object** Graph::new_vect(const DataVec*) {
	return 0;
}
#endif
//#include "\nrn\src\mswin\winio\debug.h"
void Rubberband::rubber_on(Canvas* c) {
//	DebugEnterMessage("Rubberband::rubber_on\n");
	c->front_buffer();
}
void Rubberband::rubber_off(Canvas* c) {
	c->back_buffer();
#ifdef WIN32
// this prevents failure for all future paints
	c->damage_all();
#endif
//	DebugExitMessage("Rubberband::rubber_off\n");
}
#if 0
double* ivoc_vector_ptr(Object*, int) {return 0;}
int ivoc_vector_size(Object*) {return 0;}
#endif

#ifdef MINGW
IOHandler::IOHandler(){}
IOHandler::~IOHandler(){}
int IOHandler::inputReady(int){return 0;}
int IOHandler::outputReady(int){return 0;}
int IOHandler::exceptionRaised(int){return 0;}
void IOHandler::timerExpired(long, long){}
void IOHandler::childStatus(pid_t, int){}
#endif // MINGW

#ifdef MINGW
extern "C" {
int stdin_event_ready();
}
int stdin_event_ready() {
  static DWORD main_threadid = -1;
  if (main_threadid == -1) {
	main_threadid = GetCurrentThreadId();
	return 1;
  }
  PostThreadMessage(main_threadid, WM_QUIT, 0, 0);
  return 1;
}

#include <nrnmutdec.h>
extern "C" {
static int bind_tid_;
void nrniv_bind_thread(void);
extern int (*iv_bind_enqueue_)(void* w, int type);
extern void iv_bind_call(void* w, int type);

extern void* (*nrnpy_save_thread)();
extern void (*nrnpy_restore_thread)(void*);

static void* w_;
static int type_;
static pthread_mutex_t* mut_;
static pthread_cond_t* cond_;

int nrn_is_gui_thread() {
	if (cond_ && GetCurrentThreadId() != bind_tid_) {
		return 0;
	}
	return 1;
}

static int iv_bind_enqueue(void* w, int type) {
	//printf("iv_bind_enqueue %p thread %d\n", w, GetCurrentThreadId());
	if (GetCurrentThreadId() == bind_tid_) {
		return 0;
	}
	// wait for the gui thread to handle the operation
	void* gs = (*nrnpy_save_thread)();
	pthread_mutex_lock(mut_);
	w_ = w;
	type_ = type;
	while (w_) {
		pthread_cond_wait(cond_, mut_);
	}
	pthread_mutex_unlock(mut_);
	(*nrnpy_restore_thread)(gs);
	return 1;
}

void nrniv_bind_call() {
	if (!cond_) { return; }
	void* w;
	int type;
	pthread_mutex_lock(mut_);
	w = w_;
	type = type_;
	if (w) {
		w_ = NULL;
		type_ = 0;
		iv_bind_call(w, type);
		pthread_cond_signal(cond_);
	}
	pthread_mutex_unlock(mut_);
}

void nrniv_bind_thread() {
IFGUI
        bind_tid_ = int(*hoc_getarg(1));
        //printf("nrniv_bind_thread %d\n", bind_tid_);
	iv_bind_enqueue_ = iv_bind_enqueue;
	cond_ = new pthread_cond_t;
	mut_ = new pthread_mutex_t;
	pthread_cond_init(cond_, NULL);
	pthread_mutex_init(mut_, NULL);
	w_ = NULL;
	type_ = 0;
ENDGUI
        hoc_pushx(1.);
        hoc_ret();
}
} // end of extern "C"

#endif // MINGW

#endif //HAVE_IV
