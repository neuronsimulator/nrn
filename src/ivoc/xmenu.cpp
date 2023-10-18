#include <../../nrnconf.h>

#include "oc2iv.h"
#include "classreg.h"
#include "gui-redirect.h"
#include "utils/enumerate.h"

#if HAVE_IV  // to end of file except for a few small fragments.

#include <cstdio>
#include <cstring>
#include <cmath>
#include <cctype>
#include <cerrno>


#include <InterViews/box.h>
#include <IV-look/kit.h>
#include <InterViews/event.h>
#include <InterViews/layout.h>
#include <InterViews/style.h>
#include <InterViews/background.h>
#include <InterViews/border.h>
#include <InterViews/dialog.h>
#include <InterViews/printer.h>
#include <InterViews/geometry.h>
#include <InterViews/transformer.h>
#include <InterViews/patch.h>
#include <InterViews/color.h>
#include <InterViews/telltale.h>
#include <InterViews/hit.h>
#include <InterViews/resource.h>

#include <InterViews/display.h>
#include "mymath.h"
#include "xmenu.h"
#include "datapath.h"
#include "ivoc.h"
#include "bndedval.h"
#include "nrnpy.h"
#include "objcmd.h"
#include "parse.hpp"
#include "utility.h"
#include "scenepic.h"

// The problem this overcomes is that the pick of an input handler normally
// succeeds for a keystroke only if the mouse is over one of the child
// handlers. We want it to succeed whenever it is inside the panel.
class PanelInputHandler: public InputHandler {
  public:
    PanelInputHandler(Glyph*, Style*);
    virtual ~PanelInputHandler();
    virtual void pick(Canvas*, const Allocation&, int depth, Hit&);
    virtual void focus(InputHandler*);
    static bool has_old_focus() {
        bool old = sema_;
        sema_ = false;
        return old;
    }
    static void handle_old_focus();

  private:
    static InputHandler* focus_;
    static bool sema_;
};

void handle_old_focus() {
    PanelInputHandler::handle_old_focus();
}

InputHandler* PanelInputHandler::focus_ = NULL;
bool PanelInputHandler::sema_ = false;

PanelInputHandler::PanelInputHandler(Glyph* g, Style* s)
    : InputHandler(g, s) {}
PanelInputHandler::~PanelInputHandler() {}
void PanelInputHandler::pick(Canvas* c, const Allocation& a, int depth, Hit& h) {
    const Event* e = h.event();
    if (focus_ && e && e->type() == Event::key && focus_->handler()) {
        h.target(depth, this, 0, focus_->handler());
    } else {
        InputHandler::pick(c, a, depth, h);
    }
}
void PanelInputHandler::focus(InputHandler* h) {
    if (focus_ && focus_ != h) {
        if (h) {
            sema_ = true;
        }
        InputHandler* f = focus_;
        focus_ = NULL;
        f->focus_out();
    }
    focus_ = h;
    InputHandler::focus(h);
}

void PanelInputHandler::handle_old_focus() {
    if (focus_) {
        // printf("handle_old_focus %p\n", focus_);
        sema_ = true;
        InputHandler* f = focus_;
        focus_ = NULL;
        f->focus_out();
    }
}

class ValEdLabel: public MonoGlyph {
  public:
    ValEdLabel(Glyph*);
    virtual ~ValEdLabel();
    virtual void draw(Canvas*, const Allocation&) const;
    void state(bool);
    void tts(TelltaleState*);

  private:
    static const Color* color_;
    bool state_;
    TelltaleState* tts_;
};

const Color* ValEdLabel::color_;

ValEdLabel::ValEdLabel(Glyph* g)
    : MonoGlyph(g) {
    state_ = false;
    if (!color_) {
        color_ = Color::lookup(Session::instance()->default_display(), "yellow");
        Resource::ref(color_);
    }
    tts_ = NULL;
}

ValEdLabel::~ValEdLabel() {}
void ValEdLabel::draw(Canvas* c, const Allocation& a) const {
    if (state_) {
        c->fill_rect(a.left(), a.bottom(), a.right(), a.top(), color_);
    }
    MonoGlyph::draw(c, a);
}
void ValEdLabel::state(bool s) {
    if (state_ != s) {
        state_ = s;
        tts_->notify();
    }
}

void ValEdLabel::tts(TelltaleState* t) {
    tts_ = t;  // not reffed
}

static void hoc_ivpanelPlace(Coord, Coord, int scroll = -1);

static String* xvalue_format;

#define Editor_Default "DefaultValueEditor"
#define Editor_Stepper "DefaultValueEditor"


extern int units_on_flag_;
extern Symbol* hoc_get_last_pointer_symbol();
void hoc_notify_value() {
    Oc oc;
    oc.notify();
}

void hoc_xpanel() {
    TRY_GUI_REDIRECT_DOUBLE("xpanel", NULL);
    IFGUI
    if (ifarg(1) && hoc_is_str_arg(1)) {  // begin spec
        bool h = false;
        if (ifarg(2)) {
            h = (int) chkarg(2, 0, 1) ? true : false;
        }
        hoc_ivpanel(gargstr(1), h);
    } else {              // map
        int scroll = -1;  // leave up to panel_scroll attribute
        if (ifarg(2)) {
            if (ifarg(3)) {
                scroll = (int) chkarg(3, -1, 1);
            }
            hoc_ivpanelPlace((Coord) *getarg(1), (Coord) *getarg(2), scroll);
        } else {
            if (ifarg(1)) {
                scroll = (int) chkarg(1, -1, 1);
            }
            hoc_ivpanelmap(scroll);
        }
    }
    ENDGUI

    hoc_ret();
    hoc_pushx(0.);
}

void hoc_xmenu() {
    TRY_GUI_REDIRECT_DOUBLE("xmenu", NULL);
    IFGUI
    bool add2menubar = false;
    char* mk = NULL;
    Object* pyact = NULL;
    int i = 2;
    if (ifarg(i)) {
        if (hoc_is_str_arg(i)) {
            mk = gargstr(i);
            ++i;
        } else if (hoc_is_object_arg(i)) {
            pyact = *hoc_objgetarg(i);
            ++i;
        }
        if (ifarg(i)) {
            add2menubar = int(chkarg(i, 0, 1));
        }
    }
    if (ifarg(1)) {
        if (mk || pyact) {
            hoc_ivvarmenu(gargstr(1), mk, add2menubar, pyact);
        } else {
            hoc_ivmenu(gargstr(1), add2menubar);
        }
    } else {
        hoc_ivmenu((char*) 0);
    }
    ENDGUI
    hoc_ret();
    hoc_pushx(0.);
}

void hoc_xbutton() {
    TRY_GUI_REDIRECT_DOUBLE("xbutton", NULL);

    IFGUI
    char* s1;
    s1 = gargstr(1);
    if (ifarg(2)) {
        if (hoc_is_object_arg(2)) {
            hoc_ivbutton(s1, NULL, *hoc_objgetarg(2));
        } else {
            hoc_ivbutton(s1, gargstr(2));
        }
    } else {
        hoc_ivbutton(s1, s1);
    }
    ENDGUI
    hoc_ret();
    hoc_pushx(0.);
}

/*
xstatebutton("prompt",&var [,"action"])
   like xbutton, but var is set to 0 or 1 depending to match the
   telltale state of the button
*/

void hoc_xstatebutton() {
    TRY_GUI_REDIRECT_DOUBLE("xstatebutton", NULL);
    IFGUI
    char *s1, *s2 = (char*) 0;

    s1 = gargstr(1);

    if (hoc_is_object_arg(2)) {
        neuron::container::data_handle<double> ptr1{};
        hoc_ivstatebutton(ptr1,
                          s1,
                          NULL,
                          HocStateButton::PALETTE,
                          *hoc_objgetarg(2),
                          ifarg(3) ? *hoc_objgetarg(3) : NULL);
    } else {
        if (ifarg(3)) {
            s2 = gargstr(3);
        }
        hoc_ivstatebutton(hoc_hgetarg<double>(2), s1, s2, HocStateButton::PALETTE);
    }
    ENDGUI
    hoc_ret();
    hoc_pushx(0.);
}


/*
xcheckbox("prompt",&var [,"action"])
   like xbutton, but var is set to 0 or 1 depending to match the
   telltale state of the button
*/

void hoc_xcheckbox() {
    TRY_GUI_REDIRECT_DOUBLE("xcheckbox", NULL);
    IFGUI

    char *s1, *s2 = (char*) 0;

    s1 = gargstr(1);

    if (hoc_is_object_arg(2)) {
        neuron::container::data_handle<double> ptr1{};
        hoc_ivstatebutton(ptr1,
                          s1,
                          NULL,
                          HocStateButton::CHECKBOX,
                          *hoc_objgetarg(2),
                          ifarg(3) ? *hoc_objgetarg(3) : 0);
    } else {
        if (ifarg(3)) {
            s2 = gargstr(3);
        }
        hoc_ivstatebutton(hoc_hgetarg<double>(2), s1, s2, HocStateButton::CHECKBOX);
    }
    ENDGUI
    hoc_ret();
    hoc_pushx(0.);
}

void hoc_xradiobutton() {
    TRY_GUI_REDIRECT_DOUBLE("xradiobutton", NULL);

    IFGUI
    char *s1, *s2 = (char*) 0;
    Object* po = NULL;
    bool activate = false;
    s1 = gargstr(1);
    if (ifarg(2)) {
        if (hoc_is_object_arg(2)) {
            po = *hoc_objgetarg(2);
        } else {
            s2 = gargstr(2);
        }
        if (ifarg(3)) {
            activate = (chkarg(3, 0, 1) != 0.);
        }
    } else {
        s2 = s1;
    }
    if (po) {
        hoc_ivradiobutton(s1, NULL, activate, po);
    } else {
        hoc_ivradiobutton(s1, s2, activate);
    }
    ENDGUI
    hoc_ret();
    hoc_pushx(0.);
}

static void hoc_xvalue_helper() {
    IFGUI  // prompt, variable, deflt,action,canrun,usepointer
        char *s1,
        *s2, *s3;
    // allow variable arg2 to be data_handle
    neuron::container::data_handle<double> ptr2{};
    Object* pyvar = NULL;
    Object* pyact = NULL;
    s2 = s3 = NULL;
    s1 = gargstr(1);
    if (ifarg(2)) {
        if (hoc_is_object_arg(2)) {
            pyvar = *hoc_objgetarg(2);
        } else if (hoc_is_pdouble_arg(2)) {
            ptr2 = hoc_hgetarg<double>(2);
        } else {
            s2 = gargstr(2);
        }
    } else {
        s2 = s1;
    }
    bool deflt = false;
    if (ifarg(3) && *getarg(3)) {
        if (*getarg(3) == 2.) {
            if (pyvar) {
                hoc_ivvalue_keep_updated(s1, NULL, pyvar);
            } else {
                hoc_ivvalue_keep_updated(s1, s2);
            }
            return;
        }
        deflt = true;
    }
    bool canRun = false, usepointer = false;
    if (ifarg(4)) {
        if (hoc_is_object_arg(4)) {
            pyact = *hoc_objgetarg(4);
        } else {
            s3 = gargstr(4);
        }
        if (ifarg(5) && *getarg(5)) {
            canRun = true;
        }
        if (ifarg(6) && *getarg(6)) {
            usepointer = true;
        }
    }
    hoc_ivvaluerun_ex(s1, s2, ptr2, pyvar, s3, pyact, deflt, canRun, usepointer);

    ENDGUI
}

