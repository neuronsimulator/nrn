#include <../../nrnconf.h>
#include "gui-redirect.h"

extern char* ivoc_get_temp_file();
extern int hoc_return_type_code;

#if HAVE_IV
#if defined(WIN32)
#define MACPRINT 1
#else
#define MACPRINT 0
#endif

#if defined(WIN32)
#define SNAPSHOT 0
#else
#define SNAPSHOT 1
#endif

#define DECO 2  // 1 means default on, 2 off. for Carnvale,Hines book figures

#include <string.h>
#include "ivoc.h"
#endif  // HAVE_IV
#include <stdio.h>
#include <stdlib.h>
#include "classreg.h"
#include "oc2iv.h"
#include <cmath>

#if HAVE_IV
#include "utility.h"

void single_event_run();
extern char** hoc_strpop();

#ifdef MINGW
#include <IV-Win/mprinter.h>
void iv_display_scale(float);
void iv_display_scale(Coord, Coord);  // Make if fit into the screen
char* hoc_back2forward(char*);
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#define IOS_OUT std::ios::out

#include <IV-look/kit.h>
#include <IV-look/dialogs.h>
#include <InterViews/layout.h>
#include <InterViews/hit.h>
#include <InterViews/display.h>
#include <InterViews/session.h>
#include <InterViews/color.h>
#include <InterViews/brush.h>
#include <InterViews/font.h>
#include <InterViews/event.h>
#include <InterViews/handler.h>
#include <InterViews/printer.h>
#include <InterViews/style.h>
#include <InterViews/background.h>
#include <InterViews/label.h>
#include <InterViews/resource.h>
#include <OS/string.h>
#include "apwindow.h"
#include "scenevie.h"
#include "rect.h"
#include "ivoc.h"
#include "utility.h"
#include "ocbox.h"
#include "idraw.h"
#include "mymath.h"
#include "graph.h"
#include "oc2iv.h"
#include "rubband.h"

// PGH begin
// static const float Scl =  10.;
// static const float pr_scl = 8.;
static float Scl;
static float pr_scl;
static PixelCoord pixres = 0;
// PGH end

#if SNAPSHOT
extern bool (*ivoc_snapshot_)(const Event*);
static bool ivoc_snapshot(const Event*);
#endif

#define PWM_help_             "help"
#define PWM_do_print_         "Print PWM"
#define PWM_ScreenItem_       "ScreenItem PWM"
#define PWM_PaperItem_        "PaperItem PWM"
#define PWM_landscape_        "LandPort Other"
#define PWM_virt_screen_      "VirtualScreen Other"
#define PWM_printer_control_  "SelectPrinter Other"
#define PWM_file_control_     "PostScript PrintToFile"
#define PWM_idraw_control_    "Idraw PrintToFile"
#define PWM_save_control2_    "SaveAll Session"
#define PWM_save_control1_    "SaveSelected Session"
#define PWM_retrieve_control_ "Retrieve Session"
#define PWM_tray_             "Tray Other"
#define PWM_ascii_            "Ascii PrintToFile"
#define PWM_quit_             "Quit Other"

#define pwm_impl PrintableWindowManager::current()->pwmi_
class HocPanel {
  public:
    static void save_all(std::ostream&);
};

int inside(Coord x, Coord y, const Allocation& a) {
    if (x >= a.left() && x <= a.right() && y >= a.bottom() && y <= a.top()) {
        return true;
    } else {
        return false;
    }
}

#define DBG 0
void print_alloc(Canvas* c, char* s, const Allocation& a) {
#if DBG || 1
    printf("%s allocation %g %g %g %g\n", s, a.left(), a.bottom(), a.right(), a.top());
    if (c) {
        Extension e;
        e.set(c, a);
        printf("	canvas %g %g %g %g\n", e.left(), e.bottom(), e.right(), e.top());
    }
#endif
}

/* static */ class ScreenScene: public Scene {
  public:
    ScreenScene(Coord, Coord, Coord, Coord, Glyph* g = NULL);
    virtual void pick(Canvas*, const Allocation&, int depth, Hit&);
    virtual Coord mbs() const;
};

/* static */ class PaperScene: public Scene {
  public:
    PaperScene(Coord, Coord, Coord, Coord, Glyph* g = NULL);
    virtual Coord mbs() const;
};

/* static */ class ScreenSceneHandler: public Handler {
  public:
    ScreenSceneHandler(Coord, Coord);
    virtual bool event(Event&);

  private:
    Coord x_, y_;
};

class PaperItem;

/*static*/ class ScreenItem: public Glyph {
  public:
    friend class PaperItem;
    ScreenItem(PrintableWindow*);
    ~ScreenItem();
    virtual void request(Requisition&) const;
    virtual void allocate(Canvas*, const Allocation&, Extension&);
    virtual void draw(Canvas*, const Allocation&) const;
    virtual void pick(Canvas*, const Allocation&, int depth, Hit&);
    PrintableWindow* window() {
        return w_;
    }
    void relabel(GlyphIndex);
    //	void reconfigured(Scene*);
    PaperItem* paper_item() const {
        return pi_;
    }
    GlyphIndex index() const {
        return i_;
    }
    Object* group_obj_;
    bool iconify_via_hide_;

  private:
    Glyph* label_;
    GlyphIndex i_;
    PrintableWindow* w_;
    PaperItem* pi_;
};

/*static*/ class PaperItem: public Glyph {
  public:
    PaperItem(ScreenItem*);
    ~PaperItem();
    virtual void request(Requisition&) const;
    virtual void allocate(Canvas*, const Allocation&, Extension&);
    virtual void draw(Canvas*, const Allocation&) const;
    //	virtual void print(Printer*, const Allocation&) const;
    virtual void pick(Canvas*, const Allocation&, int depth, Hit&);
    void scale(float s) {
        scale_ = s;
    }
    float scale() {
        return scale_;
    }
    ScreenItem* screen_item() const {
        return si_;
    }
    Coord width();        /* width of icon in pixels */
    static Coord fsize_;  // font height
  private:
#if 1
    friend class ScreenItem;
#else
    // I prefer this but the SGI compiler doesn't like it
    friend ScreenItem::~ScreenItem();
#endif
    ScreenItem* si_;
    float scale_;
};

/*static*/ class PWMImpl {
  public:
    void append_paper(ScreenItem*);
    void remove_paper(PaperItem*);
    void unshow_paper(PaperItem*);
    GlyphIndex paper_index(PaperItem*);
    PaperScene* paper() {
        return paper_;
    }
    ScreenScene* screen() {
        return screen_;
    }

    void help();
#if SNAPSHOT
    void snapshot(const Event*);
    Window* snap_owned(Printer*, Window*);
    void snap(Printer*, Window*);
    void snap_cursor(Printer*, const Event*);
#endif
    void do_print0();
    void do_print(bool printer, const char* name);
    void do_print_session(bool also_controller = true);
    void do_print_session(bool printer, const char* name);
    void ps_file_print(bool, const char*, bool, bool);
    void common_print(Printer*, bool, bool);
#if DECO
    void print_deco(Printer*, Allocation& a, const char*);
#endif
#if MACPRINT
    void mac_do_print();
    MacPrinter* mprinter();
    void paperscale();
#endif
    void select_tool();
    EventButton tool(EventButton);
    void move_tool();
    void resize_tool();
    void landscape();
    void landscape(bool);
    bool is_landscape() {
        return landscape_;
    }
    void deco(int);
    void virt_screen();
    void tray();
    void printer_control();
    void file_control();
#if SNAPSHOT
    void snapshot_control();
#endif
    bool file_control1();
    void idraw_control();
    void idraw_write(const char* fname, bool ses_style = false);
    void ascii_control();
    void ascii_write(const char* fname, bool ses_style = false);
    void quit_control();
    void save_selected_control();
    void save_all_control();
    void save_control(int);
    void save_session(int, const char*, const char* head = NULL);
    int save_group(Object*, const char*);
    void retrieve_control();
    float round(float);
    float round_factor() {
        return round_factor_;
    }
    void map_all();
    void unmap_all();
    StandardWindow* window();
    void window(StandardWindow* w) {
        w_ = w;
    }
    void all_window_bounding_box(Extension&, bool with_screen = true, bool also_controller = true);
    void view_screen(Coord, Coord);
    FileChooser* fc_save_;
    const Color* window_outline_;
    CopyString cur_ses_name_;

  private:
    friend class PrintableWindowManager;
    PWMImpl(ScreenScene*, PaperScene*, Rect*);
    ~PWMImpl();
    GlyphIndex index(void*);
    void relabel();
    GlyphIndex upper_left();
    void redraw(Window*);
    bool none_selected(const char*, const char*) const;
    void ses_group(ScreenItem* si, std::ostream& o);
    int ses_group_first_;
    void save_begin(std::ostream&);
    void save_list(int, ScreenItem**, std::ostream&);

  private:
    StandardWindow* w_;
    ScreenScene* screen_;
    PaperScene* paper_;
    View* pview_;
    bool landscape_;
    Rect* prect_;
    bool use_printer;
    bool printer_control_accept_;
    String printer_;
    FieldDialog* b_printer_;
    FileChooser* fc_print_;
    FileChooser* fc_idraw_;
    FileChooser* fc_ascii_;
    FileChooser* fc_retrieve_;
    Coord canvasheight_;
    float round_factor_;
    TelltaleState* p_title_;
    Glyph* left_;  // ugh
    EventButton tool_;
    const Event* snap_event_;
    bool print_leader_flag_;
#if DECO
    TelltaleState* p_deco_;
#endif
    Rect* screen_rect_;
#if MACPRINT
    MacPrinter* mprinter_;
#endif
};

/* static */ class VirtualWindow: public DismissableWindow {
  public:
    static void makeVirtualWindow();
    static void view();
    virtual ~VirtualWindow();

  private:
    VirtualWindow(View*, Glyph*);

  private:
    static VirtualWindow* virt_win_;
    View* view_;
};

VirtualWindow* VirtualWindow::virt_win_;

#ifdef WIN32

/* static */ class VirtualWindowScale: public Action {
  public:
    VirtualWindowScale(float);
    virtual void execute();

  private:
    float scale_;
};
#endif

/*static*/ class PaperItem_handler: public Handler {
  public:
    enum { resize, move };
    PaperItem_handler(int type, Coord x, Coord y, PaperItem*, const Transformer&);
    virtual ~PaperItem_handler();
    virtual bool event(Event&);

  private:
    void resize_action(Coord, Coord);
    void move_action(Coord, Coord);

  private:
    void (PaperItem_handler::*action_)(Coord, Coord);
    Transformer t_;
    PaperItem* pi_;
    GlyphIndex index_;
};

/*static*/ class ScreenItemHandler: public Handler {
  public:
    ScreenItemHandler(Coord x, Coord y, ScreenItem*, const Transformer&);
    virtual ~ScreenItemHandler();
    virtual bool event(Event&);

  private:
    void move_action(bool, Coord, Coord);

  private:
    Transformer t_;
    ScreenItem* si_;
};

PrintableWindowManager* PrintableWindowManager::current_;

/*static*/ class PWMDismiss: public WinDismiss {
  public:
    PWMDismiss(DismissableWindow*);
    virtual ~PWMDismiss();
    virtual void execute();
};
PWMDismiss::PWMDismiss(DismissableWindow* w)
    : WinDismiss(w) {}
PWMDismiss::~PWMDismiss() {}
void PWMDismiss::execute() {
    if (!DismissableWindow::is_transient()) {
        pwm_impl->unmap_all();
    } else {
        PrintableWindow::leader()->iconify();
    }
}

#else  //! HAVE_IV
#ifdef MINGW
char* hoc_back2forward(char*);
#endif
#endif  // HAVE_IV

static void* pwman_cons(Object*) {
    TRY_GUI_REDIRECT_OBJ("PWManager", NULL);
    void* v = NULL;
#if HAVE_IV
    IFGUI
    v = (void*) PrintableWindowManager::current();
    ENDGUI
#endif
    return v;
}
static void pwman_destruct(void* v) {
    TRY_GUI_REDIRECT_NO_RETURN("~PWManager", v);
}

