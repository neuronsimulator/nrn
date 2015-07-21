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

class VoidPQueue {
public:
  VoidPQueue(int n);
  virtual ~VoidPQueue();
  void put(void*, int);
  void* get(int&);
private:
  void resize(int n);
  MUTDEC;
  int size_, head_, tail_;
  void** q_;
  int* type_;
};

VoidPQueue::VoidPQueue(int n) {
  MUTCONSTRUCT(1);
  head_ = 0;
  tail_ = 0;
  size_ = (n > 0) ? n : 10;
  q_ = new void*[size_];
  type_ = new int[size_];
}
VoidPQueue::~VoidPQueue() {
  MUTDESTRUCT;
  if (size_) {
    delete [] q_;
    delete [] type_;
  }
}
void VoidPQueue::put(void* v, int type) {
  MUTLOCK;
  q_[head_] = v;  
  type_[head_] = type;
  head_++;
  if (head_ >= size_) {
    head_ = 0;
  }
  if (head_ == tail_) {
    resize(size_*2);
  }
  MUTUNLOCK;
}
void* VoidPQueue::get(int& type) {
  MUTLOCK;
  void* v = NULL;
  type = 0;
  if (head_ != tail_) {
    v = q_[tail_];
    type = type_[tail_];
    tail_++;
    if (tail_ >= size_) {
      tail_ = 0;
    }
  }
  MUTUNLOCK;
  return v;
}
void VoidPQueue::resize(int n) {
  void** qold = q_;
  int* typeold = type_;
  q_ = new void*[n];
  type_ = new int[n];
  for (int i=0; i < size_; ++i) {
    q_[i] = qold[head_];
    type_[i] = typeold[head_];
    head_++;
    if (head_ >= size_) {
      head_ = 0;
    }
  }  
  delete [] qold;
  delete [] typeold;
  tail_ = 0;
  head_ = size_-1;
  size_ = n;
}


extern "C" {
static int bind_tid_;
void nrniv_bind_thread(void);
extern int (*iv_bind_enqueue_)(void* w, int type);
extern void iv_bind_call(void* w, int type);
static VoidPQueue* bindq_;

int nrn_is_gui_thread() {
	if (bindq_ && GetCurrentThreadId() != bind_tid_) {
		return 0;
	}
	return 1;
}

static int iv_bind_enqueue(void* w, int type) {
	//printf("iv_bind_enqueue %p thread %d\n", w, GetCurrentThreadId());
	if (GetCurrentThreadId() == bind_tid_) {
		return 0;
	}
	bindq_->put(w, type);
	return 1;
}

void nrniv_bind_call() {
	if (!bindq_) { return; }
	void* w;
	int type;
	while((w = bindq_->get(type)) != NULL) {
		//printf("nrniv_bind_call %p\n", w);
		iv_bind_call(w, type);
	}
}

void nrniv_bind_thread() {
IFGUI
	bindq_ = new VoidPQueue(10);
        bind_tid_ = int(*hoc_getarg(1));
        //printf("nrniv_bind_thread %d\n", bind_tid_);
	iv_bind_enqueue_ = iv_bind_enqueue;
ENDGUI
        hoc_pushx(1.);
        hoc_ret();
}
} // end of extern "C"

#endif // MINGW

#endif //HAVE_IV