void hoc_xfixedvalue() {
    TRY_GUI_REDIRECT_DOUBLE("xfixedvalue", NULL);

    IFGUI  // prompt, variable, deflt,action,canrun,usepointer
        char *s1,
        *s2;
    s1 = gargstr(1);
    if (ifarg(2)) {
        s2 = gargstr(2);
    } else {
        s2 = s1;
    }
    bool deflt = false;
    if (ifarg(3) && *getarg(3)) {
        deflt = true;
    }
    bool usepointer = false;
    if (ifarg(4) && *getarg(4)) {
        usepointer = true;
    }
    hoc_ivfixedvalue(s1, s2, deflt, usepointer);

    ENDGUI
    hoc_ret();
    hoc_pushx(0.);
}

static void hoc_xpvalue_helper() {
    IFGUI  // prompt,variable,deflt,action,canrun
        char *s1,
        *s3;
    neuron::container::data_handle<double> pd{};
    HocSymExtension* extra = NULL;
    Symbol* sym;
    s1 = gargstr(1);
    if (ifarg(2)) {
        pd = hoc_hgetarg<double>(2);
        sym = hoc_get_last_pointer_symbol();
    } else {
        pd = hoc_val_handle(s1);
        sym = hoc_get_symbol(s1);
    }
    if (sym) {
        extra = sym->extra;
    }
    bool deflt = false;
    if (ifarg(3) && *getarg(3)) {
        deflt = true;
    }
    if (ifarg(4)) {
        s3 = gargstr(4);
        bool canRun = false;
        if (ifarg(5) && *getarg(5)) {
            canRun = true;
        }
        hoc_ivpvaluerun(s1, pd, s3, deflt, canRun, extra);
    } else {
        hoc_ivpvalue(s1, pd, deflt, extra);
    }
    ENDGUI
}

void hoc_xvalue() {
    TRY_GUI_REDIRECT_DOUBLE("xvalue", NULL);

    hoc_xvalue_helper();
    hoc_ret();
    hoc_pushx(0.);
}

void hoc_xpvalue() {
    TRY_GUI_REDIRECT_DOUBLE("xpvalue", NULL);
    hoc_xpvalue_helper();
    hoc_ret();
    hoc_pushx(0.);
}

void hoc_xlabel() {
    TRY_GUI_REDIRECT_DOUBLE("xlabel", NULL);
    IFGUI
    char* s1;
    s1 = gargstr(1);
    hoc_ivlabel(s1);
    ENDGUI
    hoc_ret();
    hoc_pushx(0.);
}

void hoc_xvarlabel() {
    TRY_GUI_REDIRECT_DOUBLE_SEND_STRREF("xvarlabel", NULL);
    IFGUI
    if (hoc_is_object_arg(1)) {
        hoc_ivvarlabel(NULL, *hoc_objgetarg(1));
    } else {
        hoc_ivvarlabel(hoc_pgargstr(1));
    }
    ENDGUI
    hoc_ret();
    hoc_pushx(0.);
}

// ZFM modified to add vertical vs. horizontal
void hoc_xslider() {
    TRY_GUI_REDIRECT_DOUBLE("xslider", NULL);
    IFGUI
    float low = 0, high = 100;
    float resolution = 1;
    int nsteps = 10;
    char* send = NULL;
    Object* pysend = NULL;
    neuron::container::data_handle<double> pval{};
    Object* pyvar = NULL;
    bool vert = 0;
    if (ifarg(3)) {
        low = *getarg(2);
        high = *getarg(3);
        resolution = (high - low) / 100.;
    }
    int iarg = 4;
    if (ifarg(iarg)) {
        if (hoc_is_str_arg(iarg)) {
            send = gargstr(4);
            ++iarg;
        } else if (hoc_is_object_arg(iarg)) {
            pysend = *hoc_objgetarg(iarg);
            ++iarg;
        }
    }
    if (ifarg(iarg)) {
        vert = int(chkarg(iarg, 0, 1));
    }
    bool slow = false;
    if (ifarg(++iarg)) {
        slow = int(chkarg(iarg, 0, 1));
    }
    if (hoc_is_object_arg(1)) {
        pyvar = *hoc_objgetarg(1);
    } else {
        pval = hoc_hgetarg<double>(1);
    }
    hoc_ivslider(pval, low, high, resolution, nsteps, send, vert, slow, pyvar, pysend);
    ENDGUI
    hoc_ret();
    hoc_pushx(0.);
}


class HocButton: public Button {
  public:
    HocButton(const char*, Glyph*, Style*, TelltaleState*, Action*);
    virtual ~HocButton();
    static HocButton* instance(const char*, Action*);
    virtual void print(Printer*, const Allocation&) const;

  private:
    Glyph* l_;
};
HocButton::HocButton(const char* text, Glyph* g, Style* s, TelltaleState* t, Action* a)
    : Button(g, s, t, a) {
    l_ = WidgetKit::instance()->label(text);
    l_->ref();
}
HocButton::~HocButton() {
    Resource::unref(l_);
}
HocButton* HocButton::instance(const char* s, Action* a) {
    Button* b = WidgetKit::instance()->push_button(s, a);
    b->ref();
    HocButton* hb = new HocButton(s, b->body(), b->style(), b->state(), b->action());
    b->unref();
    return hb;
}
void HocButton::print(Printer* pr, const Allocation& a) const {
    l_->print(pr, a);
}

static std::vector<HocPanel*>* hoc_panel_list;
static HocPanel* curHocPanel;
static HocValEditor* last_fe_constructed_;
static void checkOpenPanel() {
    if (!curHocPanel) {
        hoc_execerror("No panel is open", NULL);
    }
}

/*static*/ class MenuStack {
  public:
    bool isEmpty() {
        return l_.empty();
    }
    void push(HocMenu* m);
    void pop() {
        if (!l_.empty()) {
            l_.front()->unref();
            l_.erase(l_.begin());
        }
    }
    Menu* top() {
        return (l_.empty()) ? nullptr : l_.front()->menu();
    }
    HocItem* hoc_item() {
        return (l_.empty()) ? nullptr : l_.front();
    }
    void clean();

  private:
    std::vector<HocMenu*> l_;
};
void MenuStack::push(HocMenu* m) {
    m->ref();
    l_.insert(l_.begin(), m);
}
void MenuStack::clean() {
    for (auto& item: l_) {
        item->unref();
    }
    l_.clear();
}
static MenuStack* menuStack;
static Menu* hocmenubar;

class OcTelltaleGroup: public TelltaleGroup {
  public:
    OcTelltaleGroup();
    virtual ~OcTelltaleGroup();
    virtual void update(TelltaleState*);
    virtual void remove(TelltaleState*);
    virtual void restore();

  private:
    TelltaleState* previous_;
    TelltaleState* current_;
};

OcTelltaleGroup::OcTelltaleGroup() {
    previous_ = NULL;
    current_ = NULL;
}
OcTelltaleGroup::~OcTelltaleGroup() {}
void OcTelltaleGroup::update(TelltaleState* t) {
    if (t != current_ && t->test(TelltaleState::is_chosen)) {
        previous_ = current_;
        current_ = t;
    }
    TelltaleGroup::update(t);
}
void OcTelltaleGroup::remove(TelltaleState* t) {
    if (previous_ == t) {
        previous_ = NULL;
    }
    if (current_ == t) {
        current_ = NULL;
    }
    TelltaleGroup::remove(t);
}
void OcTelltaleGroup::restore() {
    if (previous_) {
        previous_->set(TelltaleState::is_chosen, true);
    } else if (current_) {
        TelltaleGroup::update(current_);
        current_->set(TelltaleState::is_chosen, false);
        current_ = NULL;
    }
}

class HocRadioAction: public HocAction {
  public:
    HocRadioAction(const char* action, OcTelltaleGroup*, Object* pyact = NULL);
    virtual ~HocRadioAction();
    virtual void help();

  private:
    OcTelltaleGroup* tg_;
};

HocRadioAction::HocRadioAction(const char* action, OcTelltaleGroup* tg, Object* pyact)
    : HocAction(action, pyact) {
    tg_ = tg;
    Resource::ref(tg_);
}

HocRadioAction::~HocRadioAction() {
    Resource::unref(tg_);
}
void HocRadioAction::help() {
    tg_->restore();
    HocAction::help();
}

/*static*/ class HocRadio {
  public:
    HocRadio();
    virtual ~HocRadio();

    OcTelltaleGroup* group() {
        return g_;
    }
    void start();
    void stop();

  private:
    OcTelltaleGroup* g_;
};

HocRadio::HocRadio() {
    g_ = NULL;
}
HocRadio::~HocRadio() {
    Resource::unref(g_);
}
void HocRadio::start() {
    Resource::unref(g_);
    g_ = new OcTelltaleGroup();
    g_->ref();
}

void HocRadio::stop() {
    Resource::unref(g_);
    g_ = NULL;
}

static HocRadio* hoc_radio;

void hoc_ivpanel(const char* name, bool h) {
    if (!hoc_radio) {
        hoc_radio = new HocRadio();
    }
    if (curHocPanel) {
        fprintf(stderr, "%s not closed\n", curHocPanel->getName());
        if (menuStack) {
            menuStack->clean();
        }
        curHocPanel->unref();
        curHocPanel = NULL;
        hoc_execerror("Didn't close the previous panel", NULL);
    } else {
        curHocPanel = new HocPanel(name, h);
        curHocPanel->ref();
    }
    hoc_radio->stop();
}

void hoc_ivpanelmap(int scroll) {
    checkOpenPanel();
    curHocPanel->map_window(scroll);
    curHocPanel->unref();
    curHocPanel = NULL;
    if (menuStack && !menuStack->isEmpty()) {
        fprintf(stderr, "%s menu not closed\n", menuStack->hoc_item()->getStr());
        menuStack->clean();
        hoc_execerror("A menu is still open", 0);
    }
    hoc_radio->stop();
}

void hoc_ivpanelPlace(Coord left, Coord bottom, int scroll) {
    checkOpenPanel();
    curHocPanel->left_ = left;
    curHocPanel->bottom_ = bottom;
    hoc_ivpanelmap(scroll);
}

void hoc_ivbutton(const char* name, const char* action, Object* pyact) {
    checkOpenPanel();
    hoc_radio->stop();
    if (menuStack && !menuStack->isEmpty()) {
        menuStack->top()->append_item(curHocPanel->menuItem(name, action, false, pyact));
    } else {
        curHocPanel->pushButton(name, action, false, pyact);
    }
}

void hoc_ivstatebutton(neuron::container::data_handle<double> pd,
                       const char* name,
                       const char* action,
                       int style,
                       Object* pyvar,
                       Object* pyact) {
    checkOpenPanel();
    hoc_radio->stop();
    if (menuStack && !menuStack->isEmpty()) {
        menuStack->top()->append_item(curHocPanel->menuStateItem(pd, name, action, pyvar, pyact));
    } else {
        curHocPanel->stateButton(pd, name, action, style, pyvar, pyact);
    }
}