static double pwman_count(void* v) {
    int cnt = 0;
    hoc_return_type_code = 1;  // integer
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("PWManager.count", v);
#if HAVE_IV
    IFGUI
    PWMImpl* p = PrintableWindowManager::current()->pwmi_;
    cnt = p->screen()->count();
    ENDGUI
#endif
    return double(cnt);
}
static double pwman_is_mapped(void* v) {
    hoc_return_type_code = 2;  // boolean
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("PWManager.is_mapped", v);
#if HAVE_IV
    IFGUI
    PWMImpl* p = PrintableWindowManager::current()->pwmi_;
    int i = (int) chkarg(1, 0, p->screen()->count() - 1);
    ScreenItem* si = (ScreenItem*) p->screen()->component(i);
    if (si->window()) {
        return double(si->window()->is_mapped());
    }
    ENDGUI
#endif
    return 0.;
}
static double pwman_map(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("PWManager.map", v);
#if HAVE_IV
    IFGUI
    PWMImpl* p = PrintableWindowManager::current()->pwmi_;
    int i = (int) chkarg(1, 0, p->screen()->count() - 1);
    ScreenItem* si = (ScreenItem*) p->screen()->component(i);
    if (si->window()) {
        si->window()->map();
    }
    ENDGUI
#endif
    return 0.;
}
static double pwman_hide(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("PWManager.hide", v);
#if HAVE_IV
    IFGUI
    PWMImpl* p = PrintableWindowManager::current()->pwmi_;
    int i = (int) chkarg(1, 0, p->screen()->count() - 1);
    ScreenItem* si = (ScreenItem*) p->screen()->component(i);
    if (si->window()) {
        si->window()->hide();
    }
    ENDGUI
#endif
    return 0.;
}
static const char** pwman_name(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_STR("PWManager.name", v);
#if HAVE_IV
    IFGUI
    PWMImpl* p = PrintableWindowManager::current()->pwmi_;
    int i = (int) chkarg(1, 0, p->screen()->count() - 1);
    ScreenItem* si = (ScreenItem*) p->screen()->component(i);
    char** ps = hoc_temp_charptr();
    if (si->window()) {
        *ps = (char*) si->window()->name();
    }
    return (const char**) ps;
    ENDGUI
#endif
    return 0;
}
static double pwman_close(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("PWManager.close", v);
#if HAVE_IV
    IFGUI
    PWMImpl* p = PrintableWindowManager::current()->pwmi_;
    int i = (int) chkarg(1, 0, p->screen()->count() - 1);
    ScreenItem* si = (ScreenItem*) p->screen()->component(i);
    if (p->window() == si->window()) {
        p->window(NULL);
    }
    si->window()->dismiss();
    ENDGUI
#endif
    return 0.;
}
#ifdef MINGW
static void pwman_iconify1(void* v) {
#if HAVE_IV
    IFGUI((PrintableWindow*) v)->dismiss();
    ENDGUI
#endif
}
#endif

static double pwman_iconify(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("PWManager.iconify", v);
#if HAVE_IV
    IFGUI
    PrintableWindow* pw = PrintableWindow::leader();
#ifdef MINGW
    if (!nrn_is_gui_thread()) {
        nrn_gui_exec(pwman_iconify1, pw);
        return 0.;
    }
#endif
    pw->dismiss();
    ENDGUI
#endif
    return 0.;
}
static double pwman_deiconify(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("PWManager.deiconify", v);
#if HAVE_IV
    IFGUI
    PrintableWindow* pw = PrintableWindow::leader();
    pw->map();
    ENDGUI
#endif
    return 0.;
}
static double pwman_leader(void* v) {
    hoc_return_type_code = 1;  // integer
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("PWManager.leader", v);
#if HAVE_IV
    IFGUI
    PWMImpl* p = PrintableWindowManager::current()->pwmi_;
    PrintableWindow* pw = PrintableWindow::leader();
    int i, cnt = p->screen()->count();
    for (i = 0; i < cnt; ++i) {
        ScreenItem* si = (ScreenItem*) p->screen()->component(i);
        if (si->window() == pw) {
            return double(i);
        }
    }
    ENDGUI
#endif
    return -1.;
}
static double pwman_manager(void* v) {
    hoc_return_type_code = 1;  // integer
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("PWManager.manager", v);
#if HAVE_IV
    IFGUI
    PWMImpl* p = PrintableWindowManager::current()->pwmi_;
    PrintableWindow* pw = p->window();
    int i, cnt = p->screen()->count();
    for (i = 0; i < cnt; ++i) {
        ScreenItem* si = (ScreenItem*) p->screen()->component(i);
        if (si->window() == pw) {
            return double(i);
        }
    }
    ENDGUI
#endif
    return -1.;
}

static double pwman_save(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("PWManager.save", v);
    int n = 0;
#if HAVE_IV
    IFGUI
    // if arg2 is an object then save all windows with that group_obj
    // if arg2 is 1 then save all windows.
    // if arg2 is 0 then save selected (on paper) windows.
    PWMImpl* p = PrintableWindowManager::current()->pwmi_;
    if (ifarg(2)) {
        if (hoc_is_object_arg(2)) {
            n = p->save_group(*hoc_objgetarg(2), gargstr(1));
        } else {
            n = (int) chkarg(2, 0, 1);
            p->save_session((n ? 2 : 0), gargstr(1), (ifarg(3) ? gargstr(3) : NULL));
        }
    }
    ENDGUI
#endif
    return (double) n;
}

static Object** pwman_group(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_OBJ("PWManager.group", v);
#if HAVE_IV
    IFGUI
    PWMImpl* p = PrintableWindowManager::current()->pwmi_;
    int i;
    i = int(chkarg(1, 0, p->screen()->count() - 1));
    ScreenItem* si = (ScreenItem*) p->screen()->component(i);
    if (ifarg(2)) {
        hoc_obj_unref(si->group_obj_);
        si->group_obj_ = *hoc_objgetarg(2);
        hoc_obj_ref(si->group_obj_);
    }
    return hoc_temp_objptr(si->group_obj_);
    ENDGUI
#endif
    return hoc_temp_objptr(0);
}

static double pwman_snap(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("PWManager.snap", v);
#if HAVE_IV
    IFGUI
#if SNAPSHOT
    PWMImpl* p = PrintableWindowManager::current()->pwmi_;
    if (!ifarg(1)) {
        p->snapshot_control();
    }
#endif
    return 1.;
    ENDGUI
#endif
    return 0;
}

#ifdef MINGW
static double scale_;
static void pwman_scale1(void*) {
#if HAVE_IV
    IFGUI
    iv_display_scale(scale_);
    ENDGUI
#endif
}
#endif

static double pwman_scale(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("PWManager.scale", v);
    double scale = chkarg(1, .01, 100);
#if HAVE_IV
    IFGUI
#if defined(WIN32)
#ifdef MINGW
    if (!nrn_is_gui_thread()) {
        scale_ = scale;
        nrn_gui_exec(pwman_scale1, (void*) ((intptr_t) 1));
        return scale;
    }
#endif
    iv_display_scale(scale);
#endif
    ENDGUI
#endif
    return scale;
}

static double pwman_window_place(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("PWManager.window_place", v);
#if HAVE_IV
    IFGUI
    int i;
    PWMImpl* p = PrintableWindowManager::current()->pwmi_;
    i = int(chkarg(1, 0, p->screen()->count() - 1));
    ScreenItem* si = (ScreenItem*) p->screen()->component(i);
    if (si->window()) {
        si->window()->xmove(int(*getarg(2)), int(*getarg(3)));
    }
    ENDGUI
#endif
    return 1.;
}

static double pwman_paper_place(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("PWManager.paper_place", v);
#if HAVE_IV
    IFGUI
    // index, show=0 or 1
    // index, x, y, scale where x and y in inches from left bottom
    int i;
    PWMImpl* p = PrintableWindowManager::current()->pwmi_;
    i = int(chkarg(1, 0, p->screen()->count() - 1));
    ScreenItem* si = (ScreenItem*) p->screen()->component(i);
    p->append_paper(si);
    PaperItem* pi = si->paper_item();
    if (!ifarg(3)) {
        if ((int(chkarg(2, 0, 1))) == 0) {
            p->unshow_paper(pi);
        }
    } else {
        pi->scale(chkarg(4, 1e-4, 1e4));
        p->paper()->move(p->paper_index(pi), *getarg(2) / pr_scl, *getarg(3) / pr_scl);
    }
    ENDGUI
#endif
    return 1.;
}

static double pwman_printfile(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("PWManager.printfile", v);
#if HAVE_IV
    IFGUI
    // first arg is filename
    // second arg is 0,1,2 refers to postscript, idraw, ascii mode
    // third arg is 0,1 refers to selected, all
    PWMImpl* p = PrintableWindowManager::current()->pwmi_;
    bool ses_style = false;
    if (ifarg(3)) {
        ses_style = int(chkarg(3, 0, 1)) ? true : false;
    }
    char* fname = gargstr(1);
    switch ((int) chkarg(2, 0, 2)) {
    case 0:
        p->ps_file_print(false, fname, p->is_landscape(), ses_style);
        break;
    case 1:
        p->idraw_write(fname, ses_style);
        break;
    case 2:
        p->ascii_write(fname, ses_style);
        break;
    }
    ENDGUI
#endif
    return 1.;
}

static double pwman_landscape(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("PWManager.landscape", v);
#if HAVE_IV
    IFGUI
    PWMImpl* p = PrintableWindowManager::current()->pwmi_;
    p->landscape(int(chkarg(1, 0, 1)) ? true : false);
    ENDGUI
#endif
    return 1.;
}

static double pwman_deco(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("PWManager.deco", v);
#if HAVE_IV
    IFGUI
    PWMImpl* p = PrintableWindowManager::current()->pwmi_;
    p->deco(int(chkarg(1, 0, 2)));
    ENDGUI
#endif
    return 1.;
}

static Member_func members[] = {{"count", pwman_count},
                                {"is_mapped", pwman_is_mapped},
                                {"map", pwman_map},
                                {"hide", pwman_hide},
                                {"close", pwman_close},
                                {"iconify", pwman_iconify},
                                {"deiconify", pwman_deiconify},
                                {"leader", pwman_leader},
                                {"manager", pwman_manager},
                                {"save", pwman_save},
                                {"snap", pwman_snap},
                                {"scale", pwman_scale},
                                {"window_place", pwman_window_place},
                                {"paper_place", pwman_paper_place},
                                {"printfile", pwman_printfile},
                                {"landscape", pwman_landscape},
                                {"deco", pwman_deco},
                                {0, 0}};

static Member_ret_obj_func retobj_members[] = {{"group", pwman_group}, {0, 0}};

static Member_ret_str_func s_memb[] = {{"name", pwman_name}, {0, 0}};

void PWManager_reg() {
    class2oc("PWManager", pwman_cons, pwman_destruct, members, NULL, retobj_members, s_memb);
}

#if HAVE_IV  // almost to end of file

// PaperItem_handler

PaperItem_handler::PaperItem_handler(int type,
                                     Coord x,
                                     Coord y,
                                     PaperItem* pi,
                                     const Transformer& t) {
    // printf("PaperItem_handler\n");
    t_ = t;
    pi_ = pi;
    Resource::ref(pi);
    index_ = pwm_impl->paper_index(pi);
    Coord left, bottom;
    pwm_impl->paper()->location(index_, left, bottom);
    t_.invert();
    switch (type) {
    case resize:
        action_ = &PaperItem_handler::resize_action;
        break;
    case move:
        t_.translate(left - x, bottom - y);
        action_ = &PaperItem_handler::move_action;
        break;
    }
}

PaperItem_handler::~PaperItem_handler() {
    // printf("~PaperItem_handler\n");
    Resource::unref(pi_);
}

bool PaperItem_handler::event(Event& e) {
    // printf("PaperItem_handler::event (%g, %g)\n", e.pointer_x(), e.pointer_y());
    switch (e.type()) {
    case Event::down:
        e.grab(this);
#ifdef WIN32
        e.window()->grab_pointer();
#endif
        (this->*action_)(e.pointer_x(), e.pointer_y());
        break;
    case Event::up:
        e.ungrab(this);
#ifdef WIN32
        e.window()->ungrab_pointer();
#endif
        break;
    case Event::motion:
        (this->*action_)(e.pointer_x(), e.pointer_y());
        break;
    }
    return true;
}

void PaperItem_handler::move_action(Coord x, Coord y) {
    // printf("move_action\n");
    Coord xs, ys;
    t_.transform(x, y, xs, ys);
    xs = pwm_impl->round(xs);
    ys = pwm_impl->round(ys);
    pwm_impl->paper()->move(index_, xs, ys);
}

void PaperItem_handler::resize_action(Coord x, Coord y) {
    Allotment ax;
    pwm_impl->paper()->allotment(index_, Dimension_X, ax);
    Allotment ay;
    pwm_impl->paper()->allotment(index_, Dimension_Y, ay);
    Coord xs, ys;
    t_.transform(x, y, xs, ys);
    float scl = std::max((xs - ax.begin()) / ax.span(), (ys - ay.begin()) / ay.span());
    // printf("scl = %g\n", scl);
    scl = pi_->scale() * scl;
    scl = (scl > .1) ? scl : .1;
    Coord w1;
    w1 = pwm_impl->round(scl * pi_->width());
    w1 = std::max(w1, pwm_impl->round_factor());
    scl = w1 / pi_->width();
    pi_->scale(scl);
    pwm_impl->paper()->modified(index_);
}

