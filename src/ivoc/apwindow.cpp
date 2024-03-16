#include <../../nrnconf.h>
#if HAVE_IV  // to end of file

#ifdef WIN32
#include <IV-Win/MWlib.h>
#endif

#include <InterViews/style.h>
#include <InterViews/action.h>
#ifdef WIN32
#include <IV-Win/event.h>
#include <IV-Win/window.h>
#else
#include <IV-X11/xevent.h>
#include <IV-X11/xwindow.h>
#endif
#include <InterViews/event.h>
#include <InterViews/handler.h>
#include <IV-look/kit.h>
#include <InterViews/background.h>
#include <InterViews/layout.h>
#include <InterViews/box.h>
#include <InterViews/session.h>
#include <OS/string.h>

#include "apwindow.h"
#include "ocglyph.h"
#include "ivoc.h"
#include <stdio.h>
#include <string.h>

declareActionCallback(PrintableWindow);
implementActionCallback(PrintableWindow);

extern void single_event_run();

extern void handle_old_focus();

#ifdef WIN32
#include <windows.h>
extern int iv_mere_dismiss;
#endif

// just because avoiding virtual resource
/*static*/ class DBAction: public Action {
  public:
    DBAction(WinDismiss*);
    virtual ~DBAction();
    virtual void diswin(WinDismiss*);
    virtual void execute();

  private:
    friend class DismissableWindow;
    WinDismiss* wd_;
};
DBAction::DBAction(WinDismiss* wd) {
    wd_ = wd;
    Resource::ref(wd_);
}
DBAction::~DBAction() {
    //	printf("~DBAction wd_=%p\n", wd_);
    Resource::unref(wd_);
}
void DBAction::execute() {
    if (wd_) {
        wd_->execute();
    }
}

void DBAction::diswin(WinDismiss* wd) {
    Resource::ref(wd);
    Resource::unref(wd_);
    wd_ = wd;
}

// WinDismiss

WinDismiss::WinDismiss(DismissableWindow* w) {
    win_ = w;
}

WinDismiss::~WinDismiss() {
    //	printf("~WinDismiss %p win_=%p\n", this, win_);
}

DismissableWindow* WinDismiss::win_defer_;
DismissableWindow* WinDismiss::win_defer_longer_;

void WinDismiss::execute() {
    if (Oc::helpmode()) {
        Oc::help("Dismiss GUI");
        return;
    }
    // printf("WinDismiss:: execute win_defer_=%p win_=%p\n", win_defer_,win_);
    if (win_) {
        win_->unmap();
    }
    Session::instance()->quit();
    dismiss_defer();
    win_defer_ = win_;
    win_ = NULL;
}

// the win_defer_longer_ mechanism is a hack to both avoid changing InterViews and to
// prevent the deletion of the window during receive processing (A close
// event from the window manager). The problem
// is that if the window is deleted, then during the Event::handle phase,
// the event will still access the window to figure out the target.
// Unfortunately, the dismiss_defer mechanism was broken because of the
// multiple times it was called (from within Oc::notify()). It is no longer
// known what problem that fixed so it is dangerous to remove it from there
// For this reason we avoid deleting the window while inside WinDismiss::event

bool WinDismiss::event(Event&) {
    win_defer_longer_ = win_;
    execute();
    // but maybe it is not supposed to be dismissed
    if (!win_) {
        dismiss_defer();
        win_defer_ = win_defer_longer_;
        win_defer_longer_ = NULL;
    }
    return true;
}

void ivoc_dismiss_defer() {
    WinDismiss::dismiss_defer();
}

void WinDismiss::dismiss_defer() {
    /* purify complains when window is deleted during handling of
        event that occurred in the window. So we defer the deletion
    */
    if (win_defer_ && win_defer_ != win_defer_longer_) {
        // printf("WinDismiss::dismiss_defer %p %p\n", win_defer_, win_defer_longer_);
        DismissableWindow* w = win_defer_;  // prevents BadDrawable X Errors
        win_defer_ = NULL;
        delete w;
    }
}