void hoc_ivradiobutton(const char* name, const char* action, bool activate, Object* pyact) {
    checkOpenPanel();
    if (!hoc_radio->group()) {
        hoc_radio->start();
    }
    if (menuStack && !menuStack->isEmpty()) {
        menuStack->top()->append_item(curHocPanel->menuItem(name, action, activate, pyact));
    } else {
        curHocPanel->pushButton(name, action, activate, pyact);
    }
}

void hoc_ivmenu(const char* name, bool add2menubar) {
    if (!menuStack) {
        menuStack = new MenuStack();
    }
    checkOpenPanel();
    hoc_radio->stop();
    if (name) {
        HocMenu* m = curHocPanel->menu(name, add2menubar);
        menuStack->push(m);
    } else {
        curHocPanel->itemAppend("xmenu()");
        menuStack->pop();
    }
}

void hoc_ivvarmenu(const char* name, const char* action, bool add2menubar, Object* pyvar) {
    if (!menuStack) {
        menuStack = new MenuStack();
    }
    checkOpenPanel();
    hoc_radio->stop();
    HocMenu* m = curHocPanel->menu(name, add2menubar);
    HocMenuAction* hma = new HocMenuAction(action, pyvar, m);
    m->item()->action(hma);
}

void hoc_ivvalue_keep_updated(const char* name, const char* variable, Object* pyvar) {
    checkOpenPanel();
    hoc_radio->stop();
    Symbol* s = hoc_get_symbol(variable);
    curHocPanel->valueEd(name,
                         variable,
                         NULL,
                         false,
                         hoc_val_handle(variable),
                         false,
                         true,
                         (s ? s->extra : NULL),
                         pyvar);
}

void hoc_ivvalue(const char* name, const char* variable, bool deflt, Object* pyvar) {
    hoc_ivvaluerun(name, variable, NULL, deflt, false, false);
}

void hoc_ivfixedvalue(const char* name, const char* variable, bool deflt, bool usepointer) {
    hoc_ivvaluerun(name, variable, NULL, deflt, false, usepointer);
}

void hoc_ivpvalue(const char* name,
                  neuron::container::data_handle<double> pd,
                  bool deflt,
                  HocSymExtension* extra) {
    hoc_ivpvaluerun(name, pd, 0, deflt, false, extra);
}

void hoc_ivvaluerun(const char* name,
                    const char* variable,
                    const char* action,
                    bool deflt,
                    bool canRun,
                    bool usepointer,
                    Object* pyvar,
                    Object* pyact) {
    hoc_ivvaluerun_ex(name, variable, {}, pyvar, action, pyact, deflt, canRun, usepointer);
}

void hoc_ivvaluerun_ex(CChar* name,
                       CChar* variable,
                       neuron::container::data_handle<double> pvar,
                       Object* pyvar,
                       CChar* action,
                       Object* pyact,
                       bool deflt,
                       bool canrun,
                       bool usepointer,
                       HocSymExtension* extra) {
    checkOpenPanel();
    hoc_radio->stop();
    Symbol* s = NULL;
    if (!pvar && !pyvar) {
        s = hoc_get_symbol(variable);
        if (usepointer) {
            pvar = hoc_val_handle(variable);
        }
    }
    HocSymExtension* xtra = extra;
    if (!xtra) {
        xtra = s ? s->extra : NULL;
    }
    curHocPanel->valueEd(name, variable, action, canrun, pvar, deflt, false, xtra, pyvar, pyact);
}

void hoc_ivpvaluerun(const char* name,
                     neuron::container::data_handle<double> pd,
                     const char* action,
                     bool deflt,
                     bool canRun,
                     HocSymExtension* extra) {
    checkOpenPanel();
    hoc_radio->stop();
    curHocPanel->valueEd(name, 0, action, canRun, pd, deflt, false, extra);
}

void hoc_ivlabel(const char* s) {
    checkOpenPanel();
    hoc_radio->stop();
    curHocPanel->label(s);
}

void hoc_ivvarlabel(char** s, Object* pyvar) {
    checkOpenPanel();
    hoc_radio->stop();
    curHocPanel->var_label(s, pyvar);
}

// ZFM added vert
void hoc_ivslider(neuron::container::data_handle<double> pd,
                  float low,
                  float high,
                  float resolution,
                  int nsteps,
                  const char* s,
                  bool vert,
                  bool slow,
                  Object* pyvar,
                  Object* pyact) {
    checkOpenPanel();
    curHocPanel->slider(pd, low, high, resolution, nsteps, s, vert, slow, pyvar, pyact);
}

static char* hideQuote(const char* s) {
    static char buf[200];
    const char* cp1;
    char* cp2;

    cp2 = buf;
    if (s)
        for (cp1 = s; *cp1; cp1++, cp2++) {
            if (*cp1 == '"') {
                *cp2++ = '\\';
            }
            *cp2 = *cp1;
        }
    *cp2 = '\0';
    return buf;
}

static void saveMenuFile() {}

void HocPanel::save_all(std::ostream&) {
    if (!hoc_panel_list)
        return;

    long i, cnt;

    HocDataPaths* data_paths = new HocDataPaths();
    if (hoc_panel_list) {
        for (auto& item: *hoc_panel_list) {
            item->data_path(data_paths, true);
        }
    }
    data_paths->search();
    if (hoc_panel_list) {
        for (auto& item: *hoc_panel_list) {
            item->data_path(data_paths, false);
        }
    }
    delete data_paths;
}

void HocPanel::map_window(int scroll) {
    // switch to scrollbox if too many items
    static GlyphIndex maxcnt = -1;
    if (1 || maxcnt == -1) {
        maxcnt = 12;
        Style* s = WidgetKit::instance()->style();
        s->find_attribute("panel_scroll", maxcnt);
    }
    if ((scroll == -1 && box_->count() > maxcnt) || scroll == 1) {
        LayoutKit& lk = *LayoutKit::instance();
        WidgetKit& wk = *WidgetKit::instance();
        ScrollBox* vsb = lk.vscrollbox(box_->count());
        while (box_->count()) {
            vsb->append(box_->component(0));
            box_->remove(0);
        }
        box_->append(lk.hbox(vsb, lk.hspace(4), wk.vscroll_bar(vsb)));
    }

    PrintableWindow* w = OcGlyph::make_window(left_, bottom_);
    w->style(new Style(WidgetKit::instance()->style()));
    w->style()->attribute("name", getName());
    w->map();
}

// HocPanel
static void var_freed(void* pd, int size) {
    if (hoc_panel_list) {
        for (auto&& elem: reverse(*hoc_panel_list)) {
            elem->check_valid_pointers(pd, size);
        }
    }
}

HocPanel::HocPanel(const char* name, bool h)
    : OcGlyph(NULL) {
    LayoutKit& lk = *LayoutKit::instance();
    WidgetKit& wk = *WidgetKit::instance();
    horizontal_ = h;
    hocmenubar = NULL;
    if (h) {
        box_ = lk.hbox();
    } else {
        box_ = lk.vbox();
    }
    box_->ref();
    body(ih_ = new PanelInputHandler(
             new Background(new Border(lk.margin(lk.hflexible(box_, fil, 0), 3), wk.foreground()),
                            wk.background()),
             wk.style()));
    if (!hoc_panel_list) {
        hoc_panel_list = new std::vector<HocPanel*>;
        Oc oc;
        oc.notify_freed(var_freed);
    }
    hoc_panel_list->push_back(this);
    item_append(new HocItem(name));
    left_ = -1000.;
    bottom_ = -1000.;
    errno = 0;
}

HocPanel::~HocPanel() {
    box_->unref();
    for (auto& item: ilist_) {
        item->HocItem::unref();
    }
    for (auto& item: elist_) {
        item->HocItem::unref();
    }
    erase_first(*hoc_panel_list, this);
    ilist_.clear();
    ilist_.shrink_to_fit();
    elist_.clear();
    elist_.shrink_to_fit();
    //	printf("~HocPanel\n");
}

// HocUpdateItem
HocUpdateItem::HocUpdateItem(const char* name, HocItem* hi)
    : HocItem(name, hi) {}
HocUpdateItem::~HocUpdateItem() {
    HocPanel::keep_updated(this, false);
}
void HocUpdateItem::update_hoc_item() {}
void HocUpdateItem::check_pointer(void*, int) {}
void HocUpdateItem::data_path(HocDataPaths*, bool) {}

// ones that get updated on every doEvents()
std::vector<HocUpdateItem*>* HocPanel::update_list_;

void HocPanel::keep_updated() {
    static int cnt = 0;
    if (update_list_ && (++cnt % 10 == 0)) {
        for (auto& item: *update_list_) {
            item->update_hoc_item();
        }
    }
}
void HocPanel::keep_updated(HocUpdateItem* hui, bool add) {
    if (!update_list_) {
        update_list_ = new std::vector<HocUpdateItem*>();
    }
    if (add) {
        update_list_->push_back(hui);
    } else {
        erase_first(*update_list_, hui);
    }
}

void HocPanel::paneltool(const char* name,
                         const char* proc,
                         const char* selact,
                         ScenePicker* sp,
                         Object* pycallback,
                         Object* pyselact) {
    HocCommand* hc = pycallback ? new HocCommand(pycallback) : new HocCommand(proc);
    HocCommandTool* hct = new HocCommandTool(hc);
    HocAction* ha = NULL;
    if (selact || pyselact) {
        ha = new HocAction(selact, pyselact);
    }
    if (curHocPanel && (!menuStack || menuStack->isEmpty())) {
        Button* b = sp->radio_button(name, hct, ha);
        curHocPanel->box()->append(b);
    } else {
        sp->add_radio_menu(gargstr(1), hct, ha);
    }
}

void HocPanel::itemAppend(const char* str) {
    item_append(new HocItem(str));
}

PolyGlyph* HocPanel::box() {
    //	return (PolyGlyph*)(((MonoGlyph*)body())->body());
    return box_;
}

const char* HocPanel::getName() {
    return ilist_.front()->getStr();
}

HocItem* HocPanel::hoc_item() {
    return ilist_.front();
}

void HocPanel::pushButton(const char* name, const char* action, bool activate, Object* pyact) {
    if (hoc_radio->group()) {
        HocRadioAction* a = new HocRadioAction(action, hoc_radio->group(), pyact);
        Button* b = WidgetKit::instance()->radio_button(hoc_radio->group(), name, a);
        box()->append(b);
        item_append(new HocRadioButton(name, a, hoc_item()));
        if (activate) {
            TelltaleState* tts = b->state();
            tts->set(TelltaleState::is_chosen, true);
            hoc_radio->group()->update(tts);
        }
    } else {
        HocAction* a = new HocAction(action, pyact);
        box()->append(WidgetKit::instance()->push_button(name, a));
        item_append(new HocPushButton(name, a, hoc_item()));
    }
}

HocPushButton::HocPushButton(const char* name, HocAction* a, HocItem* hi)
    : HocItem(name, hi) {
    a_ = a;
    Resource::ref(a);
    a->hoc_item(this);
}
HocPushButton::~HocPushButton() {
    Resource::unref(a_);
}
void HocPushButton::write(std::ostream& o) {
    char buf[200];
    nrn_assert(snprintf(buf, 200, "xbutton(\"%s\",\"%s\")", getStr(), hideQuote(a_->name())) < 200);
    o << buf << std::endl;
}