VirtualWindow::VirtualWindow(View* v, Glyph* g)
    : DismissableWindow(g, true) {
    view_ = v;
    view_->ref();
#ifdef WIN32
    if (!style()) {
        style(new Style(Session::instance()->style()));
        style()->attribute("nrn_virtual_screen", "0");
    }

    MenuItem* mi = append_menubar("Scale");
    WidgetKit& wk = *WidgetKit::instance();
    Menu* m = wk.pulldown();
    mi->menu(m);

    mi = K::menu_item("normal");
    mi->action(new VirtualWindowScale(1.0));
    m->append_item(mi);

    mi = K::menu_item("see all");
    mi->action(new VirtualWindowScale(fil));
    m->append_item(mi);

    mi = K::menu_item("1.2");
    mi->action(new VirtualWindowScale(1.2));
    m->append_item(mi);

    mi = K::menu_item("1.5");
    mi->action(new VirtualWindowScale(1.5));
    m->append_item(mi);

    mi = K::menu_item("2.0");
    mi->action(new VirtualWindowScale(2.0));
    m->append_item(mi);
#endif
}

VirtualWindow::~VirtualWindow() {
    view_->unref();
    virt_win_ = NULL;
}
#if defined(WIN32)
extern void ivoc_bring_to_top(Window*);
#endif

void VirtualWindow::makeVirtualWindow() {
    if (!virt_win_) {
        View* v = new View(pwm_impl->screen());
        virt_win_ = new VirtualWindow(v,
                                      LayoutKit::instance()->variable_span(
                                          new Background(v, WidgetKit::instance()->background())));
        virt_win_->map();
    }
#ifdef WIN32
    ivoc_bring_to_top(virt_win_);
#endif
}

void virtual_window_top() {
    VirtualWindow::makeVirtualWindow();
}

void VirtualWindow::view() {
    if (virt_win_) {
        View* v = virt_win_->view_;
        Scene* s = v->scene();
        v->size(s->x1(), s->y1(), s->x2(), s->y2());
        virt_win_->canvas()->damage_all();
    }
}

#ifdef WIN32
VirtualWindowScale::VirtualWindowScale(float scale) {
    scale_ = scale;
}

void VirtualWindowScale::execute() {
    float scale = scale_;
    if (scale_ >= fil / 10.) {
        Extension e;
        pwm_impl->all_window_bounding_box(e);
        iv_display_scale(e.right() - e.left(), e.top() - e.bottom());
    } else {
        iv_display_scale(scale);
    }
}
#endif

// ScreenItemHandler

ScreenItemHandler::ScreenItemHandler(Coord x, Coord y, ScreenItem* si, const Transformer& t) {
    // printf("ScreenItemHandler\n");
    t_ = t;
    si_ = si;
    Resource::ref(si);
    Coord left, bottom;
    pwm_impl->screen()->location(si_->index(), left, bottom);
    t_.invert();
    t_.translate(left - x, bottom - y);
}

ScreenItemHandler::~ScreenItemHandler() {
    // printf("~ScreenItemHandler\n");
    Resource::unref(si_);
}

bool ScreenItemHandler::event(Event& e) {
    // printf("ScreenItemHandler::event (%g, %g)\n", e.pointer_x(), e.pointer_y());
    switch (e.type()) {
    case Event::down:
        e.grab(this);
#ifdef WIN32
        e.window()->grab_pointer();
#endif
        move_action(false, e.pointer_x(), e.pointer_y());
        break;
    case Event::up:
        e.ungrab(this);
#ifdef WIN32
        e.window()->ungrab_pointer();
#endif
        move_action(true, e.pointer_x(), e.pointer_y());
        break;
    case Event::motion:
        move_action(false, e.pointer_x(), e.pointer_y());
        break;
    }
    return true;
}

void ScreenItemHandler::move_action(bool doit, Coord x, Coord y) {
    // printf("move_action\n");
    Coord xs, ys;
    t_.transform(x, y, xs, ys);
    if (doit) {
        if (si_->window()) {
            si_->window()->move(xs * Scl, ys * Scl);
        }
    } else {
        pwm_impl->screen()->move(si_->index(), xs, ys);
    }
}

ScreenScene::ScreenScene(Coord x1, Coord y1, Coord x2, Coord y2, Glyph* g)
    : Scene(x1, y1, x2, y2, g) {}

void ScreenScene::pick(Canvas* c, const Allocation& a, int depth, Hit& h) {
    if (pwm_impl->tool(h.event()->pointer_button()) == Event::middle) {
        if (h.event()->type() == Event::down) {
            h.target(depth, this, 0, new ScreenSceneHandler(h.left() * Scl, h.bottom() * Scl));
        }
    } else {
        Scene::pick(c, a, depth, h);
    }
}

Coord ScreenScene::mbs() const {
    return 0.;
}

PaperScene::PaperScene(Coord x1, Coord y1, Coord x2, Coord y2, Glyph* g)
    : Scene(x1, y1, x2, y2, g) {}

Coord PaperScene::mbs() const {
    return 0.;
}

ScreenSceneHandler::ScreenSceneHandler(Coord x, Coord y)
    : Handler() {
    x_ = x;
    y_ = y;
}
bool ScreenSceneHandler::event(Event&) {
    pwm_impl->view_screen(x_, y_);
    return true;
}

// PrintableWindowManager

declareActionCallback(PWMImpl)
implementActionCallback(PWMImpl)

PrintableWindowManager* PrintableWindowManager::current() {
    if (!current_) {
        current_ = new PrintableWindowManager();
    }
    return current_;
}

PrintableWindowManager::PrintableWindowManager() {
    LayoutKit& layout = *LayoutKit::instance();
    WidgetKit& kit = *WidgetKit::instance();
    PaperItem::fsize_ = kit.font()->size();
    current_ = this;
    Display* d = Session::instance()->default_display();

    // PGH begin
    Coord canvasheight;
    Style* q = Session::instance()->style();
    if (!q->find_attribute("pwm_canvas_height", canvasheight)) {
        canvasheight = 100.;
    }
    const Color* outline_color;
    String c;
    Display* dis = Session::instance()->default_display();
    if (!q->find_attribute("pwm_screen_outline_color", c) ||
        (outline_color = Color::lookup(dis, c)) == NULL) {
        outline_color = Color::lookup(dis, "#ff0000");
    }
    Scl = d->height() / canvasheight;
    Rect* sr = new Rect(0, 0, d->width() / Scl, d->height() / Scl, outline_color);
    sr->ref();
    ScreenScene* screen = new ScreenScene(-5, -2, d->width() / Scl + 5, d->height() / Scl + 2, sr);
    Coord pageheight;
    Coord pagewidth;
    if (!q->find_attribute("pwm_paper_height", pageheight)) {
        pageheight = 11.;
    }
    if (!q->find_attribute("pwm_paper_width", pagewidth)) {
        pagewidth = 8.5;
    }
    Coord wp1;
    if (pageheight > pagewidth)
        pr_scl = pageheight / canvasheight;
    else
        pr_scl = pagewidth / canvasheight;

    // width = max(d->width/Scl,pagewidth/prl_scl,pageheight/prl_scl)
    if (d->width() > d->height())
        wp1 = d->width() / Scl;
    else
        wp1 = canvasheight;

    Coord wp = pagewidth / pr_scl;
    Coord hp = pageheight / pr_scl;
    Coord max = std::max(wp, hp);
    Rect* r = new Rect(0, 0, wp, hp, outline_color);
    //        wp1 = wp1*1.2;
    //	Scene* paper = new Scene(-5, -1, hp*1.2, hp+1, r);
    PaperScene* paper = new PaperScene(-5, -2, std::max(max, d->width() / Scl), max + 2, r);

    // PGH end
    pwmi_ = new PWMImpl(screen, paper, r);
    if (!q->find_attribute("pwm_window_outline_color", c) ||
        (outline_color = Color::lookup(dis, c)) == NULL) {
        outline_color = Color::lookup(dis, "#0000ff");
    }
    outline_color->ref();
    pwmi_->window_outline_ = outline_color;
    pwmi_->screen_rect_ = sr;
    if (!q->find_attribute("pwm_paper_resolution", pwmi_->round_factor_)) {
        pwmi_->round_factor_ = .25;
    }
    pwmi_->canvasheight_ = canvasheight;
    pwmi_->round_factor_ /= pr_scl;
    long ltmp;
    if (q->find_attribute("pwm_pixel_resolution", ltmp)) {
        pixres = (PixelCoord) ltmp;
    }

    PolyGlyph* hb = layout.hbox(6);
    pwmi_->left_ = hb;
    pwmi_->left_->ref();

    Menu *mbar, *mprint, *mses, *mother;
#if 0
	if (q->value_is_on("pwm_help")) {
		vb->append(kit.push_button("Help",
			new ActionCallback(PWMImpl)(pwmi_, &PWMImpl::help)
		));
	}
#endif
    hb->append(mbar = kit.menubar());

    MenuItem* mi;

    mi = kit.menubar_item("Print");
    mbar->append_item(mi);
    mprint = kit.pulldown();
    mi->menu(mprint);

    mi = kit.menubar_item("Session");
    mbar->append_item(mi);
    mses = kit.pulldown();
    mi->menu(mses);

#if 0
	mi = kit.menubar_item("Other");
	mbar->append_item(mi);
	mother = kit.pulldown();
	mi->menu(mother);
#endif

    TelltaleGroup* ttg = new TelltaleGroup();
    mi = kit.radio_menu_item(ttg, "select");
    mbar->append_item(mi);
    mi->state()->set(TelltaleState::is_chosen, true);
    mi->action(new ActionCallback(PWMImpl)(pwmi_, &PWMImpl::select_tool));

    mi = kit.radio_menu_item(ttg, "move");
    mbar->append_item(mi);
    mi->action(new ActionCallback(PWMImpl)(pwmi_, &PWMImpl::move_tool));

    mi = kit.radio_menu_item(ttg, "resize");
    mbar->append_item(mi);
    mi->action(new ActionCallback(PWMImpl)(pwmi_, &PWMImpl::resize_tool));

    mi = K::menu_item("To Printer");
    mprint->append_item(mi);
    mi->action(new ActionCallback(PWMImpl)(pwmi_, &PWMImpl::do_print0));

    mi = K::menu_item("PostScript");
    mprint->append_item(mi);
    mi->action(new ActionCallback(PWMImpl)(pwmi_, &PWMImpl::file_control));

#if SNAPSHOT
    mi = K::menu_item("PS snapshot");
    mprint->append_item(mi);
    mi->action(new ActionCallback(PWMImpl)(pwmi_, &PWMImpl::snapshot_control));
#endif

    mi = K::menu_item("Idraw");
    mprint->append_item(mi);
    mi->action(new ActionCallback(PWMImpl)(pwmi_, &PWMImpl::idraw_control));

    mi = K::menu_item("Ascii");
    mprint->append_item(mi);
    mi->action(new ActionCallback(PWMImpl)(pwmi_, &PWMImpl::ascii_control));

    mi = K::menu_item("Select Printer");
    mprint->append_item(mi);
    mi->action(new ActionCallback(PWMImpl)(pwmi_, &PWMImpl::printer_control));

    mi = K::check_menu_item("Window Titles Printed");
    mprint->append_item(mi);
    pwmi_->p_title_ = mi->state();

#if DECO
    mi = K::check_menu_item("Window Decorations Printed");
    mprint->append_item(mi);
    pwmi_->p_deco_ = mi->state();
    // automatically on. comment out otherwise
#if DECO == 1
    pwmi_->p_deco_->set(TelltaleState::is_chosen, true);
#else
    pwmi_->p_deco_->set(TelltaleState::is_chosen, false);
#endif
#endif

    mi = K::menu_item("Retrieve");
    mses->append_item(mi);
    mi->action(new ActionCallback(PWMImpl)(pwmi_, &PWMImpl::retrieve_control));

    mi = K::menu_item("Save selected");
    mses->append_item(mi);
    mi->action(new ActionCallback(PWMImpl)(pwmi_, &PWMImpl::save_selected_control));

    mi = K::menu_item("Save all");
    mses->append_item(mi);
    mi->action(new ActionCallback(PWMImpl)(pwmi_, &PWMImpl::save_all_control));

    mi = K::menu_item("VirtualScreen");
    mses->append_item(mi);
    mi->action(new ActionCallback(PWMImpl)(pwmi_, &PWMImpl::virt_screen));

    mi = K::menu_item("Land/Port");
    mprint->append_item(mi);
    mi->action(new ActionCallback(PWMImpl)(pwmi_, &PWMImpl::landscape));

    mi = K::menu_item("Tray");
    mses->append_item(mi);
    mi->action(new ActionCallback(PWMImpl)(pwmi_, &PWMImpl::tray));

#if 0
	mi = K::menu_item("Quit");
	mother->append_item(mi);
	mi->action(new ActionCallback(PWMImpl)(pwmi_, &PWMImpl::quit_control));
#endif
    if (PrintableWindow::leader() == NULL) {
        pwmi_->window();
        OcGlyphContainer* ocg = PrintableWindow::intercept(0);
        if (PrintableWindow::leader() != pwmi_->w_) {
            pwmi_->w_->replace_dismiss_action(NULL);
        }
#if OCSMALL
        pwmi_->w_->xplace(-800, 0);
#else
        pwmi_->w_->xplace(0, 0);
#endif
        //		pwmi_->w_->map();
        PrintableWindow::intercept(ocg);
    }
    PrintableWindow::leader()->replace_dismiss_action(new PWMDismiss(PrintableWindow::leader()));
}