// DismissableWindow

bool DismissableWindow::is_transient_;
PrintableWindow* PrintableWindow::leader_;


#ifdef WIN32
DismissableWindow::DismissableWindow(Glyph* g, bool force_menubar)
    : TransientWindow(
          new Background(LayoutKit::instance()->vbox(2), WidgetKit::instance()->background()))
#else
DismissableWindow::DismissableWindow(Glyph* g, bool force_menubar)
    : TransientWindow(LayoutKit::instance()->vbox(2))
#endif
{
    glyph_ = g;
    Resource::ref(g);
#ifdef WIN32
    PolyGlyph* pg = (PolyGlyph*) ((MonoGlyph*) Window::glyph())->body();
#else
    PolyGlyph* pg = (PolyGlyph*) Window::glyph();
#endif
    wd_ = new WinDismiss(this);
    wd_->ref();
    wm_delete(wd_);
    dbutton_ = NULL;
    Style* style = Session::instance()->style();
    String str("Close");
    if ((style->find_attribute("dismiss_button", str) && str != "off") || force_menubar) {
        if (!PrintableWindow::leader()) {
            style->find_attribute("pwm_dismiss_button", str);
        }
        dbutton_ = new DBAction(wd_);
        Resource::ref(dbutton_);
        menubar_ = WidgetKit::instance()->menubar();
        Resource::ref(menubar_);
        pg->append(menubar_);
        MenuItem* mi = append_menubar(str.string());
        mi->action(dbutton_);
    } else {
        menubar_ = NULL;
    }
    if (style->find_attribute("use_transient_windows", str) && str == "yes") {
        is_transient_ = true;
    }
    pg->append(g);
}
DismissableWindow::~DismissableWindow() {
    //	printf("~DismissableWindow %p\n", this);
    Resource::unref(glyph_);
    Resource::unref(wd_);
    Resource::unref(dbutton_);
    Resource::unref(menubar_);
    single_event_run();
}

MenuItem* DismissableWindow::append_menubar(const char* name) {
    MenuItem* mi;
    if (menubar_) {
        mi = WidgetKit::instance()->menubar_item(LayoutKit::instance()->r_margin(
            WidgetKit::instance()->fancy_label(name), 0.0, fil, 0.0));
        menubar_->append_item(mi);
        return mi;
    }
    return NULL;
}

void DismissableWindow::dismiss() {
    //	unmap();
    //	delete this;
    wd_->execute();
}

const char* DismissableWindow::name() const {
    String v;
    if (!style()->find_attribute("name", v)) {
        v = Session::instance()->name();
    }
    // printf("DismissableWindow::name %s\n", v.string());
    return v.string();
}
#ifdef MINGW
static const char* s_;
static void setwindowtext(void* v) {
    HWND hw = (HWND) v;
    SetWindowText(hw, s_);
}
#endif