HocRadioButton::HocRadioButton(const char* name, HocRadioAction* a, HocItem* hi)
    : HocItem(name, hi) {
    a_ = a;
    Resource::ref(a);
    a->hoc_item(this);
}
HocRadioButton::~HocRadioButton() {
    Resource::unref(a_);
}
void HocRadioButton::write(std::ostream& o) {
    char buf[200];
    nrn_assert(snprintf(buf, 200, "xradiobutton(\"%s\",\"%s\")", getStr(), hideQuote(a_->name())) <
               200);
    o << buf << std::endl;
}

void HocPanel::label(const char* name) {
    box()->append(LayoutKit::instance()->margin(WidgetKit::instance()->label(name), 3));
    item_append(new HocLabel(name));
}

void HocPanel::var_label(char** name, Object* pyvar) {
    HocVarLabel* l = new HocVarLabel(name, box(), pyvar);
    item_append(l);
    elist_.push_back(l);
    l->ref();
}

// ZFM added vert
void HocPanel::slider(neuron::container::data_handle<double> pd,
                      float low,
                      float high,
                      float resolution,
                      int nsteps,
                      const char* send,
                      bool vert,
                      bool slow,
                      Object* pyvar,
                      Object* pysend) {
    OcSlider* s = new OcSlider(pd, low, high, resolution, nsteps, send, vert, slow, pyvar, pysend);
    LayoutKit* lk = LayoutKit::instance();
    WidgetKit* wk = WidgetKit::instance();
    if (slow) {
        wk->begin_style("SlowSlider");
    }
    if (vert) {
        box()->append(lk->hflexible(WidgetKit::instance()->vscroll_bar(s->adjustable())));
    } else {
        box()->append(lk->hflexible(WidgetKit::instance()->hscroll_bar(s->adjustable())));
    }
    if (slow) {
        wk->end_style();
    }
    item_append(s);
    elist_.push_back(s);
    s->ref();
}

HocMenu* HocPanel::menu(const char* name, bool add2menubar) {
    WidgetKit* wk = WidgetKit::instance();
    Menu* m = wk->pulldown();
    MenuItem* mi;
    HocMenu* hm;
    if (menuStack->isEmpty()) {
        Menu* m0;
        if (!add2menubar) {
            hocmenubar = NULL;
        }
        if (hocmenubar) {
            m0 = hocmenubar;
        } else {
            m0 = wk->menubar();
            hocmenubar = m0;
            LayoutKit* lk = LayoutKit::instance();
            box()->append(lk->hbox(m0, lk->hglue()));
        }
        mi = wk->menubar_item(name);
        m0->append_item(mi);
        hm = new HocMenu(name, m, mi, hoc_item(), add2menubar);
    } else {
        mi = K::menu_item(name);
        menuStack->top()->append_item(mi);
        hm = new HocMenu(name, m, mi, menuStack->hoc_item());
    }
    item_append(hm);
    mi->menu(m);
    return hm;
}

MenuItem* HocPanel::menuItem(const char* name, const char* act, bool activate, Object* pyact) {
    MenuItem* mi;
    if (hoc_radio->group()) {
        HocRadioAction* a = new HocRadioAction(act, hoc_radio->group(), pyact);
        mi = K::radio_menu_item(hoc_radio->group(), name);
        mi->action(a);
        item_append(new HocRadioButton(name, a, menuStack->hoc_item()));
        if (activate) {
            TelltaleState* tts = mi->state();
            tts->set(TelltaleState::is_chosen, true);
            hoc_radio->group()->update(tts);
        }
    } else {
        HocAction* a = new HocAction(act, pyact);
        mi = K::menu_item(name);
        mi->action(a);
        item_append(new HocPushButton(name, a, menuStack->hoc_item()));
    }
    return mi;
}

HocMenu::HocMenu(const char* name, Menu* m, MenuItem* mi, HocItem* hi, bool add2menubar)
    : HocItem(name, hi) {
    menu_ = m;
    mi_ = mi;
    add2menubar_ = add2menubar;
    m->ref();
}
HocMenu::~HocMenu() {
    menu_->unref();
}
void HocMenu::write(std::ostream& o) {
    char buf[200];
    Sprintf(buf, "xmenu(\"%s\", %d)", getStr(), add2menubar_);
    o << buf << std::endl;
}

static Coord xvalue_field_size;

void HocPanel::valueEd(const char* prompt,
                       Object* pyvar,
                       Object* pyact,
                       bool canrun,
                       bool deflt,
                       bool keep_updated) {
    valueEd(prompt, NULL, NULL, canrun, {}, deflt, keep_updated, NULL, pyvar, pyact);
}

void HocPanel::valueEd(const char* name,
                       const char* variable,
                       const char* action,
                       bool canrun,
                       neuron::container::data_handle<double> pd,
                       bool deflt,
                       bool keep_updated,
                       HocSymExtension* extra,
                       Object* pyvar,
                       Object* pyact) {
    HocValAction* act;
    if (pyact || action) {
        act = new HocValAction(action, pyact);
    } else {
        act = new HocValAction("");
    }
    ValEdLabel* vel;
    float* limits = 0;
    if (extra && extra->parmlimits) {
        limits = extra->parmlimits;
    }
    if (extra && extra->units && units_on_flag_) {
        char nu[256];
        Sprintf(nu, "%s (%s)", name, extra->units);
        vel = new ValEdLabel(WidgetKit::instance()->label(nu));
    } else {
        vel = new ValEdLabel(WidgetKit::instance()->label(name));
    }
    Button* prompt;
    if (canrun) {
        prompt = WidgetKit::instance()->default_button(vel, act);
    } else {
        prompt = WidgetKit::instance()->push_button(vel, act);
    }
    vel->tts(prompt->state());
    HocValEditor* fe;
    Button* def;
    if (deflt) {
        HocDefaultValEditor* dve =
            new HocDefaultValEditor(name, variable, vel, act, pd, canrun, hoc_item(), pyvar);
        def = dve->checkbox();
        fe = dve;
    } else if (keep_updated) {
        fe = new HocValEditorKeepUpdated(name, variable, vel, act, pd, hoc_item(), pyvar);
    } else {
        fe = new HocValEditor(name, variable, vel, act, pd, canrun, hoc_item(), pyvar);
    }
    ih_->append_input_handler(fe->field_editor());
    elist_.push_back(fe);
    fe->ref();
    act->setFieldSEditor(fe);  // so button can change the editor
    LayoutKit* lk = LayoutKit::instance();
    // from mike_neubig_ivoc_xmenu
    float fct;
    Style* s = WidgetKit::instance()->style();
    if (!s->find_attribute("stepper_size", fct)) {
        fct = 20;
    }
    if (deflt) {
        box()->append(lk->hbox(lk->vcenter(prompt),
                               lk->vcenter(def),
                               lk->vcenter(lk->h_fixed_span(fe->field_editor(), xvalue_field_size)),
                               lk->vcenter(lk->fixed(fe->stepper(), (int) fct, (int) fct))));
    } else {
        box()->append(
            lk->hbox(prompt,
                     lk->h_fixed_span(fe->field_editor(), xvalue_field_size),
                     (fe->stepper() ? lk->fixed(fe->stepper(), int(fct), int(fct)) : NULL)));
    }
    item_append(fe);
    if (limits) {
        fe->setlimits(limits);
    }
    last_fe_constructed_ = fe;
}

void HocPanel::save(std::ostream& o) {
    o << "{" << std::endl;
    write(o);
    o << "}" << std::endl;
}

void HocPanel::write(std::ostream& o) {
    Oc oc;
    char buf[200];
    //	o << "xpanel(\"" << getName() << "\")" << std::endl;
    Sprintf(buf, "xpanel(\"%s\", %d)", getName(), horizontal_);
    o << buf << std::endl;
    if (ilist_.size() > 1) {
        for (std::size_t i = 1; i < ilist_.size(); ++i) {
            ilist_[i]->write(o);
        }
    }
    if (has_window()) {
        Sprintf(buf, "xpanel(%g,%g)", window()->save_left(), window()->save_bottom());
        o << buf << std::endl;
    } else {
        o << "xpanel()" << std::endl;
    }
}

void HocPanel::item_append(HocItem* hi) {
    hi->ref();
    ilist_.push_back(hi);
}

// HocItem
HocItem::HocItem(const char* str, HocItem* hi)
    : str_(str) {
    help_parent_ = hi;
}
HocItem::~HocItem() {
    //	printf("~HocItem %s\n", str_.string());
}
void HocItem::write(std::ostream& o) {
    o << str_.string() << std::endl;
}

const char* HocItem::getStr() {
    return str_.string();
}

void HocItem::help_parent(HocItem* hi) {
    help_parent_ = hi;  // not reffed
}

void HocItem::help(const char* child) {
    const char* c1;
    char buf[200], *c2 = buf;
    char path[512];
    for (c1 = getStr(); *c1; ++c1) {
        if (isalnum(*c1)) {
            *c2++ = *c1;
        }
    }
    *c2 = '\0';
    if (child) {
        Sprintf(path, "%s %s", child, buf);
    } else {
        strcpy(path, buf);
    }
    if (help_parent_) {
        help_parent_->help(path);
    } else {
        Oc::help(path);
    }
}

// HocLabel
HocLabel::HocLabel(const char* s)
    : HocItem(s) {}
HocLabel::~HocLabel() {}
void HocLabel::write(std::ostream& o) {
    char buf[210];
    Sprintf(buf, "xlabel(\"%s\")", hideQuote(getStr()));
    o << buf << std::endl;
}

#if 0
extern void purify_watch_rw_4(char**);
#endif

// HocVarLabel
HocVarLabel::HocVarLabel(char** cpp, PolyGlyph* pg, Object* pyvar)
    : HocUpdateItem("") {
    // purify_watch_rw_4(cpp);
    pyvar_ = pyvar;
    cpp_ = cpp;
    cp_ = NULL;
    if (pyvar_) {
        hoc_obj_ref(pyvar_);
        neuron::python::methods.guigetstr(pyvar_, &cp_);
    } else {
        cp_ = *cpp_;
    }
    p_ = new Patch(LayoutKit::instance()->margin(WidgetKit::instance()->label(cp_), 3));
    p_->ref();
    pg->append(p_);
}

HocVarLabel::~HocVarLabel() {
    p_->unref();
    if (pyvar_) {
        hoc_obj_unref(pyvar_);
        if (cp_) {
            delete[] cp_;
        }
    }
}

void HocVarLabel::write(std::ostream& o) {
    if (!variable_.empty() && cpp_) {
        char buf[256];
        Sprintf(buf, "xvarlabel(%s)", variable_.c_str());
        o << buf << std::endl;
    } else {
        o << "xlabel(\"<can't retrieve>\")" << std::endl;
    }
}