void PWMImpl::select_tool() {
    tool_ = Event::right;
}
void PWMImpl::move_tool() {
    tool_ = Event::left;
}
void PWMImpl::resize_tool() {
    tool_ = Event::middle;
}

EventButton PWMImpl::tool(EventButton b) {
    if (b == Event::left) {
        return tool_;
    }
    return b;
}

StandardWindow* PWMImpl::window() {
    if (w_ == NULL) {
        LayoutKit& layout = *LayoutKit::instance();
        OcGlyphContainer* ocg = PrintableWindow::intercept(0);
        w_ = new StandardWindow(
#if 1
            layout.hbox(layout.variable_span(new View(screen_)),
                        layout.variable_span(pview_ = new View(paper_))),
#else
            new View(screen_),
#endif
            left_,  // really info
            NULL,
            NULL,
            NULL);
        PrintableWindow::intercept(ocg);
        Style* s = new Style(Session::instance()->style());
        s->attribute("name", "Print & File Window Manager");
        w_->style(s);
    }
    return w_;
}

PrintableWindowManager::~PrintableWindowManager() {
    //	printf("~PrintableWindowManager\n");
    delete pwmi_;
    if (current_ == this) {
        current_ = NULL;
    }
}

void hoc_pwman_place() {
    TRY_GUI_REDIRECT_DOUBLE("pwman_place", NULL);
#if HAVE_IV
    IFGUI
    int x, y;
    x = int(*getarg(1));
    y = int(*getarg(2));
    bool m = (ifarg(3) && int(*getarg(3)) == 0) ? false : true;
    PrintableWindowManager::current()->xplace(x, y, m);
    ENDGUI
#endif
    hoc_ret();
    hoc_pushx(0.);
}

void hoc_save_session() {
    TRY_GUI_REDIRECT_DOUBLE("save_session", NULL);
#if HAVE_IV
    IFGUI
    if (pwm_impl) {
        pwm_impl->save_session(2, gargstr(1), (ifarg(2) ? gargstr(2) : NULL));
    }
    ENDGUI
#endif
    hoc_ret();
    hoc_pushx(0.);
}
const char* pwm_session_filename() {
    if (pwm_impl) {
        return pwm_impl->cur_ses_name_.string();
    }
    return 0;
}

void hoc_print_session() {
    TRY_GUI_REDIRECT_DOUBLE("print_session", NULL);
#if HAVE_IV
    IFGUI
    if (pwm_impl) {
        if (ifarg(3) && chkarg(3, 0, 1) == 1.) {
            pwm_impl->do_print((int) chkarg(1, 0, 1), gargstr(2));
        } else if (ifarg(2)) {
            pwm_impl->do_print_session((int) chkarg(1, 0, 1), gargstr(2));
        } else {
            bool b = ifarg(1) ? (chkarg(1, 0, 1) == 1.) : true;
            pwm_impl->do_print_session(b);
        }
    }
    ENDGUI
#endif
    hoc_ret();
    hoc_pushx(0.);
}

void PrintableWindowManager::xplace(int left, int top, bool m) {
    PrintableWindow* w = pwm_impl->window();
    if (!w->is_mapped()) {
        PrintableWindow* pw = PrintableWindow::leader();
        if (pw && pw->is_mapped() && pw != w) {
            if (w->is_transient()) {
                w->transient_for(pw);
            } else {
                w->group_leader(pw);
            }
        }
        w->xplace(left, top);
    }
    if (m) {
        w->map();
        w->xmove(left, top);
    } else {
        w->hide();
    }
}

void PrintableWindowManager::update(Observable* o) {
    PrintableWindow* w = (PrintableWindow*) o;
    // printf("PrintableWindowManager::update(%p)\n", w);
    reconfigured(w);
}

void PrintableWindowManager::disconnect(Observable* o) {
    //	printf("disconnect %p\n", (PrintableWindow*)o);
}

void PrintableWindowManager::append(PrintableWindow* w) {
    // printf("PrintableWindowManager::append(%p)\n", w);
    if (w == NULL) {
        return;
    }
    w->attach(this);
    pwmi_->screen_->append(new ScreenItem(w));
    pwmi_->relabel();
    PrintableWindow* pw = PrintableWindow::leader();
    if (pw && pw->is_mapped() && pw != w) {
        if (w->is_transient()) {
            w->transient_for(pw);
            // printf("transient for %p\n", pw);
        } else {
            w->group_leader(pw);
            // printf("group leader is %p\n", pw);
        }
    }
}


void PrintableWindowManager::remove(PrintableWindow* w) {
    // printf("PrintableWindowManager::remove(%p)\n", w);
    PWMImpl* impl = pwmi_;
    if (w == impl->window()) {
        impl->w_ = NULL;
    }
    //	printf("remove %p\n", w);
    w->detach(this);
    Scene* s = impl->screen_;
    if (s) {
        GlyphIndex i = impl->index(w);
        if (i >= 0)
            s->remove(i);
    }
    impl->relabel();
}

#define PIXROUND(x, y, r) x = int((y + r / 2) / r) * r

void PrintableWindow::map_notify() {
    // only if leader
    if (this == leader()) {
        PrintableWindowManager::current()->pwmi_->map_all();
    }
}

// LCOV_EXCL_START
void PrintableWindow::reconfigured() {
    if (!pixres) {
        return;
    }
    PixelCoord x, y, x1, y1;
    x1 = xleft();
    y1 = xtop();
    PIXROUND(x, x1, pixres);
    PIXROUND(y, y1, pixres);
    if (x != x1 || y != y1) {
        xmove(x, y);
    }
}
// LCOV_EXCL_END

void ViewWindow::reconfigured() {
    if (!pixres) {
        return;
    }
    PixelCoord x, y, w, h;
    w = canvas()->pwidth();
    h = canvas()->pheight();
    PIXROUND(x, w, pixres);
    PIXROUND(y, h, pixres);
    if (x == 0)
        x = pixres;
    if (y == 0)
        y = pixres;
    if (x != w || y != h) {
        canvas()->psize(x, y);
        Window::resize();
    }
    PrintableWindow::reconfigured();
}

void PrintableWindowManager::reconfigured(PrintableWindow* w) {
    PWMImpl* impl = pwmi_;

    GlyphIndex i = impl->index(w);
    if (i < 0)
        return;  // mswin after a ShowWindow(hwnd, SW_HIDE);
    Coord l = w->left_pw();
    Coord r = l + w->width_pw();
    Coord b = w->bottom_pw();
    Coord t = b + w->height_pw();
    impl->screen_->move(i, l / Scl, b / Scl);
    impl->screen_->change(i);
    impl->screen_->show(i, w->is_mapped());
    ScreenItem* si = (ScreenItem*) impl->screen_->component(i);
    PaperItem* pi = si->paper_item();
    if (pi) {
        impl->paper_->change(impl->paper_index(pi));
    }
    Extension e;
    impl->all_window_bounding_box(e);
    impl->screen_->new_size(e.left() / Scl - 5,
                            e.bottom() / Scl - 2,
                            e.right() / Scl + 5,
                            e.top() / Scl + 2);
    VirtualWindow::view();
#if DBG
    Coord x, y;
    impl->screen_->location(i, x, y);
    printf("reconfigured %d %d %g %g\n", i, impl->screen_->showing(i), x, y);
#endif
}

void PrintableWindowManager::doprint() {
    pwmi_->do_print0();
}

void PWMImpl::help() {
    Oc::helpmode(!Oc::helpmode());
    Oc::helpmode(w_);
    if (Oc::helpmode()) {
        Oc::help(PWM_help_);
    }
}

void PWMImpl::all_window_bounding_box(Extension& e, bool with_screen, bool also_leader) {
    GlyphIndex i;
    PrintableWindow* w;
    Display* d = Session::instance()->default_display();
    if (with_screen) {
        e.set_xy(NULL, 0., 0., d->width(), d->height());
    } else {
        e.clear();
    }
    PrintableWindow* wl = PrintableWindow::leader();
    bool empty = true;
    for (i = 0; i < screen_->count(); i++) {
        w = ((ScreenItem*) (screen_->component(i)))->window();
        if (w && w->is_mapped() && w != wl) {
            e.merge_xy(
                NULL, w->left(), w->bottom(), w->left() + w->width(), w->bottom() + w->height());
            empty = false;
        }
    }
    w = wl;
    if (w && w->is_mapped() && (also_leader || empty)) {
        e.merge_xy(NULL, w->left(), w->bottom(), w->left() + w->width(), w->bottom() + w->height());
        print_leader_flag_ = true;
    } else {
        print_leader_flag_ = false;
    }
    screen_rect_->width(d->width() / Scl);
    screen_rect_->height(d->height() / Scl);
    // printf("all_window_bounding_box %g %g %g %g\n", e.left(), e.bottom(), e.right(), e.top());
}

void PWMImpl::view_screen(Coord x, Coord y) {
    int xp, yp;
    //	printf("view_sceen %g %g\n", x, y);
    GlyphIndex i;
    PrintableWindow* w;
    Display* d = Session::instance()->default_display();
    xp = d->to_pixels(-x) + d->pwidth() / 2;
    yp = d->to_pixels(y) - d->pheight() / 2;
    for (i = 0; i < screen_->count(); i++) {
        ScreenItem* si = (ScreenItem*) screen_->component(i);
        if (si->window()) {
            w = si->window();
            if (w != window()) {
                w->xmove(w->xleft() + xp, w->xtop() + yp);
            }
        }
    }
}

void PWMImpl::do_print0() {
    if (Oc::helpmode()) {
        Oc::help(PWM_do_print_);
        return;
    }
    if (use_printer) {
        if (none_selected("No windows to print", "Print Anyway")) {
            return;
        }

        if (!b_printer_) {
            printer_control();
            if (!printer_control_accept_) {
                Resource::unref(b_printer_);
                b_printer_ = NULL;
                return;
            }
        }
        CopyString name(b_printer_->text()->string());
        do_print(use_printer, name.string());
    } else {
        if (!fc_print_) {
            file_control();
            return;  // file_control calls do_print
        }
        do_print(use_printer, fc_print_->selected()->string());
    }
}

void PWMImpl::do_print(bool use_printer, const char* name) {
#if defined(WIN32)
    if (use_printer && strcmp(name, "Windows") == 0) {
        mac_do_print();
        return;
    }
#endif
    ps_file_print(use_printer, name, landscape_, false);
}

void PWMImpl::do_print_session(bool also_leader) {
    // must work for mac, mswin, unix. All windows on screen
    // scale so on paper
    bool p = true;
#if DECO
    bool deco = p_deco_->test(TelltaleState::is_chosen);
    p_deco_->set(TelltaleState::is_chosen, true);
#endif

#if MACPRINT
    Extension e;
#if defined(WIN32)
    if (!mprinter()->get()) {
        return;
    }
#endif
    all_window_bounding_box(e, false, also_leader);
    // want 1/2 inch margins
    float s1 = (mprinter()->width() - 72.) / (e.right() - e.left() + 6.);    // with deco
    float s2 = (mprinter()->height() - 72.) / (e.top() - e.bottom() + 23.);  // with deco
    float sfac = (s1 < s2) ? s1 : s2;
    float xoff = mprinter()->width() / 2 / sfac - (e.right() + e.left() + 6.) / 2.;
    float yoff = mprinter()->height() / 2 / sfac - (e.top() + e.bottom() + 23.) / 2.;
    Transformer t;
    t.translate(xoff, yoff);
    mprinter()->prolog(sfac);
    mprinter()->push_transform();
    mprinter()->transform(t);
    common_print(mprinter(), false, true);
    mprinter()->pop_transform();
    mprinter()->epilog();
#endif

#if !defined(WIN32)
    // must be a postscript printer so can use landscape mode
    if (!b_printer_) {
        printer_control();
        if (!printer_control_accept_) {
            Resource::unref(b_printer_);
            b_printer_ = NULL;
            p = false;
        }
    }
    if (p) {
        CopyString name(b_printer_->text()->string());
        ps_file_print(true, name.string(), true, true);
    }
#endif

#if DECO
    p_deco_->set(TelltaleState::is_chosen, deco);
#endif
    print_leader_flag_ = true;
}