void DismissableWindow::name(const char* s) {
#ifdef WIN32
    HWND hw = Window::rep()->msWindow();
    if (hw) {
#ifdef MINGW
        if (!nrn_is_gui_thread()) {
            s_ = s;
            nrn_gui_exec(setwindowtext, hw);
        } else
#endif
        {
            SetWindowText(hw, s);
        }
    } else if (style()) {
#else  // not WIN32
    if (style()) {
#endif
        style()->attribute("name", s);
        set_props();  // replaces following two statements
        //		rep()->wm_name(this);
        //		rep()->do_set(this, &ManagedWindowRep::set_name);
        // printf("DismissableWindow::name set to %s\n", name());
    } else {
        style(new Style(Session::instance()->style()));
        style()->attribute("name", s);
    }
}

void DismissableWindow::replace_dismiss_action(WinDismiss* wd) {
    Resource::ref(wd);
    Resource::unref(wd_);
    wd_ = wd;
    wm_delete(wd_);
    if (dbutton_) {
        ((DBAction*) dbutton_)->diswin(wd_);
    }
}

void DismissableWindow::configure() {
    if (is_transient()) {
        TransientWindow::configure();
    } else {
        TopLevelWindow::configure();
    }
}
void DismissableWindow::set_attributes() {
    if (is_transient()) {
        TransientWindow::set_attributes();
    } else {
        TopLevelWindow::set_attributes();
    }
}

// PrintableWindow
PrintableWindow::PrintableWindow(OcGlyph* g)
    : DismissableWindow(g) {
    // printf("PrintableWindow %p\n", this);
    xplace_ = false;
    g->window(this);
    if (intercept_) {
        intercept_->box_append(g);
        mappable_ = false;
    } else {
        if (!leader_) {
            leader_ = this;
        } else {
            MenuItem* mi = append_menubar("Hide");
            if (mi) {
                mi->action(new ActionCallback(PrintableWindow)(this, &PrintableWindow::hide));
            }
        }
        PrintableWindowManager::current()->append(this);
        mappable_ = true;
    }
    type_ = "";
};
PrintableWindow::~PrintableWindow() {
    // printf("~PrintableWindow %p\n", this);
    ((OcGlyph*) glyph())->window(NULL);
    if (leader_ == this) {
        leader_ = NULL;  // mswin deletes everthing on quit
    }
    PrintableWindowManager::current()->remove(this);
}
Coord PrintableWindow::left_pw() const {
    return Window::left();
}
Coord PrintableWindow::bottom_pw() const {
    return Window::bottom();
}
Coord PrintableWindow::width_pw() const {
    return Window::width();
}
Coord PrintableWindow::height_pw() const {
    return Window::height();
}

void PrintableWindow::request_on_resize(bool b) {
    ((Window*) this)->rep()->request_on_resize_ = b;
}

Coord PrintableWindow::save_left() const {
#if 0
	Coord decor = 0.;
	if (style()) {
		style()->find_attribute("pwm_win_left_decor", decor);
	}
	return Window::left() - decor;
#else
    return Coord(xleft());
#endif
}

Coord PrintableWindow::save_bottom() const {
#if 0
	Coord decor = 0.;
	if (style()) {
		style()->find_attribute("pwm_win_top_decor", decor);
	}
	return Window::bottom() + decor;
#else
    return Coord(xtop());
#endif
}

Glyph* PrintableWindow::print_glyph() {
    return glyph();
}

void PrintableWindow::map() {
    if (mappable_) {
        DismissableWindow::map();
        single_event_run();
        notify();
    } else {
        delete this;
    }
}

void PrintableWindow::unmap() {
    handle_old_focus();
    if (is_mapped()) {
        // printf("unmap %p xleft=%d xtop=%d\n", this, xleft(), xtop());
        xplace_ = true;
        xleft_ = xleft();
        xtop_ = xtop();
        DismissableWindow::unmap();
    }
    notify();
}

OcGlyphContainer* PrintableWindow::intercept_ = NULL;

OcGlyphContainer* PrintableWindow::intercept(OcGlyphContainer* b) {
    OcGlyphContainer* i = intercept_;
    Resource::ref(b);
    Resource::unref(i);
    intercept_ = b;
    return i;
}
#ifdef WIN32
void virtual_window_top();
bool iv_user_keydown(long w) {
    if (w == 0x70) {  // F1
        virtual_window_top();
    }
    return false;
}

bool PrintableWindow::receive(const Event& e) {
    if (e.rep()->messageOf() == WM_WINDOWPOSCHANGED) {
        reconfigured();
        notify();
    }
    return DismissableWindow::receive(e);
}
#else
bool PrintableWindow::receive(const Event& e) {
    DismissableWindow::receive(e);
    if (e.type() == Event::other_event) {
        XEvent& xe = e.rep()->xevent_;
        switch (xe.type) {
        // LCOV_EXCL_START
        case ConfigureNotify:
            reconfigured();
            notify();
            break;
        // LCOV_EXCL_END
        case MapNotify:
            if (xplace_) {
                if (xtop() != xtop_ || xleft() != xleft_) {
                    // printf("MapNotify move %p (%d, %d) to (%d, %d)\n", this, xleft(), xtop(),
                    // xleft_, xtop_);
                    xmove(xleft_, xtop_);
                }
            }
            map_notify();
            notify();
            break;
        case UnmapNotify:
            // printf("UnMapNotify %p xleft=%d xtop=%d\n", this, xleft(), xtop());
            // having trouble with remapping after a "hide" that the left and top are
            // set to incorrect values. i.e.the symptom is that xleft() and xtop() are
            // wrong by the time we get this event.
            // xplace_ = true;
            // xleft_ = xleft();
            // xtop_ = xtop();
            unmap_notify();
            notify();
            break;
        case EnterNotify:
            //			printf("EnterNotify\n");
            Oc::helpmode(this);
            break;
        }
    }
    return false;
}
#endif

void PrintableWindow::type(const char* s) {
    type_ = s;
}
const char* PrintableWindow::type() const {
    return type_.c_str();
}

// StandardWindow

StandardWindow::StandardWindow(Glyph* main, Glyph* info, Menu* m, Glyph* l, Glyph* r)
    : PrintableWindow(new OcGlyph(
          new Background(LayoutKit::instance()->variable_span(LayoutKit::instance()->vbox(
                             info,
                             m,
                             LayoutKit::instance()->variable_span(LayoutKit::instance()->hbox(
                                 l,
                                 LayoutKit::instance()->variable_span(
                                     LayoutKit::instance()->vbox(WidgetKit::instance()->inset_frame(
                                         LayoutKit::instance()->variable_span(main)))),
                                 r)))),
                         WidgetKit::instance()->background()))) {
    m_ = m;
    can_ = main;
    info_ = info;
    l_ = l;
    r_ = r;
    Resource::ref(m_);
    Resource::ref(can_);
    Resource::ref(info_);
    Resource::ref(l_);
    Resource::ref(r_);
}

StandardWindow::~StandardWindow() {
    //	printf("~StandardWindow\n");
    Resource::unref(m_);
    Resource::unref(can_);
    Resource::unref(info_);
    Resource::unref(l_);
    Resource::unref(r_);
}

Glyph* StandardWindow::canvas_glyph() {
    return can_;
}
Menu* StandardWindow::menubar() {
    return m_;
}
Glyph* StandardWindow::info() {
    return info_;
}
Glyph* StandardWindow::lbox() {
    return l_;
}
Glyph* StandardWindow::rbox() {
    return r_;
}

OcGlyph::OcGlyph(Glyph* body)
    : MonoGlyph(body) {
    w_ = NULL;
    parents_ = 0;
    def_w_ = -1;
    def_h_ = -1;
    d_ = NULL;
    session_priority_ = 1;
}

OcGlyph::~OcGlyph() {
    //	printf("~OcGlyph\n");
}

void OcGlyph::def_size(Coord& w, Coord& h) const {
    if (def_w_ > 0) {
        w = def_w_;
        h = def_h_;
    }
}

void OcGlyph::save(std::ostream&) {
    printf("OcGlyph::save (not implemented for relevant class)\n");
}

bool OcGlyph::has_window() {
    return (w_ != NULL);
}

PrintableWindow* OcGlyph::window() {
    return w_;
}
void OcGlyph::window(PrintableWindow* w) {
    w_ = w;
    parents(w_ != NULL);
}

PrintableWindow* OcGlyph::make_window(Coord left, Coord top, Coord w, Coord h) {
    new PrintableWindow(this);
#if 0
if (has_window()) {
printf("%s %g %g\n", window()->name(), window()->width(), window()->height());
}
#endif
    def_w_ = w;
    def_h_ = h;
    if (left >= 0) {
        w_->xplace((int) left, (int) top);
        //		w_->place(left, bottom);
    }
    return w_;
}

void OcGlyph::parents(bool b) {
    if (b) {
        ++parents_;
    } else {
        --parents_;
    }
    if (parents_ <= 0) {
        no_parents();
        parents_ = 0;
    }
}

void OcGlyph::no_parents() {}
#endif