void HocVarLabel::update_hoc_item() {
    if (pyvar_) {
        if (neuron::python::methods.guigetstr(pyvar_, &cp_)) {
            p_->body(LayoutKit::instance()->margin(WidgetKit::instance()->label(cp_), 3));
            p_->redraw();
            p_->reallocate();
            p_->redraw();
        }
    } else if (cpp_) {
        // printf("update %s\n", cp_);
        if (*cpp_ != cp_) {
            cp_ = *cpp_;
            // printf("replacing with %s\n", cp_);
            p_->body(LayoutKit::instance()->margin(WidgetKit::instance()->label(cp_), 3));
            p_->redraw();
            p_->reallocate();
            p_->redraw();
        }
    } else if (cp_) {
        cp_ = 0;
        // printf("HocVarLabel::update() freed\n");
        p_->body(LayoutKit::instance()->margin(WidgetKit::instance()->label("Free'd"), 3));
        p_->redraw();
        p_->reallocate();
        p_->redraw();
    }
}

HocMenuAction::HocMenuAction(const char* action, Object* pyact, HocMenu* hm)
    : HocAction(action, pyact) {
    hm_ = hm;
    hp_ = NULL;
}
HocMenuAction::~HocMenuAction() {
    Resource::unref(hp_);
}
void HocMenuAction::execute() {
    while (hm_->menu()->item_count()) {
        hm_->menu()->remove_item(0);
    }
    Resource::unref(hp_);
    hp_ = NULL;
    hoc_ivpanel("");
    menuStack->push(hm_);
    HocAction::execute();
    menuStack->pop();
    checkOpenPanel();
    hp_ = curHocPanel;
    curHocPanel = NULL;
    hm_->item()->menu(hm_->menu());
}

// HocAction
HocAction::HocAction(const char* action, Object* pyact) {
    hi_ = NULL;
    if (pyact) {
        action_ = new HocCommand(pyact);
    } else if (action && action[0] != '\0') {
        action_ = new HocCommand(action);
    } else {
        action_ = NULL;
    }
}

HocAction::~HocAction() {
    if (action_) {
        delete action_;
    }
}

void HocAction::hoc_item(HocItem* hi) {
    hi_ = hi;
}

void HocAction::execute() {
    if (Oc::helpmode()) {
        help();
        return;
    }
    PanelInputHandler::handle_old_focus();
    if (action_) {
        action_->audit();
        action_->execute();
    } else {
        Oc oc;
        oc.notify();
    }
}

void HocAction::help() {
    if (hi_) {
        hi_->help();
    }
}

const char* HocAction::name() const {
    if (action_) {
        return action_->name();
    } else {
        return "";
    }
}