void PWMImpl::do_print_session(bool use_printer, const char* name) {
    print_leader_flag_ = true;
    ps_file_print(use_printer, name, true, true);
}

void PWMImpl::ps_file_print(bool use_printer, const char* name, bool land_style, bool ses_style) {
    Style* s = Session::instance()->style();
    static char* tmpfile = (char*) 0;
    std::filebuf obuf;
    if (!tmpfile) {
        tmpfile = ivoc_get_temp_file();
    }
#ifdef WIN32
    unlink(tmpfile);
#endif
    obuf.open(tmpfile, IOS_OUT);
    std::ostream o(&obuf);
    Printer* pr = new Printer(&o);
    pr->prolog();

    if (ses_style) {
#if DECO
        bool deco = p_deco_->test(TelltaleState::is_chosen);
        p_deco_->set(TelltaleState::is_chosen, true);
#endif
        Style* s = Session::instance()->style();
        Coord pageheight;
        Coord pagewidth;
        if (!s->find_attribute("pwm_paper_height", pageheight)) {
            pageheight = 11.;
        }
        if (!s->find_attribute("pwm_paper_width", pagewidth)) {
            pagewidth = 8.5;
        }
        Extension e;
        all_window_bounding_box(e, false, true);
        // want 1/2 inch margins
        float s1 = (pagewidth * 72 - 72.) / (e.right() - e.left() + 6.);    // with deco
        float s2 = (pageheight * 72 - 72.) / (e.top() - e.bottom() + 23.);  // with deco
        float sfac = (s1 < s2) ? s1 : s2;
        float xoff = pagewidth * 72 / 2 / sfac - (e.right() + e.left() + 6.) / 2.;
        float yoff = pageheight * 72 / 2 / sfac - (e.top() + e.bottom() + 23.) / 2.;
        Transformer t;
        t.translate(xoff, yoff);
        t.scale(sfac, sfac);
        pr->push_transform();
        pr->transform(t);
        common_print(pr, false, ses_style);
        pr->pop_transform();
#if DECO
        p_deco_->set(TelltaleState::is_chosen, deco);
#endif
    } else {
        common_print(pr, land_style, ses_style);
    }
    pr->epilog();
    obuf.close();

    String filt("cat");
    s->find_attribute("pwm_postscript_filter", filt);
    auto const buf_size = 200 + strlen(name) + strlen(filt.string()) + 2 * strlen(tmpfile);
    char* buf = new char[buf_size];

    if (use_printer) {
#ifdef WIN32
        std::snprintf(buf, buf_size, "%s %s %s", filt.string(), tmpfile, name);
#else
        std::snprintf(
            buf, buf_size, "%s < %s |  %s ; rm %s", filt.string(), tmpfile, name, tmpfile);
#endif
    } else {
#ifdef WIN32
        std::snprintf(buf, buf_size, "%s %s > %s", filt.string(), tmpfile, name);
#else
        std::snprintf(buf, buf_size, "%s < %s > %s ; rm %s", filt.string(), tmpfile, name, tmpfile);
#endif
    }
    // printf("%s\n", buf);
    nrnignore = system(buf);
    delete[] buf;
#ifdef WIN32
    unlink(tmpfile);
#endif
    delete pr;  // input handlers later crash doing pr->damage()
}

#ifdef WIN32
extern bool hoc_copyfile(const char*, const char*);
#endif

#if MACPRINT
void PWMImpl::mac_do_print() {
#if defined(WIN32)
    if (!mprinter()->get()) {
        return;
    }
#endif
    mprinter()->prolog();
    common_print(mprinter(), landscape_, false);
    mprinter()->epilog();
}
#endif

void PWMImpl::common_print(Printer* pr, bool land_style, bool ses_style) {
    Scene* p;
    if (ses_style) {
        p = screen();
    } else {
        p = paper();
    }
    Style* s = Session::instance()->style();
    Coord pageheight;
    Coord pagewidth;
    if (!s->find_attribute("pwm_paper_height", pageheight)) {
        pageheight = 11.;
    }
    if (!s->find_attribute("pwm_paper_width", pagewidth)) {
        pagewidth = 8.5;
    }
    pr->resize(0, 0, pagewidth * 72, pageheight * 72);
    if (land_style) {
        Transformer t;
        t.rotate(-90);
        // t.translate(0, pageheight*72);
        if (ses_style) {
            t.translate(20, pr->height() - 70);
        } else {
            t.translate(0, pr->height());
        }
        pr->transform(t);
    }
    GlyphIndex count = p->count();
    for (GlyphIndex i = 0; i < count; ++i) {
        if (!p->showing(i)) {
            continue;
        }
        PrintableWindow* pw;
        float sfac;
        Transformer t;
        Coord x, y, x1, y1;
        if (ses_style) {
            ScreenItem* pi = (ScreenItem*) p->component(i);
            pw = pi->window();
            if (!pw->is_mapped()) {
                continue;
            }  // probably the PFWM
            if (!print_leader_flag_ && pw == PrintableWindow::leader()) {
                continue;
            }
            float sfac = .9 * pagewidth * 72 / pw->display()->height();
            sfac = 1.;
            x = pw->left_pw();
            y = pw->bottom_pw();
            t.translate(x, y);
            t.scale(sfac, sfac);
            x1 = (x) *sfac;
            y1 = (y + pw->height_pw()) * sfac;
        } else {
            PaperItem* pi = (PaperItem*) p->component(i);
            pw = pi->screen_item()->window();
            float sfac = pr_scl * 72 * pi->scale() / Scl;
            p->location(i, x, y);
            t.scale(sfac, sfac);
            t.translate(72 * x * pr_scl, 72 * y * pr_scl);
            // t.translate((pr->width() - pagewidth*72)/2,  (pr->height() - pageheight*72)/2);
            x1 = 72 * (x) *pr_scl;
            y1 = 72 * (y + pi->width() * pw->height_pw() / pw->width_pw() * pi->scale()) * pr_scl;
        }
        Requisition req;
        pw->print_glyph()->request(req);
        float align_x = req.x_requirement().alignment();
        float align_y = req.y_requirement().alignment();
        Allotment ax(align_x * pw->width_pw(), pw->width_pw(), align_x);
        Allotment ay(align_y * pw->height_pw(), pw->height_pw(), align_y);
        Allocation a;
        a.allot_x(ax);
        a.allot_y(ay);

        pr->push_transform();
        pr->transform(t);
        pr->push_clipping();
        pr->clip_rect(0, 0, pw->width_pw(), pw->height_pw());

        pw->print_glyph()->print(pr, a);  // FieldEditor glyphs crash
        pr->pop_clipping();
#if DECO
        if (p_deco_->test(TelltaleState::is_chosen) == true) {
            print_deco(pr, a, pw->name());
        }
#endif
        pr->pop_transform();

        // flush the allocation tables for InputHandler glyphs so
        // no glyphs try to use the Printer after it has been deleted
        pw->print_glyph()->undraw();
        redraw(pw);
        // print the window titles
        if ((ses_style || p_title_->test(TelltaleState::is_chosen) == true)
#if DECO
            && p_deco_->test(TelltaleState::is_chosen) == false
#endif
        ) {
            WidgetKit& wk = *WidgetKit::instance();
            Label label(pw->name(), wk.font(), wk.foreground());
            Requisition r;
            label.request(r);
            Allocation al;
            Allotment& alx = al.x_allotment();
            Allotment& aly = al.y_allotment();
            alx.origin(x1);
            alx.span(r.x_requirement().natural());
            aly.origin(y1);
            aly.span(r.y_requirement().natural());
            label.draw(pr, al);
        }
    }
}

#if DECO
// for Carnevale, Hines book figures

void PWMImpl::print_deco(Printer* pr, Allocation& a, const char* title) {
    WidgetKit& wk = *WidgetKit::instance();
    Coord l, b, r, t, w, h, s, x, y, dx, xx;

    // attributes
    static const Color* ctitle;
    static const Color* ctitlebar;
    static const Color* coutline;
    static const Color* bright;
    static const Color* dark;
    static const Font* ftitle;
    static const Brush* br;
    static int first = 1;

    w = 3;
    h = 20;   // width of outer deco, height of title bar
    s = 2;    // close button offset from bottom of title bar
    xx = 10;  // x part of close button size

    if (first) {
        first = 0;
        bright = new Color(.9, .9, .9, 1.);
        bright->ref();
        dark = new Color(.1, .1, .1, 1.);
        dark->ref();
        ctitle = new Color(0., 0., 0., 1.);
        ctitle->ref();
        ctitlebar = new Color(.8, .8, .8, 1.);
        ctitlebar->ref();
        coutline = new Color(.7, .7, .7, 1.);
        coutline->ref();
        br = new Brush(1);
        br->ref();
        ftitle = wk.font();
        ftitle->ref();
    }

    l = a.left();
    b = a.bottom();
    r = a.right();
    t = a.top();  // inside

    // title bar
    pr->fill_rect(l, t, r, t + h, ctitlebar);

    // title
    Label label(title, ftitle, ctitle);
    Requisition req;
    label.request(req);
    x = req.x_requirement().natural();
    y = req.y_requirement().natural();
    Allocation al;
    Coord xo = (l + r) / 2 - x / 2;
    xo = (xo < h) ? h : xo;
    al.allot_x(Allotment(xo, x, 0.));
    al.allot_y(Allotment(t + h / 2 - y / 3, y, 0.));
    // clip the title
    pr->push_clipping();
    pr->clip_rect(l + h, t, r, t + h);
    label.draw(pr, al);
    pr->pop_clipping();

    // outline
    pr->fill_rect(l, b - w, l - w, t + h + w, coutline);  // left
    pr->fill_rect(r, b - w, r + w, t + h + w, coutline);  // right
    pr->fill_rect(l, b, r, b - w, coutline);              // bottom
    pr->fill_rect(l, t + h, r, t + h + w, coutline);      // top
    pr->rect(l - w, b - w, r + w, t + h + w, dark, br);   // outside boundary

    // close button
    x = (l + (l + h - s)) / 2;
    y = (t + s + t + h) / 2;
    dx = (h - s) / 2;
    pr->rect(x - dx, y - dx, x + dx, y + dx, bright, br);
    dx = xx / 2;
    pr->line(x - dx, y - dx, x + dx, y + dx, bright, br);  // the x
    pr->line(x - dx, y + dx, x + dx, y - dx, bright, br);  // the x
}
#endif

void PrintableWindowManager::psfilter(const char* filename) {
    static char* tmpfile = (char*) 0;
    if (!tmpfile) {
        tmpfile = ivoc_get_temp_file();
    }
    Style* s = Session::instance()->style();
    char buf[512];
    String filt("cat");
    if (s->find_attribute("pwm_postscript_filter", filt)) {
        Sprintf(
            buf, "cat %s > %s; %s < %s > %s", filename, tmpfile, filt.string(), tmpfile, filename);
        nrnignore = system(buf);
        unlink(tmpfile);
    }
}

#if defined(WIN32)
void pwmimpl_redraw(Window* pw);
#endif

void PWMImpl::redraw(Window* pw) {
    // redraw the canvas so HocValEditor fields show up
    if (!pw->is_mapped()) {
        return;
    }
    Canvas* c = pw->canvas();
    c->damage_all();
#if defined(WIN32)
    pwmimpl_redraw(pw);
#else
    Requisition req;
    Allocation a;
    Coord xsize = c->width();
    Coord ysize = c->height();
    pw->glyph()->request(req);
    Coord ox = xsize * req.x_requirement().alignment();
    Coord oy = ysize * req.y_requirement().alignment();
    a.allot_x(Allotment(ox, xsize, ox / xsize));
    a.allot_y(Allotment(oy, ysize, oy / ysize));
    Transformer t1;
    c->push_transform();
    c->transformer(t1);
    pw->glyph()->draw(c, a);
    c->pop_transform();
#endif
}

// ScreenItem
ScreenItem::ScreenItem(PrintableWindow* w) {
    w_ = w;
    label_ = NULL;
    pi_ = NULL;
    i_ = -1;
    group_obj_ = NULL;
    iconify_via_hide_ = false;
}

