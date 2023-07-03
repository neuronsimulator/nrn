#include <../../nrnconf.h>
#include "oc2iv.h"

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
#define MOVE   mlhmove
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
#undef max
#undef min
#include "graph.h"

#include <condition_variable>
#include <mutex>

#ifdef MINGW
// the link step needs liboc after libivoc but liboc refers to some
// things in libivoc that wouldn't normally be linked because nothing
// refers to them while libivoc is linking. So force them to link here
int ivoc_list_look(Object*, Object*, char*, int);
void hoc_class_registration();
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
    SelectClipRgn(((MWcanvas*) c)->hdcOf(), clip);
    return clip;
}

void mswin_delete_object(void* v) {
    DeleteObject((HRGN) v);
}

//---------------------------------------------------------

void ivoc_win32_cleanup() {
    Oc::cleanup();
}

void Oc::cleanup() {
    if (help_cursor_) {
        delete help_cursor_;
    }
}

#ifdef MINGW
static void hidewindow(void* v) {
    HWND w = (HWND) v;
    ShowWindow(w, SW_HIDE);
}

static int gui_thread_xmove_x;
static int gui_thread_xmove_y;
void gui_thread_xmove(void* v) {
    PrintableWindow* w = (PrintableWindow*) v;
    w->xmove(gui_thread_xmove_x, gui_thread_xmove_y);
}

#endif

void PrintableWindow::hide() {
    if (is_mapped()) {
        HWND hwnd = Window::rep()->msWindow();
// printf("hide %p\n", this);
#ifdef MINGW
        if (!nrn_is_gui_thread()) {
            nrn_gui_exec(hidewindow, hwnd);
            return;
        }
#endif
        ShowWindow(hwnd, SW_HIDE);
    }
}

void PrintableWindow::xmove(int x, int y) {
#ifdef MINGW
    if (!nrn_is_gui_thread()) {
        gui_thread_xmove_x = x;
        gui_thread_xmove_y = y;
        nrn_gui_exec(gui_thread_xmove, this);
        return;
    }
#endif
    HWND hwnd = Window::rep()->msWindow();
    // int width = canvas()->pwidth();
    // int height = canvas()->pheight();
    RECT r;
    GetWindowRect(hwnd, &r);
    MoveWindow(
        hwnd, iv_pixel_to_mswin(x), iv_pixel_to_mswin(y), r.right - r.left, r.bottom - r.top, TRUE);
}
int PrintableWindow::xleft() const {
    WindowRep& w = *Window::rep();
    if (w.bound()) {
        HWND hwnd = w.msWindow();
        RECT winRect;
        GetWindowRect(hwnd, &winRect);
        return iv_mswin_to_pixel(winRect.left);
    } else {
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
    } else {
        return 0;
    }
}

void PrintableWindow::xplace(int x, int y) {
    WindowRep& wr = *Window::rep();
    if (wr.bound()) {
        xmove(x, y);
    } else {
        xplace_ = true;
        xleft_ = x;
        xtop_ = y;
    }
}
void PrintableWindow::default_geometry() {
    DismissableWindow::default_geometry();
    if (xplace_) {
        pplace(iv_pixel_to_mswin(xleft_),
               display()->pheight() - iv_pixel_to_mswin(xtop_) -
                   canvas()->to_pixels(height(), Dimension_Y));
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
IOHandler::IOHandler() {}
IOHandler::~IOHandler() {}
int IOHandler::inputReady(int) {
    return 0;
}
int IOHandler::outputReady(int) {
    return 0;
}
int IOHandler::exceptionRaised(int) {
    return 0;
}
void IOHandler::timerExpired(long, long) {}
void IOHandler::childStatus(pid_t, int) {}
#endif  // MINGW

#ifdef MINGW

#include <nrnmutdec.h>
static int bind_tid_;
void nrniv_bind_thread(void);
extern int (*iv_bind_enqueue_)(void (*)(void*), void* w);
extern void iv_bind_call(void* w, int type);
extern void nrnpy_setwindowtext(void*);

static void* w_;
static void (*nrn_gui_exec_)(void*);

namespace {
std::unique_ptr<std::condition_variable> cond_;
std::mutex mut_;
}  // namespace

bool nrn_is_gui_thread() {
    if (cond_ && GetCurrentThreadId() != bind_tid_) {
        return false;
    }
    return true;
}

int iv_bind_enqueue(void (*cb)(void*), void* w) {
    // printf("iv_bind_enqueue %p thread %d\n", w, GetCurrentThreadId());
    if (GetCurrentThreadId() == bind_tid_) {
        return 0;
    }
    nrn_gui_exec(cb, w);
    return 1;
}

void nrn_gui_exec(void (*cb)(void*), void* v) {
    assert(GetCurrentThreadId() != bind_tid_);
    // wait for the gui thread to handle the operation
    auto* const gs = neuron::python::methods.save_thread();
    {
        std::unique_lock<std::mutex> lock{mut_};
        w_ = v;
        nrn_gui_exec_ = cb;
        cond_->wait(lock, [] { return !w_; });
    }
    neuron::python::methods.restore_thread(gs);
}

void nrniv_bind_call() {
    if (!cond_) {
        return;
    }
    std::lock_guard<std::mutex> _{mut_};
    void* w = w_;
    if (w_) {
        w_ = NULL;
        (*nrn_gui_exec_)(w);
        // TODO: slight optimisation to release the mutex first
        cond_->notify_one();
    }
}


#endif  // MINGW

#endif  // HAVE_IV

void nrniv_bind_thread() {
#if HAVE_IV
    IFGUI
    bind_tid_ = int(*hoc_getarg(1));
    // printf("nrniv_bind_thread %d\n", bind_tid_);
    iv_bind_enqueue_ = iv_bind_enqueue;
    cond_ = std::make_unique<std::condition_variable>();
    w_ = NULL;
    ENDGUI
#endif
    hoc_pushx(1.);
    hoc_ret();
}

int stdin_event_ready() {
#if HAVE_IV
    IFGUI
    static DWORD main_threadid = -1;
    if (main_threadid == -1) {
        main_threadid = GetCurrentThreadId();
        return 1;
    }
    PostThreadMessage(main_threadid, WM_QUIT, 0, 0);
    ENDGUI
#endif
    return 1;
}