#if UseFieldEditor
declareFieldEditorCallback(HocValAction)
implementFieldEditorCallback(HocValAction)
#else
declareFieldSEditorCallback(HocValAction);
implementFieldSEditorCallback(HocValAction);
#endif
// HocValAction
HocValAction::HocValAction(const char* action, Object* pyact)
    : HocAction(action, pyact) {
    fe_ = NULL;
#if UseFieldEditor
    fea_ = new FieldEditorCallback(HocValAction)(
#else
    fea_ = new FieldSEditorCallback(HocValAction)(
#endif
        this, &HocValAction::accept, NULL);
    fea_->ref();
}

HocValAction::~HocValAction() {
    // printf("~HocValAction\n");
    fea_->unref();
}

void HocValAction::setFieldSEditor(HocValEditor* fe) {
    fe_ = fe;  // but not referenced since this action is referenced by fe
}

void HocValAction::accept(FieldSEditor*) {
    if (fe_->active()) {
        fe_->field_editor()->parent()->focus(NULL);
    } else {
        fe_->evalField();
    }
    fe_->audit();
    HocAction::execute();
}

void HocValAction::execute() {
    if (Oc::helpmode()) {
        fe_->help();
        return;
    }
    accept(fe_->field_editor());
}

class HocDefaultCheckbox: public Button {
  public:
    HocDefaultCheckbox(HocDefaultValEditor*, Glyph*, Style*, TelltaleState*, Action*);
    virtual ~HocDefaultCheckbox();
    static HocDefaultCheckbox* instance(HocDefaultValEditor*);
    virtual void release(const Event&);

  private:
    HocDefaultValEditor* dve_;
};

HocDefaultCheckbox::HocDefaultCheckbox(HocDefaultValEditor* dve,
                                       Glyph* g,
                                       Style* s,
                                       TelltaleState* t,
                                       Action* a)
    : Button(g, s, t, a) {
    dve_ = dve;
}

HocDefaultCheckbox::~HocDefaultCheckbox() {}

HocDefaultCheckbox* HocDefaultCheckbox::instance(HocDefaultValEditor* dve) {
    Glyph* g;
    TelltaleState* t;
    WidgetKit& k = *WidgetKit::instance();
    Style* s;
    k.begin_style("ToggleButton", "Button");
    t = new TelltaleState(TelltaleState::is_enabled | TelltaleState::is_toggle);
    g = k.check_box_look(NULL, t);
    s = k.style();
    HocDefaultCheckbox* cb = new HocDefaultCheckbox(dve, g, s, t, NULL);
    k.end_style();
    return cb;
}

void HocDefaultCheckbox::release(const Event& e) {
    if (Oc::helpmode()) {
        Button::release(e);
    }
    if (e.pointer_button() == Event::right) {
        dve_->def_change(e.pointer_root_x(), e.pointer_root_y());
    }
    Button::release(e);
}

declareActionCallback(HocDefaultValEditor);
implementActionCallback(HocDefaultValEditor);

// HocDefaultValEditor
HocDefaultValEditor::HocDefaultValEditor(const char* name,
                                         const char* variable,
                                         ValEdLabel* prompt,
                                         HocValAction* a,
                                         neuron::container::data_handle<double> pd,
                                         bool canrun,
                                         HocItem* hi,
                                         Object* pyvar)
    : HocValEditor(name, variable, prompt, a, pd, canrun, hi, pyvar) {
    checkbox_ = HocDefaultCheckbox::instance(this);
    checkbox_->ref();
    checkbox_->action(
        new ActionCallback(HocDefaultValEditor)(this, &HocDefaultValEditor::def_action));
    evalField();
    deflt_ = most_recent_ = get_val();
    vs_ = HocValStepper::instance(this);
    Resource::ref(vs_);
}

HocDefaultValEditor::~HocDefaultValEditor() {
    checkbox_->unref();
    vs_->unref();
}

void HocDefaultValEditor::def_change(float x0, float y0) {
    evalField();
    double x = get_val();
    if (x != deflt_) {
        char form[200], buf[200];
        Sprintf(form,
                "Permanently replace default value %s with %s",
                xvalue_format->string(),
                xvalue_format->string());
        Sprintf(buf, form, deflt_, x);
        if (boolean_dialog(buf, "Replace", "Cancel", NULL, x0, y0)) {
            deflt_ = x;
            most_recent_ = x;
        }
    }
}

void HocDefaultValEditor::updateField() {
    HocValEditor::updateField();
    TelltaleState* t = checkbox_->state();
    //	printf("telltale flag %x\n", t->flags());
    bool same = (hoc_ac_ == deflt_);
    bool chosen = t->test(TelltaleState::is_chosen);
    if (same && chosen) {
        t->set(TelltaleState::is_chosen, false);
    } else if (!same) {
        most_recent_ = hoc_ac_;
        if (!chosen) {
            t->set(TelltaleState::is_chosen, true);
        }
    }
}

void HocDefaultValEditor::deflt(double d) {
    deflt_ = d;
}

void HocDefaultValEditor::def_action() {
    if (Oc::helpmode()) {
        checkbox_->state()->set(TelltaleState::is_chosen,
                                !checkbox_->state()->test(TelltaleState::is_chosen));
        Oc::help(Editor_Default);
        return;
    }
    bool chosen = checkbox_->state()->test(TelltaleState::is_chosen);
    if (chosen) {
        if (most_recent_ != deflt_) {
            set_val(most_recent_);
        }
    } else {
        double x = get_val();
        if (deflt_ != x) {
            most_recent_ = x;
            set_val(deflt_);
        }
    }
    //	Oc oc;
    //	oc.notifyHocValue();
    updateField();
    exec_action();
}

// HocValEditorKeepUpdated
HocValEditorKeepUpdated::HocValEditorKeepUpdated(const char* name,
                                                 const char* variable,
                                                 ValEdLabel* prompt,
                                                 HocValAction* act,
                                                 neuron::container::data_handle<double> pd,
                                                 HocItem* hi,
                                                 Object* pyvar)
    : HocValEditor(name, variable, prompt, act, pd, false, hi, pyvar) {
    //	printf("~HocValEditorKeepUpdated\n");
    HocPanel::keep_updated(this, true);
}
HocValEditorKeepUpdated::~HocValEditorKeepUpdated() {
    //	printf("~HocValEditorKeepUpdated\n");
    HocPanel::keep_updated(this, false);
}

// HocEditorForItem g++ doesn't do multiple inheritance of same base classes.

HocEditorForItem::HocEditorForItem(HocValEditor* he, HocValAction* a)
    : FieldSEditor("", WidgetKit::instance(), Session::instance()->style(), a->fea()) {
    hve_ = he;
#ifdef WIN32
    FieldSEditor::focus_out();
#endif
    //	hve_->ref();
}
HocEditorForItem::~HocEditorForItem() {
    //	hve_->unref();
}


static void set_format() {
    static Coord len;
    if (!xvalue_format) {
        xvalue_format = new String("%.5g");
        WidgetKit::instance()->style()->find_attribute("xvalue_format", *xvalue_format);
        char buf[100];
        Sprintf(buf, xvalue_format->string(), -8.888888888888888e-18);
        Glyph* g = WidgetKit::instance()->label(buf);
        g->ref();
        Requisition r;
        g->request(r);

        // mike_neubig_ivoc_xmenu
        float fct;
        Style* s = WidgetKit::instance()->style();
        if (!s->find_attribute("xvalue_field_size_increase", fct)) {
            fct = 10;
        }
        xvalue_field_size = r.x_requirement().natural() + fct;
        g->unref();
    }
}

double MyMath::resolution(double x) {
    if (!xvalue_format) {
        set_format();
    }
    char buf[100];
    Sprintf(buf, xvalue_format->string(), std::abs(x));
    char* cp;
    char* least = NULL;
    for (cp = buf; *cp; ++cp) {
        if (isdigit(*cp)) {
            least = cp;
            break;
        }
    }
    for (; *cp; ++cp) {
        if (*cp >= '1' && *cp <= '9') {
            *cp = '0';
            least = cp;
        }
        if (isalpha(*cp)) {
            break;
        }
    }
    assert(least);
    *least = '1';
    double y;
    sscanf(buf, "%lf", &y);
    return y;
}

// HocValEditor
HocValEditor::HocValEditor(const char* name,
                           const char* variable,
                           ValEdLabel* prompt,
                           HocValAction* a,
                           neuron::container::data_handle<double> pd,
                           bool canrun,
                           HocItem* hi,
                           Object* pyvar)
    : HocUpdateItem(name, hi)
    , pval_{pd} {
    if (!xvalue_format) {
        set_format();
    }
    action_ = a;
    fe_ = new HocEditorForItem(this, a);
    fe_->ref();
    Resource::ref(a);
    prompt_ = prompt;
    prompt->ref();
    canrun_ = canrun;
    active_ = false;
    domain_limits_ = NULL;
    pyvar_ = pyvar;
    if (pyvar) {
        hoc_obj_ref(pyvar);
    } else if (variable) {
        variable_ = variable;
        Symbol* sym = hoc_get_symbol(variable);
        if (sym && sym->extra) {
            domain_limits_ = sym->extra->parmlimits;
        }
    }
    HocValEditor::updateField();
    fe_->focus_out();
}

HocValEditor::~HocValEditor() {
    // printf("~HocValEditor\n");
    if (pyvar_) {
        hoc_obj_unref(pyvar_);
    }
    Resource::unref(action_);
    Resource::unref(prompt_);
    fe_->unref();
}

void HocValEditor::setlimits(float* limits) {
    domain_limits_ = limits;
}

void HocValEditor::update_hoc_item() {
    updateField();
}

void HocValEditor::exec_action() {
    if (action_) {
        action_->execute();
    } else {
        Oc oc;
        oc.notify();
    }
}

void HocValEditor::print(Printer* p, const Allocation& a) const {
    //	printf("HocvalEditor::print\n");
    Glyph* l = WidgetKit::instance()->label(*fe_->text());
    l->ref();
    l->print(p, a);
    l->unref();
}

void HocValEditor::set_val(double x) {
    char buf[200];
    if (pyvar_) {
        neuron::python::methods.guisetval(pyvar_, x);
        return;
    }
    hoc_ac_ = x;
    Oc oc;
    if (pval_) {
        *pval_ = hoc_ac_;
    } else if (!variable_.empty()) {
        Sprintf(buf, "%s = hoc_ac_\n", variable_.c_str());
        oc.run(buf);
    }
}

double HocValEditor::get_val() {
    char buf[200];
    if (pyvar_) {
        return neuron::python::methods.guigetval(pyvar_);
    } else if (pval_) {
        return *pval_;
    } else if (!variable_.empty()) {
        Oc oc;
        Sprintf(buf, "hoc_ac_ = %s\n", variable_.c_str());
        oc.run(buf);
        return hoc_ac_;
    } else {
        return 0.;
    }
}

double HocValEditor::domain_limits(double val) {
    return check_domain_limits(domain_limits_, val);
}

void HocValEditor::evalField() {
    char buf[200];
    Oc oc;
    Sprintf(buf, "hoc_ac_ = %s\n", fe_->text()->string());
    oc.run(buf);
    hoc_ac_ = domain_limits(hoc_ac_);
    set_val(hoc_ac_);
    // prompt_->state()->set(TelltaleState::is_active, false);
    prompt_->state(false);
}

void HocValEditor::audit() {
    auto sout = std::stringstream{};
    if (pyvar_) {
        return;
    } else if (!variable_.empty()) {
        sout << variable_ << " = " << fe_->text()->string();
    } else if (pval_) {
        sout << "// " << pval_ << " set to " << fe_->text()->string();
    }
    auto buf = sout.str();
    hoc_audit_command(buf.c_str());
}

void HocValEditor::updateField() {
    if (active_)
        return;
    char buf[200];
    if (pyvar_) {
        hoc_ac_ = get_val();
        Sprintf(buf, xvalue_format->string(), hoc_ac_);
    } else if (pval_) {
        Sprintf(buf, xvalue_format->string(), *pval_);
        hoc_ac_ = *pval_;
    } else if (!variable_.empty()) {
        Oc oc;
        Sprintf(buf, "hoc_ac_ = %s\n", variable_.c_str());
        if (oc.run(buf, 0)) {
            strcpy(buf, "Doesn't exist");
        } else {
            Sprintf(buf, xvalue_format->string(), hoc_ac_);
        }
    } else {
        Sprintf(buf, "Free'd");
    }
    if (strcmp(buf, fe_->text()->string()) != 0) {
        fe_->field(buf);
    }
}

void HocValEditor::write(std::ostream& o) {
    char buf[200];
    Oc oc;
    if (!variable_.empty()) {
        Sprintf(buf, "hoc_ac_ = %s\n", variable_.c_str());
        oc.run(buf);
        Sprintf(buf, "%s = %g", variable_.c_str(), hoc_ac_);
    } else if (pval_) {
        Sprintf(buf, "/* don't know the hoc path to %g", *pval_);
        return;
    } else {
        Sprintf(buf, "/* variable freed */");
        return;
    }
    o << buf << std::endl;

    int usepointer;
    if (pval_) {
        usepointer = 1;
    } else {
        usepointer = 0;
    }
    nrn_assert(snprintf(buf,
                        200,
                        "xvalue(\"%s\",\"%s\", %d,\"%s\", %d, %d )",
                        getStr(),
                        variable_.c_str(),
                        hoc_default_val_editor(),
                        hideQuote(action_->name()),
                        (int) canrun_,
                        usepointer) < 200);
    o << buf << std::endl;
}

const char* HocValEditor::variable() const {
    if (!variable_.empty()) {
        return variable_.c_str();
    } else {
        return NULL;
    }
}


void HocValEditorKeepUpdated::write(std::ostream& o) {
    char buf[200];
    Oc oc;
    Sprintf(buf, "hoc_ac_ = %s\n", variable());
    oc.run(buf);
    Sprintf(buf, "%s = %g", variable(), hoc_ac_);
    o << buf << std::endl;
    Sprintf(buf, "xvalue(\"%s\",\"%s\", 2 )", getStr(), variable());
    o << buf << std::endl;
}

void HocEditorForItem::keystroke(const Event& e) {
    bool unfocus = false;
    if (!hve_->active_) {
        return;
    }
    if (Oc::helpmode()) {
        hve_->help();
        return;
    }
    char buf[2];
    if (e.mapkey(buf, 1) > 0)
        switch (buf[0]) {
        case '\n':
        case '\r':
            unfocus = true;
            break;
        case '\033':
            hve_->active_ = false;
            hve_->updateField();
            hve_->active_ = true;
            parent()->focus(NULL);
            return;
        case '\007':
            hve_->active_ = false;
            hve_->updateField();
            hve_->active_ = true;
            return;
        default:
            break;
        }
    FieldSEditor::keystroke(e);
    if (unfocus) {
        parent()->focus(NULL);
    }
}
class HocEditorTempData {
  public:
    void init(const Event&);
    int sn(const Event&);
#if 0
	Coord x_, y_;
	Coord xd_, yd_;
#endif
    int sn_;
    int index_;
    EventButton b_;
};

static HocEditorTempData etd;

void HocEditorTempData::init(const Event& e) {
#if 0
	x_ = e.pointer_x();
	y_ = e.pointer_y();
	xd_ = 1.;
	yd_ = 1.;
#endif
    b_ = e.pointer_button();
    if (b_ == Event::right) {
        sn_ = -1;
    } else {
        sn_ = 1;
    }
}

int HocEditorTempData::sn(const Event&) {
#if 0
	float xnew = e.pointer_x();
	float ynew = e.pointer_y();
#undef RES
#define RES 3
	if (Math::equal(x_, xnew, float(RES)) && Math::equal(y_, ynew, float(RES))) {
		return 0;
	}
	sn_ = ( ((xnew - x_)*xd_ + (ynew - y_)*yd_ >= 0.)) ? sn_ : -sn_;
	xd_ = xnew - x_;
	yd_ = ynew - y_;
	x_ = xnew;
	y_ = ynew;
#endif
    return sn_;
}

void HocEditorForItem::press(const Event& e) {
    if (Oc::helpmode()) {
        hve_->help();
        return;
    }
    //	if (!hve_->active_) {
    //		focus_in();
    //	}
    FieldSEditor::press(e);
#if !UseFieldEditor
    int start;
    FieldSEditor::selection(start, index_);
    etd.init(e);
#endif
}

void HocEditorForItem::drag(const Event& e) {
#ifdef WIN32
    FieldSEditor::drag(e);
#else
    if (etd.b_ == Event::left) {
        FieldSEditor::drag(e);
    } else {
        val_inc(e);
    }
#endif
}

void HocEditorForItem::val_inc(const Event& e) {
    int index = index_;
    int i, sn;
    i = index;
    sn = etd.sn(e);
    if (sn == 0) {
        return;
    }
    const char* s = text()->string();
    char abuf[100];
    char* buf = abuf + 1;
    strcpy(buf, s);
    if (i == strlen(buf)) {
        buf[i] = '0';
        buf[i + 1] = '\0';
    }
    while (i >= 0) {
        if (isdigit(buf[i])) {
            buf[i] = (((buf[i] - '0') + sn + 100) % 10) + '0';
            if (sn == 1 && buf[i] != '0') {
                break;
            } else if (sn == -1 && buf[i] != '9') {
                break;
            }
        }
        --i;
    }
    if (i < 0) {
        if (buf[0] == '-') {
            if (sn == 1) {
                buf[0] = '1';
                abuf[0] = '-';
                buf = abuf;
                ++index_;
            } else {
                strcpy(buf, s);
            }
        } else {
            if (sn == 1) {
                abuf[0] = '1';
                buf = abuf;
                ++index_;
            } else {
                strcpy(buf, s);
            }
        }
    }
    field(buf);
}

void HocEditorForItem::release(const Event& e) {
    FieldSEditor::release(e);
}


InputHandler* HocEditorForItem::focus_in() {
    // printf("HocEditorForItem::focus_in()\n");
    if (Oc::helpmode()) {
        return NULL;
    }
    if (!hve_->active_) {
        // hve_->prompt_->state()->set(TelltaleState::is_active, true);
        hve_->prompt_->state(true);
        hve_->active_ = true;
        return FieldSEditor::focus_in();
    } else {
        return InputHandler::focus_in();
    }
}

void HocEditorForItem::focus_out() {
    if (hve_->active_) {
        hve_->active_ = false;
        // hve_->prompt_->state()->set(TelltaleState::is_active, false);
        hve_->prompt_->state(false);
        hve_->evalField();
    }
    FieldSEditor::focus_out();
    if (PanelInputHandler::has_old_focus()) {
        // printf("old focus out %p\n", (InputHandler*)this);
        hve_->exec_action();
    }
}

void Oc::notifyHocValue() {
    // static int j=0;
    // printf("notifyHocValue %d\n", ++j);
    ParseTopLevel ptl;
    ptl.save();
    if (hoc_panel_list) {
        for (auto&& e: reverse(*hoc_panel_list)) {
            e->notifyHocValue();
        }
    }
    ptl.restore();
}

void HocPanel::notifyHocValue() {
    for (auto&& e: reverse(elist_)) {
        e->update_hoc_item();
    }
}

void HocPanel::check_valid_pointers(void* v, int size) {
    for (auto&& e: reverse(elist_)) {
        e->check_pointer(v, size);
    }
}

void HocValEditor::check_pointer(void* v, int size) {
    auto* const pval_raw = static_cast<double const*>(pval_);
    if (pval_raw) {
        auto* const pd = static_cast<double*>(v);
        if (size == 1) {
            if (pd != pval_raw) {
                return;
            }
        } else {
            if (pval_raw < pd || pval_raw >= pd + size)
                return;
        }
        pval_ = {};
    }
}
void HocVarLabel::check_pointer(void* v, int) {
    char** cpp = (char**) v;
    if (cpp_ == cpp) {
        cpp_ = 0;
    }
}

void HocPanel::data_path(HocDataPaths* hdp, bool append) {
    for (auto&& e: reverse(elist_)) {
        e->data_path(hdp, append);
    }
}

void HocValEditor::data_path(HocDataPaths* hdp, bool append) {
    if (variable_.empty()) {
        auto* const pval_raw = static_cast<double*>(pval_);
        if (append) {
            hdp->append(pval_raw);
        } else {
            variable_ = hdp->retrieve(pval_raw);
        }
    }
}

void HocVarLabel::data_path(HocDataPaths* hdp, bool append) {
    if (cpp_ && variable_.empty()) {
        if (append) {
            hdp->append(cpp_);
        } else {
            variable_ = hdp->retrieve(cpp_);
        }
    }
}

class StepperMenuAction: public Action {
  public:
    StepperMenuAction(bool, double);
    virtual ~StepperMenuAction();
    virtual void execute();

  private:
    double x_;
    bool geometric_;
};

class StepperMenu: public PopupMenu {
  public:
    StepperMenu();
    virtual ~StepperMenu();
    void stepper(HocValStepper* vs) {
        vs_ = vs;
    }
    HocValStepper* stepper() {
        return vs_;
    }
    virtual bool event(Event&);
    void active(bool b) {
        active_ = b;
    }
    bool active() {
        return active_;
    }

  private:
    bool active_;
    HocValStepper* vs_;
};

StepperMenu* HocValStepper::menu_;

StepperMenuAction::StepperMenuAction(bool b, double x) {
    x_ = x;
    geometric_ = b;
}
StepperMenuAction::~StepperMenuAction() {}
void StepperMenuAction::execute() {
    HocValStepper::menu()->stepper()->default_inc(geometric_, x_);
}
StepperMenu::StepperMenu() {
    WidgetKit& k = *WidgetKit::instance();
    char buf[50];
    active_ = false;
    vs_ = NULL;
    MenuItem* m;
    m = K::menu_item("Res");
    m->action(new StepperMenuAction(false, 0));
    append_item(m);
    m = K::menu_item("*10");
    m->action(new StepperMenuAction(true, 10));
    append_item(m);
    m = K::menu_item("*10^.1");
    m->action(new StepperMenuAction(true, pow(10., .1)));
    append_item(m);
    m = K::menu_item("*e");
    m->action(new StepperMenuAction(true, exp(1.)));
    append_item(m);
    m = K::menu_item("*e^.1");
    m->action(new StepperMenuAction(true, exp(.1)));
    append_item(m);
    m = K::menu_item("*2");
    m->action(new StepperMenuAction(true, 2));
    append_item(m);
    m = K::menu_item("*2^.1");
    m->action(new StepperMenuAction(true, pow(2., .1)));
    append_item(m);
    for (double x = 1000; x > .0005; x /= 10.) {
        Sprintf(buf, "+%g", x);
        m = K::menu_item(buf);
        m->action(new StepperMenuAction(false, x));
        append_item(m);
    }
}
StepperMenu::~StepperMenu() {}
bool StepperMenu::event(Event& e) {
    PopupMenu::event(e);
    if (e.type() == Event::up) {
        vs_->menu_up(e);
    }
    return true;
}

/* static */ class NrnUpDown: public Glyph {
  public:
    static NrnUpDown* instance();
    virtual ~NrnUpDown();
    virtual void request(Requisition&) const;
    virtual void draw(Canvas*, const Allocation&) const;

  private:
    NrnUpDown(const Color*);
    static NrnUpDown* instance_;
    const Color* color_;
};

NrnUpDown* NrnUpDown::instance_;

NrnUpDown::NrnUpDown(const Color* c)
    : Glyph() {
    color_ = c;
    Resource::ref(c);
}

NrnUpDown::~NrnUpDown() {
    Resource::unref(color_);
}

NrnUpDown* NrnUpDown::instance() {
    if (!instance_) {
        instance_ = new NrnUpDown(WidgetKit::instance()->foreground());
        instance_->ref();
    }
    return instance_;
}

#if 1
void NrnUpDown::request(Requisition& r) const {
    Requirement x(10);
    Requirement y(20);
    r.require_x(x);
    r.require_y(y);
}
#endif

void NrnUpDown::draw(Canvas* c, const Allocation& a) const {
    Coord x1 = a.left();
    Coord y1 = a.bottom();
    Coord x2 = a.right();
    Coord y2 = a.top();
    // printf("draw %g %g %g %g\n", x1, y1, x2, y2);
    Coord x = (x1 + x2) * .5;
    Coord y = (y1 + y2) * .5;

    c->new_path();
    c->move_to(x1, y + 1);
    c->line_to(x, y2);
    c->line_to(x2, y + 1);
    c->close_path();
    c->fill(color_);

#if 1
    c->new_path();
    c->move_to(x1, y - 1);
    c->line_to(x, y1);
    c->line_to(x2, y - 1);
    c->close_path();
    c->fill(color_);
#endif
}

static Glyph* up_down_mover_look(TelltaleState* t) {
    return WidgetKit::instance()->push_button_look(NrnUpDown::instance(), t);
}

HocValStepper* HocValStepper::instance(HocValEditor* ve) {
    Glyph* g;
    TelltaleState* t;
    WidgetKit& k = *WidgetKit::instance();
    Style* s;
    k.begin_style("UpMover", "Button");
    t = new TelltaleState(TelltaleState::is_enabled);
    g = up_down_mover_look(t);
    //	g = k.up_mover_look(t);
    s = k.style();
    HocValStepper* vs = new HocValStepper(ve, g, s, t);
    k.end_style();
    return vs;
}

HocValStepper::HocValStepper(HocValEditor* ve, Glyph* g, Style* s, TelltaleState* t)
    : Stepper(g, s, t) {
    if (!menu_) {
        menu_ = new StepperMenu();
        menu_->ref();
    }
    hve_ = ve;
    default_inc_ = MyMath::resolution(hve_->get_val());
    geometric_ = false;
}
HocValStepper::~HocValStepper() {}
void HocValStepper::press(const Event& e) {
    steps_ = 0;
    inc_ = default_inc_;
    menu_->active(false);
    if (Oc::helpmode()) {
        return;
    }
    switch (e.pointer_button()) {
    case Event::left:
    case Event::middle: {
        const Allocation& a = allocation();
        if (e.pointer_y() < (a.bottom() + a.top()) * .5) {
            if (geometric_) {
                inc_ = 1. / default_inc_;
            } else {
                inc_ *= -1;
            }
        }
        menu_->stepper(this);
        Stepper::press(e);
        break;
    }
    case Event::right: {
        menu_->active(true);
        menu_->stepper(this);
        Event e1(e);
        menu_->event(e1);
    } break;
    }
}
void HocValStepper::default_inc(bool g, double x) {
    if (x == 0.) {
        default_inc_ = MyMath::resolution(hve_->get_val());
        geometric_ = false;
    } else {
        default_inc_ = x;
        geometric_ = g;
    }
}

void HocValStepper::release(const Event& e) {
    if (Oc::helpmode()) {
        Oc::help(Editor_Stepper);
        return;
    }
    if (menu_->active()) {
        menu_->active(false);
        Button::release(e);
        return;
    }
    Stepper::release(e);
    Oc oc;
    hve_->exec_action();
    oc.notify();
}
void HocValStepper::menu_up(Event& e) {
    menu_->active(true);
#ifdef WIN32
    handler()->event(e);
#else
    e.unread();
#endif
}

void HocValStepper::adjust() {
    double x, y;
    x = hve_->get_val();
    if (geometric_) {
        y = x * inc_;
    } else {
        y = x + inc_;
    }
    y = hve_->domain_limits(y);
    if (steps_ > 0 && x * y <= 0.) {
        y = 0.;
        inc_ = 0.;
    }
    hve_->set_val(y);
    hve_->updateField();
    if (!geometric_ && ((++steps_) % 20) == 0) {
        inc_ *= 10.;
    }
}
void HocValStepper::left() {
    inc_ = default_inc_;
}
void HocValStepper::middle() {
    inc_ = default_inc_;
}
void HocValStepper::right() {}

// OcSlider

// ZFM added vert_
OcSlider::OcSlider(neuron::container::data_handle<double> pd,
                   float low,
                   float high,
                   float resolution,
                   int nsteps,
                   const char* send,
                   bool vert,
                   bool slow,
                   Object* pyvar,
                   Object* pysend)
    : HocUpdateItem("") {
    resolution_ = resolution;
    pval_ = pd;
    pyvar_ = pyvar;
    if (pyvar_) {
        hoc_obj_ref(pyvar_);
    }
    vert_ = vert;
    slow_ = slow;
    bv_ = new BoundedValue(low, high);
    bv_->scroll_incr((high - low) / nsteps);
    if (send) {
        send_ = new HocCommand(send);
    } else if (pysend) {
        send_ = new HocCommand(pysend);
    } else {
        send_ = NULL;
    }
    bv_->attach(Dimension_X, this);
    scrolling_ = false;
}
OcSlider::~OcSlider() {
    if (send_) {
        delete send_;
    }
    delete bv_;
    if (pyvar_) {
        hoc_obj_unref(pyvar_);
    }
}
Adjustable* OcSlider::adjustable() {
    return bv_;
}

static double last_send;
void OcSlider::update(Observable*) {
    double x = slider_val();
    if (pval_) {
        *pval_ = x;
    } else if (pyvar_) {
        neuron::python::methods.guisetval(pyvar_, x);
    } else {
        return;
    }
    if (!scrolling_) {
        scrolling_ = true;
        while (Coord(x) != last_send) {
            audit();
            last_send = Coord(x);
            if (send_) {
                send_->execute();
            } else {
                Oc oc;
                oc.notify();
            }
        }
        scrolling_ = false;
    }
}

void OcSlider::audit() {
    auto sout = std::stringstream{};
    char buf[200];
    Sprintf(buf, "%g", *pval_);
    if (!variable_.empty()) {
        sout << variable_.c_str() << " = " << buf << "\n";
    } else if (pval_) {
        sout << "// " << pval_ << " set to " << buf << "\n";
    }
    auto str = sout.str();
    hoc_audit_command(str.c_str());
    if (send_) {
        send_->audit();
    }
}

double OcSlider::slider_val() {
    double x = double(bv_->cur_lower(Dimension_X));
    x = MyMath::anint(x / resolution_);
    x = x * resolution_;
    if (x > bv_->upper(Dimension_X) - resolution_ / 2.)
        x = bv_->upper(Dimension_X);
    if (x < bv_->lower(Dimension_X) + resolution_ / 2.)
        x = bv_->lower(Dimension_X);

    return x;
}

void OcSlider::update_hoc_item() {
    Coord x = 0.;
    if (pyvar_) {
        x = Coord(neuron::python::methods.guigetval(pyvar_));
    } else if (pval_) {
        x = Coord(*pval_);
    } else {
        pval_ = {};
        return;
    }
    if (x != bv_->cur_lower(Dimension_X)) {
        bool old = scrolling_;
        scrolling_ = true;
        bv_->scroll_to(Dimension_X, x);
        scrolling_ = old;
    }
}
void OcSlider::check_pointer(void* v, int size) {
    auto* const pval_raw = static_cast<double const*>(pval_);
    if (pval_raw) {
        auto* const pd = static_cast<double*>(v);
        if (size == 1) {
            if (pd != pval_raw)
                return;
        } else {
            if (pval_raw < pd || pval_raw >= pd + size)
                return;
        }
        pval_ = {};
    }
}
void OcSlider::data_path(HocDataPaths* hdp, bool append) {
    if (variable_.empty() && pval_) {
        auto* const pval_raw = static_cast<double*>(pval_);
        if (append) {
            hdp->append(pval_raw);
        } else {
            variable_ = hdp->retrieve(pval_raw);
        }
    }
}
void OcSlider::write(std::ostream& o) {
    if (!variable_.empty()) {
        char buf[256];
        if (send_) {
            Sprintf(buf,
                    "xslider(&%s, %g, %g, \"%s\", %d, %d)",
                    variable_.c_str(),
                    bv_->lower(Dimension_X),
                    bv_->upper(Dimension_X),
                    hideQuote(send_->name()),
                    vert_,
                    slow_);
        } else {
            Sprintf(buf,
                    "xslider(&%s, %g, %g, %d, %d)",
                    variable_.c_str(),
                    bv_->lower(Dimension_X),
                    bv_->upper(Dimension_X),
                    vert_,
                    slow_);
        }
        o << buf << std::endl;
    }
}


//  Button with state

void HocPanel::stateButton(neuron::container::data_handle<double> pd,
                           const char* name,
                           const char* action,
                           int style,
                           Object* pyvar,
                           Object* pyact) {
    HocAction* act = new HocAction(action, pyact);
    Button* button;
    if (style == HocStateButton::PALETTE) {
        button = WidgetKit::instance()->palette_button(name, act);
    } else {
        button = WidgetKit::instance()->check_box(name, act);
    }
    box()->append(button);
    HocStateButton* hsb = new HocStateButton(pd, name, button, act, style, hoc_item(), pyvar);
    item_append(hsb);
    elist_.push_back(hsb);
    hsb->ref();
}

declareActionCallback(HocStateButton);
implementActionCallback(HocStateButton);

HocStateButton::HocStateButton(neuron::container::data_handle<double> pd,
                               const char* text,
                               Button* button,
                               HocAction* action,
                               int style,
                               HocItem* hi,
                               Object* pyvar)
    : HocUpdateItem("", hi) {
    style_ = style;
    pval_ = pd;
    pyvar_ = pyvar;
    if (pyvar_) {
        hoc_obj_ref(pyvar_);
    }
    name_ = new CopyString(text);
    action_ = action;
    action->hoc_item(this);
    Resource::ref(action_);

    b_ = button;
    //	b_->ref();	// mutually reffed so don't
    b_->action(new ActionCallback(HocStateButton)(this, &HocStateButton::button_action));
}


HocStateButton::~HocStateButton() {
    if (pyvar_) {
        hoc_obj_unref(pyvar_);
    }
    delete name_;
    Resource::unref(action_);
    //	only come here when b is being deleted
    //    Resource::unref(b_);
    //   delete b_;
}

void HocStateButton::print(Printer* pr, const Allocation& a) const {
    Glyph* l = WidgetKit::instance()->label(name_->string());
    l->ref();
    l->print(pr, a);
    l->unref();
}

bool HocStateButton::chosen() {
    return b_->state()->test(TelltaleState::is_chosen);
}

void HocStateButton::button_action() {
    if (Oc::helpmode()) {
        help();
        b_->state()->set(TelltaleState::is_chosen, !chosen());
        return;
    }
    if (pyvar_) {
        TelltaleState* t = b_->state();
        if (chosen() != bool(neuron::python::methods.guigetval(pyvar_))) {
            neuron::python::methods.guisetval(pyvar_, double(chosen()));
        }
    } else if (pval_) {
        TelltaleState* t = b_->state();
        if (chosen() != bool(*pval_)) {
            *pval_ = double(chosen());
        }
    }
    if (action_) {
        action_->execute();
    } else {
        Oc oc;
        oc.notify();
    }
}


// set state of button to match variable
void HocStateButton::update_hoc_item() {
    double x = 0.;
    if (pyvar_) {
        x = neuron::python::methods.guigetval(pyvar_);
    } else if (pval_) {
        x = *pval_;
    } else {  // not (no longer) valid
        pval_ = {};
        b_->state()->set(TelltaleState::is_enabled_visible_active_chosen, false);
        return;
    }
    if (x) {
        b_->state()->set(TelltaleState::is_chosen, true);
    } else {
        b_->state()->set(TelltaleState::is_chosen, false);
    }
}

void HocStateButton::check_pointer(void* v, int size) {
    auto* const pval_raw = static_cast<double const*>(pval_);
    if (pval_raw) {
        auto* const pd = static_cast<double*>(v);
        if (size == 1) {
            if (pd != pval_raw)
                return;
        } else {
            if (pval_raw < pd || pval_raw >= pd + size)
                return;
        }
        pval_ = {};
    }
}

void HocStateButton::data_path(HocDataPaths* hdp, bool append) {
    if (variable_.empty() && pval_) {
        auto* const pval_raw = static_cast<double*>(pval_);
        if (append) {
            hdp->append(pval_raw);
        } else {
            variable_ = hdp->retrieve(pval_raw);
        }
    }
}
void HocStateButton::write(std::ostream& o) {
    if (!variable_.empty()) {
        char buf[256];
        if (style_ == PALETTE) {
            Sprintf(buf,
                    "xstatebutton(\"%s\",&%s,\"%s\")",
                    name_->string(),
                    variable_.c_str(),
                    hideQuote(action_->name()));
        } else {
            Sprintf(buf,
                    "xcheckbox(\"%s\",&%s,\"%s\")",
                    name_->string(),
                    variable_.c_str(),
                    hideQuote(action_->name()));
        }
        o << buf << std::endl;
    }
}


// menu item with state


MenuItem* HocPanel::menuStateItem(neuron::container::data_handle<double> pd,
                                  const char* name,
                                  const char* action,
                                  Object* pyvar,
                                  Object* pyact) {
    MenuItem* mi = WidgetKit::instance()->check_menu_item(name);
    HocAction* act = new HocAction(action, pyact);
    HocStateMenuItem* hsb = new HocStateMenuItem(pd, name, mi, act, hoc_item(), pyvar);
    item_append(hsb);
    elist_.push_back(hsb);
    hsb->ref();
    return mi;
}


declareActionCallback(HocStateMenuItem);
implementActionCallback(HocStateMenuItem);

HocStateMenuItem::HocStateMenuItem(neuron::container::data_handle<double> pd,
                                   const char* text,
                                   MenuItem* mi,
                                   HocAction* action,
                                   HocItem* hi,
                                   Object* pyvar)
    : HocUpdateItem("", hi) {
    pval_ = pd;
    pyvar_ = pyvar;
    if (pyvar_) {
        hoc_obj_ref(pyvar_);
    }
    name_ = new CopyString(text);
    action_ = action;
    action->hoc_item(this);
    Resource::ref(action_);

    b_ = mi;
    //	Resource::ref(b_);
    b_->action(new ActionCallback(HocStateMenuItem)(this, &HocStateMenuItem::button_action));
}


HocStateMenuItem::~HocStateMenuItem() {
    delete name_;
    if (pyvar_) {
        hoc_obj_unref(pyvar_);
    }
    Resource::unref(action_);
    //    Resource::unref(b_);
    //   delete b_;
}

void HocStateMenuItem::print(Printer* pr, const Allocation& a) const {
    Glyph* l = WidgetKit::instance()->label(name_->string());
    l->ref();
    l->print(pr, a);
    l->unref();
}

bool HocStateMenuItem::chosen() {
    return b_->state()->test(TelltaleState::is_chosen);
}


void HocStateMenuItem::button_action() {
    if (Oc::helpmode()) {
        help();
        b_->state()->set(TelltaleState::is_chosen, !chosen());
        return;
    }
    if (pval_) {
        TelltaleState* t = b_->state();
        if (chosen() != bool(*pval_)) {
            *pval_ = double(chosen());
        }
    }
    if (pyvar_) {
        TelltaleState* t = b_->state();
        if (chosen() != bool(neuron::python::methods.guigetval(pyvar_))) {
            neuron::python::methods.guisetval(pyvar_, double(chosen()));
        }
    }
    if (action_) {
        action_->execute();
    } else {
        Oc oc;
        oc.notify();
    }
}


// set state of button to match variable
void HocStateMenuItem::update_hoc_item() {
    double x = 0.;
    if (pyvar_) {
        x = neuron::python::methods.guigetval(pyvar_);
    } else if (pval_) {
        x = *pval_;
    } else {  // not (no longer) valid
        pval_ = {};
        b_->state()->set(TelltaleState::is_enabled_visible_active_chosen, false);
        return;
    }
    if (x) {
        b_->state()->set(TelltaleState::is_chosen, true);
    } else {
        b_->state()->set(TelltaleState::is_chosen, false);
    }
}

void HocStateMenuItem::check_pointer(void* v, int size) {
    auto* const pval_raw = static_cast<double const*>(pval_);
    if (pval_raw) {
        auto* const pd = static_cast<double*>(v);
        if (size == 1) {
            if (pd != pval_raw)
                return;
        } else {
            if (pval_raw < pd || pval_raw >= pd + size)
                return;
        }
        pval_ = {};
    }
}

void HocStateMenuItem::data_path(HocDataPaths* hdp, bool append) {
    if (variable_.empty() && pval_) {
        auto* const pval_raw = static_cast<double*>(pval_);
        if (append) {
            hdp->append(pval_raw);
        } else {
            variable_ = hdp->retrieve(pval_raw);
        }
    }
}

void HocStateMenuItem::write(std::ostream& o) {
    if (!variable_.empty()) {
        char buf[256];
        Sprintf(buf,
                "xcheckbox(\"%s\",&%s,\"%s\")",
                name_->string(),
                variable_.c_str(),
                hideQuote(action_->name()));

        o << buf << std::endl;
    }
}


#endif  // HAVE_IV
static void* vfe_cons(Object*) {
    TRY_GUI_REDIRECT_OBJ("ValueFieldEditor", NULL);
#if HAVE_IV
    IFGUI
    if (!ifarg(2) || hoc_is_str_arg(2)) {
        hoc_xvalue_helper();
    } else {
        hoc_xpvalue_helper();
    }
    HocValEditor* fe = last_fe_constructed_;
    Resource::ref(fe);
    return (void*) fe;
    ENDGUI
#endif
    return 0;
}
static void vfe_destruct(void* v) {
    TRY_GUI_REDIRECT_NO_RETURN("~ValueFieldEditor", v);
#if HAVE_IV
    IFGUI
    HocValEditor* fe = (HocValEditor*) v;
    Resource::unref(fe);
    ENDGUI
#endif
}
static double vfe_default(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("ValueFieldEditor.default", v);
    double x = 0.;
#if HAVE_IV
    IFGUI
    if (((HocValEditor*) v)->hoc_default_val_editor()) {
        HocDefaultValEditor* dfe = (HocDefaultValEditor*) v;
        dfe->deflt(x = dfe->get_val());
    }
    ENDGUI
#endif
    return x;
}
static Member_func vfe_members[] = {{"default", vfe_default}, {0, 0}};
void ValueFieldEditor_reg() {
    class2oc("ValueFieldEditor", vfe_cons, vfe_destruct, vfe_members, NULL, NULL, NULL);
}