ScreenItem::~ScreenItem() {
    //	printf("~ScreenItem\n");
    if (pi_) {
        pi_->si_ = NULL;
        if (pwm_impl) {
            pwm_impl->remove_paper(pi_);
        }
        Resource::unref(pi_);
        pi_ = NULL;
    }
    hoc_obj_unref(group_obj_);
    Resource::unref(label_);
}

void ScreenItem::relabel(GlyphIndex i) {
    char buf[10];
    Sprintf(buf, "%ld", i);
    i_ = i;
    Glyph* g = WidgetKit::instance()->label(buf);
    Resource::ref(g);
    Resource::unref(label_);
    label_ = g;
}

#if 0
void ScreenItem::reconfigured(Scene* s) {
	Coord l = w_->left_pw();
	Coord r = l + w_->width_pw();
	Coord b = w_->bottom_pw();
	Coord t = b + w_->height_pw();
	s->move(i, l/Scl, b/Scl);
	Coord x, y;
	s->location(i, x, y);
#if DBG
	printf("reconfigured %d %d %g %g\n", i, s->showing(i), x, y);
#endif
	if (w_->is_mapped()) {
		impl->w_->canvas()->damage_all();
	}
}
#endif

void ScreenItem::request(Requisition& req) const {
    Coord w, h;
    if (w_) {
        w = w_->width_pw() / Scl;
        h = w_->height_pw() / Scl;
    }
    Requirement rx(w + 2);
    Requirement ry(h + 2);
    req.require_x(rx);
    req.require_y(ry);
#if DBG
    printf("ScreenItem::request %d\n", index());
#endif
}

void ScreenItem::allocate(Canvas* c, const Allocation& a, Extension& ext) {
    ext.set(c, a);
    MyMath::extend(ext, 1);
#if DBG
    printf("ScreenItem::allocate %d\n", index());
#endif
}

void ScreenItem::draw(Canvas* c, const Allocation& a) const {
#if DBG
    printf("ScreenItem::draw %d\n", i_);
// print_alloc(c,"draw", a);
#endif
    Coord x = a.x();
    Coord y = a.y();
    if (w_) {
        c->rect(x,
                y,
                x + (w_->width_pw()) / Scl,
                y + (w_->height_pw()) / Scl,
                pwm_impl->window_outline_,
                NULL);
    }
    label_->draw(c, a);
}

void ScreenItem::pick(Canvas* c, const Allocation& a, int depth, Hit& h) {
    Coord x = h.left();
    Coord y = h.bottom();
    //	if (x >= 0 && x <= w_->width_pw() && y >= 0 && y <= w_->height_pw()) {
    if (inside(x, y, a)) {
        h.target(depth, this, 0);
        if (h.event()->type() == Event::down) {
            if (Oc::helpmode()) {
                Oc::help(PWM_ScreenItem_);
                return;
            }
            switch (pwm_impl->tool(h.event()->pointer_button())) {
            case Event::left:
                h.target(depth, this, 0, new ScreenItemHandler(x, y, this, c->transformer()));
                break;
            case Event::right:
                if (w_) {
                    pwm_impl->append_paper(this);
                }
                break;
            }
#if DBG
            printf("ScreenItem::pick %d hit(%g,%g)\n", i_, x, y);
            print_alloc(NULL, "ScreenItem", a);
#endif
        }
    }
}


// PaperItem
PaperItem::PaperItem(ScreenItem* s) {
    scale_ = 1.;
    si_ = s;
    s->pi_ = this;
    ref();
}
PaperItem::~PaperItem() {
    //	printf("~PaperItem\n");
    if (si_) {
        si_->pi_ = NULL;
    }
    si_ = NULL;
}

Coord PaperItem::width() {
    return si_->w_->width_pw() / Scl;
}
Coord PaperItem::fsize_;

void PaperItem::request(Requisition& req) const {
    Requirement rx(scale_ * si_->w_->width_pw() / Scl);
    Requirement ry(std::max(fsize_, scale_ * si_->w_->height_pw() / Scl));
    req.require_x(rx);
    req.require_y(ry);
#if DBG
    printf("PaperItem::request %d\n", screen_item()->index());
#endif
}

void PaperItem::allocate(Canvas* c, const Allocation& a, Extension& ext) {
    ext.set(c, a);
    MyMath::extend(ext, 1);
#if DBG
    printf("PaperItem::allocate %d\n", screen_item()->index());
#endif
}

void PaperItem::draw(Canvas* c, const Allocation& a) const {
#if DBG
    printf("PaperItem::draw %d\n", si_->i_);
#endif
    Coord x = a.x();
    Coord y = a.y();
    c->rect(x,
            y,
            x + scale_ * (si_->w_->width_pw()) / Scl,
            y + scale_ * (si_->w_->height_pw()) / Scl,
            pwm_impl->window_outline_,
            NULL);
    si_->label_->draw(c, a);
}

#if 0
void PaperItem::print(Printer* pr, const Allocation& a) const {
	Coord x = a.x();
	Coord y = a.y();
	pr->rect(x, y, x + scale_*(si_->w_->width_pw())/Scl, y + scale_*(si_->w_->height_pw())/Scl, blue, NULL);
	pr->push_transform();
	Transformer t(scale_/Scl, 0, 0, scale_/Scl, 0, 0);
	Allocation b = a;
	pr->transform(t);
	si_->w_->glyph()->print(pr, a);
	pr->pop_transform();
}
#endif
void PaperItem::pick(Canvas* c, const Allocation& a, int depth, Hit& h) {
    Coord x = h.left();
    Coord y = h.bottom();
    if (::inside(x, y, a)) {
        h.target(depth, this, 0);
        if (h.event()->type() == Event::down) {
            if (Oc::helpmode()) {
                Oc::help(PWM_PaperItem_);
                return;
            }
            switch (pwm_impl->tool(h.event()->pointer_button())) {
            case Event::left:
                h.target(
                    depth,
                    this,
                    0,
                    new PaperItem_handler(PaperItem_handler::move, x, y, this, c->transformer()));
                break;
            case Event::middle:
                h.target(
                    depth,
                    this,
                    0,
                    new PaperItem_handler(PaperItem_handler::resize, x, y, this, c->transformer()));
                break;
            case Event::right:
                pwm_impl->unshow_paper(this);
                break;
            }
#if DBG
            printf("PaperItem::pick %d hit(%g, %g)\n", si_->i_, x, y);
#endif
        }
    }
}

// PWMImpl
PWMImpl::PWMImpl(ScreenScene* screen, PaperScene* paper, Rect* prect) {
    screen_ = screen;
    paper_ = paper;
    Resource::ref(screen);
    Resource::ref(paper);
    w_ = NULL;
    landscape_ = false;
    prect_ = prect;
    printer_ = "lp";
    use_printer = true;
    printer_control_accept_ = true;
    b_printer_ = NULL;
    fc_print_ = NULL;
    fc_idraw_ = NULL;
    fc_ascii_ = NULL;
    fc_save_ = NULL;
    fc_retrieve_ = NULL;
    p_title_ = NULL;
#if MACPRINT
    mprinter_ = NULL;
#endif
    tool_ = Event::right;
}

PWMImpl::~PWMImpl() {
    //	printf("~PWMImpl\n");
    Resource::unref(screen_);
    screen_ = NULL;
    Resource::unref(paper_);
    paper_ = NULL;
    Resource::unref(b_printer_);
    Resource::unref(fc_print_);
    Resource::unref(fc_idraw_);
    Resource::unref(fc_ascii_);
    Resource::unref(fc_save_);
    Resource::unref(fc_retrieve_);
    Resource::unref(screen_rect_);
#if MACPRINT
    if (mprinter_) {
        delete mprinter_;
    }
#endif
}

#if MACPRINT
MacPrinter* PWMImpl::mprinter() {
    if (!mprinter_) {
        mprinter_ = new MacPrinter();
    }
    return mprinter_;
}
#endif

void PWMImpl::map_all() {
    GlyphIndex i;
    PrintableWindow* pw = PrintableWindow::leader();
    PrintableWindow* w;
    if (screen_)
        for (i = 0; i < screen_->count(); i++) {
            ScreenItem* si = (ScreenItem*) (screen_->component(i));
            w = si->window();
            if (w) {
                if (w != pw) {
                    if (si->iconify_via_hide_ == true) {
                        w->map();
                    }
                } else {
                    //			w->deiconify();
                }
            }
        }
}
void PWMImpl::unmap_all() {
    GlyphIndex i;
    PrintableWindow* pw = PrintableWindow::leader();
    PrintableWindow* w;
    if (screen_)
        for (i = 0; i < screen_->count(); i++) {
            ScreenItem* si = (ScreenItem*) (screen_->component(i));
            w = si->window();
            if (w) {
                if (w != pw) {
                    if (screen_->showing(i)) {
                        w->hide();
                        si->iconify_via_hide_ = true;
                    } else {
                        si->iconify_via_hide_ = false;
                    }
                } else {
                    w->iconify();
                }
            }
        }
}

GlyphIndex PWMImpl::index(void* w) {
    GlyphIndex i;
    if (screen_)
        for (i = 0; i < screen_->count(); i++) {
            ScreenItem* si = (ScreenItem*) screen_->component(i);
            if (w == (void*) si->window()) {
                return i;
            }
        }
    return -1;
}

void PWMImpl::relabel() {
    GlyphIndex i;
    for (i = 0; i < screen_->count(); i++) {
        ((ScreenItem*) screen_->component(i))->relabel(i);
    }
    //	if (w_) {
    //		w_->canvas()->damage_all();
    //	};
}

void PWMImpl::append_paper(ScreenItem* si) {
    PaperItem* pi = si->paper_item();
    GlyphIndex i;
    if (pi) {
        i = paper_index(pi);
        paper_->show(i, true);
    } else {
        pi = new PaperItem(si);
        pi->scale(.9);
        paper_->append(pi);
        i = paper_index(pi);
        Coord x = si->window()->left_pw() / Scl;
        Coord y = si->window()->bottom_pw() / Scl;
        x = (x < 0.) ? 0. : x;
        y = (y < 0.) ? 0. : y;
        x = (x > paper_->x2() * .8) ? paper_->x2() * .8 : x;
        y = (y > paper_->y2() * .8) ? paper_->y2() * .8 : y;
        paper_->move(i, x, y);
    }
    paper_->change(i);
}

void PWMImpl::unshow_paper(PaperItem* pi) {
    paper_->show(paper_index(pi), false);
}

void PWMImpl::remove_paper(PaperItem* pi) {
    GlyphIndex i = paper_index(pi);
    if (paper_ && i != -1) {
        paper_->remove(i);
    }
}

GlyphIndex PWMImpl::paper_index(PaperItem* pi) {
    GlyphIndex i;
    if (paper_)
        for (i = 0; i < paper_->count(); i++) {
            if (pi == (PaperItem*) paper_->component(i)) {
                return i;
            }
        }
    return -1;
}

float PWMImpl::round(float x) {
    return std::round(x / round_factor_) * round_factor_;
}

#if MACPRINT
void PWMImpl::paperscale() {
    mprinter()->setup();
    Coord w, h, x;
    w = mprinter()->width() / 72.;
    x = h = mprinter()->height() / 72.;
    if (w > h) {
        x = w;
    }
    x = x / canvasheight_;
    prect_->width(w / pr_scl);
    prect_->height(h / pr_scl);
    pview_->box_size(-.2, -.2, prect_->width() + .2, prect_->height() + .2);

    paper_->damage_all();
}
#endif


void PWMImpl::landscape() {
    if (Oc::helpmode()) {
        Oc::help(PWM_landscape_);
    }
    Coord w, h;
    w = prect_->width();
    h = prect_->height();
    prect_->width(h);
    prect_->height(w);
    paper_->damage_all();
    landscape_ = !landscape_;
}

void PWMImpl::landscape(bool b) {
    if (landscape_ != b) {
        landscape();
    }
}

void PWMImpl::deco(int i) {
    p_title_->set(TelltaleState::is_chosen, false);
    p_deco_->set(TelltaleState::is_chosen, false);
    if (i == 1) {
        p_title_->set(TelltaleState::is_chosen, true);
    } else if (i == 2) {
        p_deco_->set(TelltaleState::is_chosen, true);
    }
}

void PWMImpl::virt_screen() {
    if (Oc::helpmode()) {
        Oc::help(PWM_virt_screen_);
        return;
    }
    VirtualWindow::makeVirtualWindow();
}

// grabbed from unidraw dialogs.cpp
static const char* DefaultPrintCmd() {
#ifdef WIN32
    Style* style = Session::instance()->style();
    static String str;
    if (style->find_attribute("printer_command", str)) {
        ;
    } else {
        str = " > prn";
    }
    return str.string();
#else
    static char buf[200];
    static const char* print_cmd = getenv("PRINT_CMD");
    if (print_cmd == NULL) {
        const char* printer_name = getenv("PRINTER");

        if (printer_name == NULL) {
            Sprintf(buf, "lpr");
        } else {
            Sprintf(buf, "lpr -P%s", printer_name);
        }
        print_cmd = buf;
    }
    return print_cmd;
#endif
}

void PWMImpl::printer_control() {
    if (Oc::helpmode()) {
        Oc::help(PWM_printer_control_);
    }
    if (!b_printer_) {
        Style* style = new Style(Session::instance()->style());
        style->attribute("caption", "Postscript Printer Command");
        b_printer_ = FieldDialog::field_dialog_instance(DefaultPrintCmd(), style);
        b_printer_->ref();
    }
    use_printer = true;
    bool b;
    if (w_ && w_->is_mapped()) {
        b = b_printer_->post_for(w_);
    } else {
        Coord x, y, ax, ay;
        if (nrn_spec_dialog_pos(x, y)) {
            ax = 0.0;
            ay = 0.0;
        } else {  // original default
            x = 300.;
            y = 500.;
            ax = 0.5;
            ay = 0.5;
        }

        b = b_printer_->post_at_aligned(x, y, ax, ay);
    }
    if (b) {
        printer_control_accept_ = true;
    } else {
        printer_control_accept_ = false;
    }
}

void PWMImpl::quit_control() {
    if (Oc::helpmode()) {
        Oc::help(PWM_quit_);
        return;
    }

    if (boolean_dialog("Quit. Are you sure?", "Yes", "No", w_)) {
        Oc oc;
        oc.run("quit()\n");
    }
}

#if SNAPSHOT
void PWMImpl::snapshot_control() {
    if (file_control1()) {
        ivoc_snapshot_ = ivoc_snapshot;
    }
}
#endif

bool PWMImpl::file_control1() {
    if (Oc::helpmode()) {
        Oc::help(PWM_file_control_);
    }
    if (!fc_print_) {
        Style* style = new Style(Session::instance()->style());
        String str;
        if (style->find_attribute("pwm_print_file_filter", str)) {
            style->attribute("filter", "true");
            style->attribute("filterPattern", str);
        }
        style->attribute("caption", "Print Postscript to file");
        style->attribute("open", "Print to file");
        fc_print_ = DialogKit::instance()->file_chooser("./", style);
        fc_print_->ref();
    } else {
        fc_print_->reread();
    }
    while (fc_print_->post_for(w_)) {
        if (ok_to_write(*fc_print_->selected(), w_)) {
            return true;
        }
    }
    return false;
}

void PWMImpl::file_control() {
    if (none_selected("No windows to save", "Save Anyway")) {
        return;
    }
    if (file_control1()) {
        use_printer = false;
        do_print0();
        use_printer = true;
    }
}

#if SNAPSHOT
void PWMImpl::snapshot(const Event* e) {
    snap_event_ = e;
    std::filebuf obuf;
    obuf.open(fc_print_->selected()->string(), IOS_OUT);
    std::ostream o(&obuf);
    Printer* pr = new Printer(&o);
    pr->prolog();
    pr->resize(0, 0, 1200, 1000);
    Window* w = e->window();
    snap_owned(pr, w);
    //	for (w = e->window(); w; w = snap_owned(w)) {
    //		snap(pr, w);
    //	}
    snap_cursor(pr, e);
    pr->epilog();
    obuf.close();
    delete pr;
}

void PWMImpl::snap(Printer* pr, Window* w) {
    Transformer t;
    t.translate(w->left(), w->bottom());
    Requisition req;
    Glyph* g = w->Window::glyph();
    g->request(req);
    float align_x = req.x_requirement().alignment();
    float align_y = req.y_requirement().alignment();
    Allotment ax(align_x * w->width(), w->width(), align_x);
    Allotment ay(align_y * w->width(), w->height(), align_y);
    Allocation a;
    a.allot_x(ax);
    a.allot_y(ay);
    // printf("left=%g right=%g top=%g bottom=%g\n", a.left(), a.right(), a.top(), a.bottom());
    t.translate(a.left(), -a.bottom());
    Style* s = w->style();
    String str;
    bool pd = false;
    if (s && s->find_attribute("name", str)) {
        pd = true;
        pr->comment(str.string());
    }
    char buf[256];
    if (pd) {
        Sprintf(buf,
                "BoundingBox: %g %g %g %g",
                w->left() - 3,
                w->bottom() - 3,
                w->left() + w->width() + 3,
                w->bottom() + w->height() + 20 + 3);
        pr->comment(buf);
        Sprintf(buf, "\\begin{picture}(%g, %g)", w->width() + 6, w->height() + 23);
        pr->comment(buf);
    } else {
        Sprintf(buf,
                "BoundingBox: %g %g %g %g",
                w->left(),
                w->bottom(),
                w->left() + w->width(),
                w->bottom() + w->height());
        pr->comment(buf);
        Sprintf(buf, "\\begin{picture}(%g, %g)", w->width(), w->height());
        pr->comment(buf);
    }
    pr->push_transform();
    pr->transform(t);
    g->print(pr, a);
    if (pd) {
        print_deco(pr, a, str.string());
    }
    g->undraw();
    pr->pop_transform();
    // at this point cursor will be below any pop up windows so must be done last
    //	if (w == snap_event_->window()) {
    //		snap_cursor(pr, snap_event_);
    //	}
    pr->comment("End BoundingBox");
}

void PWMImpl::snap_cursor(Printer* pr, const Event* e) {
    Rubberband* rb = Rubberband::current();
    if (rb && rb->canvas()->window() == e->window()) {
        pr->comment("Begin Rubberband");
        Transformer t1;
        t1.translate(e->window()->left(), e->window()->bottom());
        pr->push_transform();
        pr->transform(t1);
        rb->snapshot(pr);
        pr->pop_transform();
        pr->comment("End Rubberband");
    }

    Coord x, y;
    x = e->pointer_x();
    y = e->pointer_y();

    Transformer t;
    t.rotate(30);
    t.translate(e->window()->left(), e->window()->bottom());
    t.translate(x, y);
    pr->comment("Begin cursor");
    pr->push_transform();
    pr->transform(t);
    pr->new_path();
    pr->move_to(0, 0);
    pr->line_to(8, -14);
    pr->line_to(2, -12);
    pr->line_to(2, -20);
    pr->line_to(-2, -20);
    pr->line_to(-2, -12);
    pr->line_to(-8, -14);
    pr->close_path();
    pr->fill(WidgetKit::instance()->foreground());
    pr->stroke(WidgetKit::instance()->background(), Appear::default_brush());
    pr->pop_transform();
    pr->comment("End cursor");
}
#endif

void PWMImpl::idraw_control() {
    if (Oc::helpmode()) {
        Oc::help(PWM_idraw_control_);
    }
    if (!fc_idraw_) {
        Style* style = new Style(Session::instance()->style());
        String str;
        if (style->find_attribute("pwm_idraw_file_filter", str)) {
            style->attribute("filter", "true");
            style->attribute("filterPattern", str);
        }
        style->attribute("caption", "Idraw format to file");
        style->attribute("open", "Write to file");
        fc_idraw_ = DialogKit::instance()->file_chooser("./", style);
        fc_idraw_->ref();
    } else {
        fc_idraw_->reread();
    }
    if (none_selected("No windows to save", "Save Anyway")) {
        return;
    }
    while (fc_idraw_->post_for(w_)) {
        if (ok_to_write(*fc_idraw_->selected(), w_)) {
            idraw_write(fc_idraw_->selected()->string());
            break;
        }
    }
}

void PWMImpl::idraw_write(const char* fname, bool ses_style) {
#ifdef WIN32
    unlink(fname);
#endif
    std::filebuf obuf;
    obuf.open(fname, IOS_OUT);
    std::ostream o(&obuf);
    OcIdraw::idraw_stream = &o;
    OcIdraw::prologue();
    Scene* p = paper();
    GlyphIndex count = p->count();
    if (ses_style) {
        for (GlyphIndex i = 0; i < screen()->count(); ++i) {
            ScreenItem* pi = (ScreenItem*) screen()->component(i);
            redraw(pi->window());
        }
    } else {
        for (GlyphIndex i = 0; i < count; ++i) {
            if (!p->showing(i)) {
                continue;
            }
            PaperItem* pi = (PaperItem*) p->component(i);
            redraw(pi->screen_item()->window());
        }
    }
    OcIdraw::epilog();
    obuf.close();
    OcIdraw::idraw_stream = NULL;
}

void PWMImpl::ascii_control() {
    if (Oc::helpmode()) {
        Oc::help(PWM_ascii_);
    }
    if (!fc_ascii_) {
        Style* style = new Style(Session::instance()->style());
        String str;
        if (style->find_attribute("pwm_ascii_file_filter", str)) {
            style->attribute("filter", "true");
            style->attribute("filterPattern", str);
        }
        style->attribute("caption", "Ascii format to file");
        style->attribute("open", "Write to file");
        fc_ascii_ = DialogKit::instance()->file_chooser("./", style);
        fc_ascii_->ref();
    } else {
        fc_ascii_->reread();
    }
    if (none_selected("No windows to save", "Save Anyway")) {
        return;
    }
    while (fc_ascii_->post_for(w_)) {
        if (ok_to_write(*fc_ascii_->selected(), w_)) {
            ascii_write(fc_ascii_->selected()->string());
            break;
        }
    }
}

void PWMImpl::ascii_write(const char* fname, bool ses_style) {
    std::filebuf obuf;
#ifdef WIN32
    unlink(fname);
#endif
    obuf.open(fname, IOS_OUT);
    std::ostream o(&obuf);
    Graph::ascii(&o);
    Scene* p = paper();
    GlyphIndex count = p->count();
    if (ses_style) {
        for (GlyphIndex i = 0; i < screen()->count(); ++i) {
            ScreenItem* pi = (ScreenItem*) screen()->component(i);
            redraw(pi->window());
        }
    } else {
        for (GlyphIndex i = 0; i < count; ++i) {
            if (!p->showing(i) && !ses_style) {
                continue;
            }
            PaperItem* pi = (PaperItem*) p->component(i);
            redraw(pi->screen_item()->window());
        }
    }
    obuf.close();
    Graph::ascii(NULL);
}

std::ostream* Oc::save_stream;

void PWMImpl::save_selected_control() {
    save_control(1);
}
void PWMImpl::save_all_control() {
    save_control(2);
}
bool PWMImpl::none_selected(const char* title, const char* accept) const {
    int i, n = 0;
    if (paper_)
        for (i = 0; i < paper_->count(); ++i) {
            if (paper_->showing(i)) {
                ++n;
            }
        }
    if (n == 0) {
        if (!boolean_dialog(title, accept, "Cancel", w_)) {
            return true;
        }
    }
    return false;
}

void PWMImpl::save_control(int mode) {
    if (Oc::helpmode()) {
        if (mode == 2) {
            Oc::help(PWM_save_control2_);
        } else {
            Oc::help(PWM_save_control1_);
        }
    }
    if (!fc_save_) {
        if (mode == 1) {
            if (none_selected("No windows to save", "Save Anyway")) {
                return;
            }
        }
        Style* style = new Style(Session::instance()->style());
        String str;
        if (style->find_attribute("pwm_save_file_filter", str)) {
            style->attribute("filter", "true");
            style->attribute("filterPattern", str);
        }
        style->attribute("caption", "Save windows on paper icon to file");
        style->attribute("open", "Save to file");
        fc_save_ = DialogKit::instance()->file_chooser("./", style);
        fc_save_->ref();
    } else {
        fc_save_->reread();
    }
    while (fc_save_->post_for(w_)) {
        if (ok_to_write(*fc_save_->selected(), w_)) {
            save_session(mode, fc_save_->selected()->string());
            break;
        }
    }
}

int PWMImpl::save_group(Object* ho, const char* filename) {
    int i;
    ScreenItem* si;
    ScreenItem** sivec = NULL;
    int nwin = 0;
    if (screen_ && screen_->count()) {
        sivec = new ScreenItem*[screen_->count()];
        for (i = 0; i < screen_->count(); i++) {
            si = (ScreenItem*) (screen_->component(i));
            if (si->group_obj_ == ho) {
                sivec[nwin++] = si;
            }
        }
    }
    if (nwin > 0) {
        cur_ses_name_ = filename;
        std::filebuf obuf;
#ifdef WIN32
        unlink(filename);
#endif
        obuf.open(filename, IOS_OUT);
        std::ostream o(&obuf);
        save_begin(o);
        save_list(nwin, sivec, o);
        obuf.close();
    }
    if (sivec) {
        delete[] sivec;
    }
    return nwin;
}

void PWMImpl::save_session(int mode, const char* filename, const char* head) {
    int nwin = 0;
    ScreenItem* si;
    ScreenItem** sivec = NULL;

    std::filebuf obuf;
    cur_ses_name_ = filename;
#ifdef WIN32
    unlink(filename);
#endif
    obuf.open(filename, IOS_OUT);
    if (!obuf.is_open()) {
        hoc_execerror(filename, "is not open for writing");
    }
    std::ostream o(&obuf);
    if (head) {
        o << head << std::endl;
    }
    save_begin(o);

    PrintableWindow* w;
    GlyphIndex i;
    if (mode == 2) {
        if (screen_ && screen_->count()) {
            sivec = new ScreenItem*[screen_->count()];
            for (i = 0; i < screen_->count(); i++) {
                si = (ScreenItem*) (screen_->component(i));
                w = si->window();
                if (w) {
                    if (/*w->is_mapped() &&*/ w != PrintableWindow::leader()) {
                        if (w_ != w) {
                            sivec[nwin++] = si;
                        } else {
                            char buf[100];
                            Sprintf(buf,
                                    "{pwman_place(%d,%d,%d)}\n",
                                    w->xleft(),
                                    w->xtop(),
                                    w->is_mapped() ? 1 : 0);
                            o << buf;
                        }
                    }
                }
            }
        }
    } else {
        if (paper_ && paper_->count()) {
            sivec = new ScreenItem*[paper_->count()];
            for (i = 0; i < paper_->count(); i++) {
                if (paper_->showing(i)) {
                    si = ((PaperItem*) (paper_->component(i)))->screen_item();
                    w = si->window();
                    if (w) {
                        if (w_ != w) {
                            sivec[nwin++] = si;
                        } else {
                            char buf[100];
                            Sprintf(buf, "{pwman_place(%d,%d)}\n", w->xleft(), w->xtop());
                            o << buf;
                        }
                    }
                }
            }
        }
    }
    save_list(nwin, sivec, o);  // sivec deleted here
    obuf.close();
    if (sivec) {
        delete[] sivec;
    }
}

void PWMImpl::save_begin(std::ostream& o) {
    Oc::save_stream = &o;
    Scene::save_all(o);
    HocPanel::save_all(o);
    o << "objectvar ocbox_, ocbox_list_, scene_, scene_list_" << std::endl;
    o << "{ocbox_list_ = new List()  scene_list_ = new List()}" << std::endl;
}

void PWMImpl::save_list(int nwin, ScreenItem** sivec, std::ostream& o) {
    // save highest first, only a few priorities
    OcGlyph* ocg;
    int i, pri, max, working;
    ses_group_first_ = 1;
    // printf("will save %d windows\n", nwin);
    for (working = 10000; working >= 0; working = max) {
        // printf("working = %d\n", working);
        max = -1;
        for (i = 0; i < nwin; ++i) {
            if (sivec[i]->window()) {
                ocg = (OcGlyph*) sivec[i]->window()->glyph();
                pri = ocg->session_priority();
            }
            if (pri == working) {
                // printf("saving item %d with priority %d\n", i, pri);
                if (sivec[i]->window()) {
                    ocg->save(o);
                }
                ses_group(sivec[i], o);
            }
            if (pri < working && pri > max) {
                max = pri;
            }
        }
    }
    Oc::save_stream = NULL;
    o << "objectvar scene_vector_[1]\n{doNotify()}" << std::endl;
}

void PWMImpl::ses_group(ScreenItem* si, std::ostream& o) {
    char buf[512];
    char* name;
    if (si->group_obj_) {
        name = Oc2IV::object_str("name", si->group_obj_);
        Sprintf(buf,
                "{WindowMenu[0].ses_gid(%d, %d, %d, \"%s\")}\n",
                ses_group_first_,
                si->group_obj_->index,
                (screen()->showing(si->index()) ? 1 : 0),
                name);
        o << buf;
        ses_group_first_ = 0;
    }
}

void PWMImpl::retrieve_control() {
    if (Oc::helpmode()) {
        Oc::help(PWM_retrieve_control_);
    }
    if (!fc_retrieve_) {
        Style* style = new Style(Session::instance()->style());
        String str;
        if (style->find_attribute("pwm_save_file_filter", str)) {
            style->attribute("filter", "true");
            style->attribute("filterPattern", str);
        }
        style->attribute("caption", "Retrieve windows from file");
        style->attribute("open", "Retrieve from file");
        fc_retrieve_ = DialogKit::instance()->file_chooser("./", style);
        fc_retrieve_->ref();
    } else {
        fc_retrieve_->reread();
    }
    while (fc_retrieve_->post_for(w_)) {
        if (ok_to_read(*fc_retrieve_->selected(), w_)) {
            Oc oc;
            char buf[256];
            Sprintf(buf, "{load_file(1, \"%s\")}\n", fc_retrieve_->selected()->string());
            if (!oc.run(buf)) {
                break;
            }
        }
    }
}

class OcLabelGlyph: public OcGlyph {
  public:
    OcLabelGlyph(const char*, OcGlyph*, Glyph*);
    virtual ~OcLabelGlyph();
    virtual void save(std::ostream&);

  private:
    CopyString label_;
    OcGlyph* og_;
};

OcLabelGlyph::OcLabelGlyph(const char* label, OcGlyph* og, Glyph* g) {
    label_ = label;
    og_ = og;
    og_->parents(true);
    Resource::ref(og_);
    body(g);
}

OcLabelGlyph::~OcLabelGlyph() {
    og_->parents(false);
    Resource::unref(og_);
}

void OcLabelGlyph::save(std::ostream& o) {
    char buf[256];
    o << "{xpanel(\"\")" << std::endl;
    Sprintf(buf, "xlabel(\"%s\")", label_.string());
    o << buf << std::endl;
    o << "xpanel()}" << std::endl;
    og_->save(o);
}

/*static*/ class TrayDismiss: public WinDismiss {
  public:
    TrayDismiss(DismissableWindow*);
    virtual ~TrayDismiss();
    virtual void execute();
};

class OcTray: public OcBox {
  public:
    OcTray(GlyphIndex cnt);
    virtual ~OcTray();
    virtual PrintableWindow* make_window(Coord = -1, Coord = -1, Coord = -1, Coord = -1);
    virtual void start_vbox();
    virtual void win(PrintableWindow*);
    virtual void dissolve(Coord, Coord);

  private:
    PolyGlyph* pg_;
    OcBox* v_;
    float *x_, *y_;
};

TrayDismiss::TrayDismiss(DismissableWindow* w)
    : WinDismiss(w) {}
TrayDismiss::~TrayDismiss() {}
void TrayDismiss::execute() {
    if (boolean_dialog("Dismiss or Dissolve into components?", "Dissolve", "Dismiss", win_)) {
        OcTray* t = (OcTray*) win_->glyph();
        t->dissolve(win_->left(), win_->bottom());
    }
    WinDismiss::execute();
}
OcTray::OcTray(GlyphIndex cnt)
    : OcBox(OcBox::H) {
    x_ = new float[cnt];
    y_ = new float[cnt];
    pg_ = new PolyGlyph();
    pg_->ref();
    v_ = NULL;
}

OcTray::~OcTray() {
    pg_->unref();
    delete[] x_;
    delete[] y_;
}

void OcTray::dissolve(Coord left, Coord bottom) {
    Window* w;
    Coord l, b;
    OcGlyph* g;
    Requisition req;
    GlyphIndex i, cnt = pg_->count();
    for (i = 0; i < cnt; ++i) {
        l = left;
        b = bottom;
        g = (OcGlyph*) pg_->component(i);
        g->request(req);
        w = g->make_window(l + x_[i] - x_[0],
                           b + y_[i] - y_[0],
                           req.x_requirement().natural(),
                           req.y_requirement().natural());
        w->map();
    }
}

void OcTray::start_vbox() {
    v_ = new OcBox(OcBox::V);
    box_append(v_);
}

void OcTray::win(PrintableWindow* w) {
    LayoutKit* lk = LayoutKit::instance();
    WidgetKit* wk = WidgetKit::instance();
    wk->begin_style("_tray_panel");
    GlyphIndex n = pg_->count();
    pg_->append(w->glyph());
    x_[n] = w->left();
    y_[n] = w->bottom();
    v_->box_append(new OcLabelGlyph(w->name(),
                                    (OcGlyph*) w->glyph(),
                                    lk->vbox(wk->label(w->name()),
                                             lk->fixed(w->glyph(), w->width(), w->height()))));
    wk->end_style();
}

PrintableWindow* OcTray::make_window(Coord left, Coord bottom, Coord width, Coord height) {
    PrintableWindow* w = OcGlyph::make_window(left, bottom, width, height);
    w->replace_dismiss_action(new TrayDismiss(w));
    w->type("Tray");
    w->name("Tray");
    return w;
}

void PWMImpl::tray() {
    if (Oc::helpmode()) {
        Oc::help(PWM_tray_);
        return;
    }
    GlyphIndex count;

    count = paper_->count();
    // don't make a tray containing the main manager

    // build hbox(vbox) of the individual panels
    long index;
    Coord minleft = -1000;
    Coord top = -1000.;
    OcTray* tray = new OcTray(count);
    while ((index = upper_left()) != -1) {
        PaperItem* pi = (PaperItem*) paper_->component(index);
        PrintableWindow* w = pi->screen_item()->window();
        Coord l, b;
        l = w->left();
        b = w->bottom();
        if (minleft < l) {
            tray->start_vbox();
            minleft = l + w->width() / 2.;
        }
        if (top < 0) {
            top = b + w->height();
        }
        tray->win(w);
        paper_->show(index, false);
        w->dismiss();
    }
    Window* w = tray->make_window();
    w->map();
}

GlyphIndex PWMImpl::upper_left() {
    GlyphIndex index = -1;
    Coord minleft = 1e10;
    Coord maxbottom = -1e10;
    GlyphIndex count = paper_->count();
    for (GlyphIndex i = 0; i < count; ++i) {
        PaperItem* pi = (PaperItem*) paper_->component(i);
        PrintableWindow* w = pi->screen_item()->window();
        Coord l, b;
        if (!paper_->showing(i))
            continue;
        if (w == pwm_impl->w_) {
            continue;
        }
        //		paper_->location(i, l, b);
        //		l *= Scl;
        //		b *= Scl;
        l = w->left();
        b = w->bottom();
        if (l < minleft - 50.) {
            index = i;
            minleft = l;
            maxbottom = b;
        } else if (l < minleft + 50.) {
            if (maxbottom < b) {
                index = i;
                minleft = l;
                maxbottom = b;
            }
        }
    }
    return index;
}

#if 0
void PWMImpl::dissolve() {
	for (long i = 0; i < panelList_.count(); i++) {
		new HocWin(panelList_.item(i));
	}
	dismiss();
}
#endif

#if SNAPSHOT
bool ivoc_snapshot(const Event* e) {
    char buf[4];
    e->mapkey(buf, 1);
    if (buf[0] == 'p') {
        ivoc_snapshot_ = NULL;
        PWMImpl* p = PrintableWindowManager::current()->pwmi_;
        p->snapshot(e);
        return true;
    }
    return false;
}

#if 1
#include <OS/table.h>
#include <InterViews/enter-scope.h>
#include <IV-X11/Xlib.h>
#include <IV-X11/xdisplay.h>

declareTable(WindowTable, XWindow, Window*)


Window* PWMImpl::snap_owned(Printer* pr, Window* wp) {
    WindowTable* wt = Session::instance()->default_display()->rep()->wtable_;
    for (TableIterator(WindowTable) i(*wt); i.more(); i.next()) {
        Window* w = i.cur_value();
        if (w->is_mapped()) {
            snap(pr, w);
        }
    }
    return NULL;
}
#endif
#endif  // SNAPSHOT

#else

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "oc2iv.h"

#endif  // HAVE_IV

char* ivoc_get_temp_file() {
    char* tmpfile;
    const char* tdir = getenv("TEMP");
    if (!tdir) {
        tdir = "/tmp";
    }
    auto const length = strlen(tdir) + 1 + 9 + 1;
    tmpfile = new char[length];
    std::snprintf(tmpfile, length, "%s/nrnXXXXXX", tdir);
#if HAVE_MKSTEMP
    int fd;
    if ((fd = mkstemp(tmpfile)) == -1) {
        hoc_execerror("Could not create temporary file:", tmpfile);
    }
    close(fd);
#else
    mktemp(tmpfile);
#endif
#if defined(WIN32)
    tmpfile = hoc_back2forward(tmpfile);
#endif
    return tmpfile;
}
