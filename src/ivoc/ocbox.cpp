#include <../../nrnconf.h>

#if HAVE_IV
#include <OS/string.h>
#include <InterViews/polyglyph.h>
#include <InterViews/layout.h>
#include <InterViews/place.h>
#include <InterViews/patch.h>
#include <InterViews/background.h>
#include <InterViews/box.h>
#include <InterViews/resource.h>
#include <IV-look/kit.h>
#include <InterViews/input.h>
#include <stdio.h>
#include "ocbox.h"
#include "apwindow.h"
#include "objcmd.h"
#include "ivoc.h"
#endif /* HAVE_IV */

#include "oc2iv.h"
#include "classreg.h"

#include "gui-redirect.h"

extern int hoc_return_type_code;

#if HAVE_IV

class NrnFixedLayout: public Layout {
  public:
    NrnFixedLayout(const DimensionName, Coord span);
    virtual ~NrnFixedLayout();

    virtual void request(GlyphIndex count, const Requisition*, Requisition& result);
    virtual void allocate(const Allocation& given,
                          GlyphIndex count,
                          const Requisition*,
                          Allocation* result);
    virtual void span(Coord);
    virtual Coord span() {
        return span_;
    }
    virtual bool vertical() {
        return dimension_ == Dimension_Y;
    }

  private:
    DimensionName dimension_;
    Coord span_;
};

/*static*/ class OcBoxImpl {
  public:
    PolyGlyph* ocglyph_list_;
    PolyGlyph* box_;
    Object* oc_ref_;  // reference to oc "this"
    CopyString* save_action_;
    Object* save_pyact_;
    int type_;
    std::ostream* o_;
    Object* keep_ref_;
    CopyString* dis_act_;
    Object* dis_pyact_;
    bool dismissing_;
    Coord next_map_adjust_;
    PolyGlyph* ba_list_;
    bool full_request_;
};

/*static*/ class BoxAdjust: public InputHandler {
  public:
    BoxAdjust(OcBox*, OcBoxImpl*, Glyph*, Coord natural);
    virtual ~BoxAdjust();
    virtual void press(const Event&);
    virtual void drag(const Event&);
    virtual void release(const Event&);
    NrnFixedLayout* fixlay_;
    OcBox* b_;
    OcBoxImpl* bi_;
    Glyph* ga_;  // not part of this glyph.
    Coord pstart_, fstart_;
};

/*static*/ class BoxDismiss: public WinDismiss {
  public:
    BoxDismiss(DismissableWindow*, String*, OcBox*, Object* pyact = NULL);
    virtual ~BoxDismiss();
    virtual void execute();

  private:
    HocCommand* hc_;
    OcBox* b_;
};

BoxDismiss::BoxDismiss(DismissableWindow* w, String* s, OcBox* b, Object* pyact)
    : WinDismiss(w) {
    if (pyact) {
        hc_ = new HocCommand(pyact);
    } else {
        hc_ = new HocCommand(s->string());
    }
    b_ = b;
}
BoxDismiss::~BoxDismiss() {
    delete hc_;
}
void BoxDismiss::execute() {
    if (b_->dismissing() == true) {
        WinDismiss::execute();
    } else {
        hc_->execute();
    }
}
#endif /* HAVE_IV */
static void* vcons(Object*) {
    TRY_GUI_REDIRECT_OBJ("VBox", NULL);
#if HAVE_IV
    OcBox* b = NULL;
    int frame = OcBox::INSET;
    bool scroll = false;
    if (ifarg(1))
        frame = int(chkarg(1, 0, 3));
    if (ifarg(2) && int(chkarg(2, 0, 1)) == 1) {
        scroll = true;
    }
    b = new OcBox(OcBox::V, frame, scroll);
    b->ref();
    return (void*) b;
#else
    return nullptr;
#endif /* HAVE_IV  */
}

static void* hcons(Object*) {
    TRY_GUI_REDIRECT_OBJ("HBox", NULL);
#if HAVE_IV
    OcBox* b = NULL;
    int frame = OcBox::INSET;
    if (ifarg(1))
        frame = int(chkarg(1, 0, 3));
    b = new OcBox(OcBox::H, frame);
    b->ref();
    return (void*) b;
#else
    return nullptr;
#endif /* HAVE_IV  */
}

static void destruct(void* v) {
    // TODO: this doesn't seem to get called; why?
    TRY_GUI_REDIRECT_NO_RETURN("~Box", v);
#if HAVE_IV
    OcBox* b = (OcBox*) v;
    IFGUI
    if (b->has_window()) {
        b->window()->dismiss();
    }
    ENDGUI
    b->unref();
#endif /* HAVE_IV */
}

static double intercept(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Box.intercept", v);
#if HAVE_IV
    bool b = int(chkarg(1, 0., 1.));
    IFGUI((OcBox*) v)->intercept(b);
    ENDGUI
    return double(b);
#else
    return 0.;
#endif /* HAVE_IV  */
}

static double ses_pri(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Box.priority", v);
#if HAVE_IV
    int p = int(chkarg(1, -1000, 10000));
    IFGUI((OcBox*) v)->session_priority(p);
    ENDGUI
    return double(p);
#else
    return 0.;
#endif /* HAVE_IV  */
}

static double map(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Box.map", v);
#if HAVE_IV
    IFGUI
    OcBox* b = (OcBox*) v;
    PrintableWindow* w;
    b->premap();
    if (ifarg(3)) {
        w = b->make_window(float(*getarg(2)),
                           float(*getarg(3)),
                           float(*getarg(4)),
                           float(*getarg(5)));
    } else {
        w = b->make_window();
    }
    if (ifarg(1)) {
        char* name = gargstr(1);
        w->name(name);
    }
    b->dismissing(false);
    w->map();
    if (b->full_request() && b->has_window()) {
        b->window()->request_on_resize(true);
    }
    b->dismiss_action(NULL);
    ENDGUI
    return 1.;
#else
    return 0.;
#endif /* HAVE_IV  */
}

static double dialog(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Box.dialog", v);
#if HAVE_IV
    bool r = false;
    IFGUI
    OcBox* b = (OcBox*) v;
    const char* a = "Accept";
    const char* c = "Cancel";
    if (ifarg(2)) {
        a = gargstr(2);
    }
    if (ifarg(3)) {
        c = gargstr(3);
    }
    Oc oc;
    oc.notify();
    r = b->dialog(gargstr(1), a, c);
    ENDGUI
    return double(r);
#else
    return 0.;
#endif /* HAVE_IV  */
}

static double unmap(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Box.unmap", v);
#if HAVE_IV
    IFGUI
    OcBox* b = (OcBox*) v;
    bool accept = true;
    if (ifarg(1)) {
        accept = (bool) chkarg(1, 0, 1);
    }
    if (b->dialog_dismiss(accept)) {
        return 0.;
    }
    if (b->has_window()) {
        b->ref();
        b->dismissing(true);
        b->window()->dismiss();
        b->window(NULL);  // so we don't come back here again before
                          // printable window destructor called
        b->unref();
    }
    ENDGUI
    return 0.;
#else
    return 0.;
#endif /* HAVE_IV  */
}

static double ismapped(void* v) {
    hoc_return_type_code = 2;
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Box.ismapped", v);
#if HAVE_IV
    bool b = false;
    IFGUI
    b = ((OcBox*) v)->has_window();
    ENDGUI
    return double(b);
#else
    return 0.;
#endif /* HAVE_IV  */
}

static double adjuster(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Box.adjuster", v);
#if HAVE_IV
    IFGUI((OcBox*) v)->adjuster(chkarg(1, -1., 1e5));
    ENDGUI
#endif /* HAVE_IV  */
    return 0.;
}

static double adjust(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Box.adjust", v);
#if HAVE_IV
    IFGUI
    int index = 0;
    if (ifarg(2)) {
        index = (int) chkarg(2, 0, 1000);
    }
    ((OcBox*) v)->adjust(chkarg(1, -1., 1e5), index);
    ENDGUI
#endif /* HAVE_IV  */
    return 0.;
}

static double full_request(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Box.full_request", v);
#if HAVE_IV
    IFGUI
    OcBox* b = (OcBox*) v;
    if (ifarg(1)) {
        bool x = ((int) chkarg(1, 0, 1) != 0) ? true : false;
        b->full_request(x);
    }
    return (b->full_request() ? 1. : 0.);
    ENDGUI
#endif /* HAVE_IV  */
    return 0.;
}

static double b_size(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Box.size", v);
#if HAVE_IV
    IFGUI
    double* p = hoc_pgetarg(1);  // array for at least 4 numbers
    OcBox* b = (OcBox*) v;
    if (b->has_window()) {
        p[0] = b->window()->save_left();
        p[1] = b->window()->save_bottom();
        p[2] = b->window()->width();
        p[3] = b->window()->height();
    }
    ENDGUI
#endif
    return 0.;
}

const char* pwm_session_filename();

static double save(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Box.save", v);
#if HAVE_IV
    IFGUI
    OcBox* b = (OcBox*) v;
    char buf[256];
    if (hoc_is_object_arg(1)) {
        b->save_action(0, *hoc_objgetarg(1));
        return 1.;
    } else if (ifarg(2)) {
        if (hoc_is_double_arg(2)) {  // return save session file name
            hoc_assign_str(hoc_pgargstr(1), pwm_session_filename());
            return 1.;
        } else {
            Sprintf(buf, "execute(\"%s\", %s)", gargstr(1), gargstr(2));
        }
    } else {
        // Sprintf(buf, "%s", gargstr(1));
        b->save_action(gargstr(1), 0);
        return 1.0;
    }
    b->save_action(buf, 0);
    ENDGUI
    return 1.;
#else
    return 0.;
#endif /* HAVE_IV  */
}

/* help ref
.ref(objectvar)
    solves the problem of keeping an anonymous hoc object which should
    be destroyed when the window it manages is dismissed.
    The box keeps a pointer to the hoc object and the object is
    referenced when the box has a window and unreferenced when the window
    is dismissed.
*/
static double ref(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Box.ref", v);
#if HAVE_IV
    OcBox* b = (OcBox*) v;
    b->keep_ref(*hoc_objgetarg(1));
    return 0.;
#else
    Object* ob = *hoc_objgetarg(1);
    hoc_obj_ref(ob);
    return 0.;
#endif /* HAVE_IV  */
}

/* help dismiss_action
.dismiss_action("action")
    execute the action when the vbox is dismissed from the screen.
*/
static double dismiss_action(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Box.dismiss_action", v);
#if HAVE_IV
    IFGUI
    OcBox* b = (OcBox*) v;
    if (hoc_is_object_arg(1)) {
        b->dismiss_action(0, *hoc_objgetarg(1));
    } else {
        b->dismiss_action(gargstr(1));
    }
    ENDGUI
    return 0.;
#else
    return 0.;
#endif /* HAVE_IV  */
}

static Member_func members[] = {{"intercept", intercept},            // #if HAVE_IV ok
                                {"adjuster", adjuster},              // #if HAVE_IV ok
                                {"adjust", adjust},                  // #if HAVE_IV ok
                                {"full_request", full_request},      // #if HAVE_IV ok
                                {"save", save},                      // #if HAVE_IV ok
                                {"map", map},                        // #if HAVE_IV ok
                                {"unmap", unmap},                    // #if HAVE_IV ok
                                {"ismapped", ismapped},              // #if HAVE_IV ok
                                {"ref", ref},                        // #if HAVE_IV ok
                                {"dismiss_action", dismiss_action},  // #if HAVE_IV ok
                                {"dialog", dialog},                  // #if HAVE_IV ok
                                {"priority", ses_pri},
                                {"size", b_size},
                                {0, 0}};

void HBox_reg() {
    class2oc("HBox", hcons, destruct, members, NULL, NULL, NULL);
}

void VBox_reg() {
    class2oc("VBox", vcons, destruct, members, NULL, NULL, NULL);
}
#if HAVE_IV
OcGlyphContainer::OcGlyphContainer()
    : OcGlyph(NULL) {
    parent_ = NULL;
    recurse_ = false;
}

void OcGlyphContainer::request(Requisition& r) const {
    if (!recurse_) {
        OcGlyphContainer* t = (OcGlyphContainer*) this;
        t->recurse_ = true;
        OcGlyph::request(r);
        Coord w, h;
        w = h = -1.;
        def_size(w, h);
        if (w != -1.) {
            r.x_requirement().natural(w);
            r.y_requirement().natural(h);
        }
        t->recurse_ = false;
    } else {
        hoc_execerror("Box or Deck is recursive. The GUI may no longer work correctly.\n",
                      "Exit program and eliminate the recursion");
    }
}
OcBox::OcBox(int type, int frame, bool scroll)
    : OcGlyphContainer() {
    ScrollBox* sb;
    PolyGlyph* box;
    bi_ = new OcBoxImpl;
    bi_->full_request_ = false;
    bi_->dismissing_ = false;
    bi_->next_map_adjust_ = -1.;
    bi_->ocglyph_list_ = new PolyGlyph();
    bi_->ba_list_ = NULL;
    Resource::ref(bi_->ocglyph_list_);
    bi_->box_ = NULL;
    IFGUI
    WidgetKit& wk = *WidgetKit::instance();
    LayoutKit& lk = *LayoutKit::instance();
    if (type == H) {
        box = bi_->box_ = lk.hbox(3);
    } else {
        if (scroll) {
            bi_->box_ = sb = lk.vscrollbox(10);
            box = lk.hbox(sb, lk.hspace(4), wk.vscroll_bar(sb));
        } else {
            box = bi_->box_ = lk.vbox(3);
        }
    }
    Resource::ref(bi_->box_);

    switch (frame) {
    case INSET:
        body(new Background(wk.inset_frame(lk.variable_span(box)), wk.background()));
        break;
    case OUTSET:
        body(new Background(wk.outset_frame(lk.variable_span(box)), wk.background()));
        break;
    case BRIGHT_INSET:
        body(new Background(wk.bright_inset_frame(lk.variable_span(box)), wk.background()));
        break;
    case FLAT:
        body(new Background(lk.variable_span(box), wk.background()));
        break;
    }
    ENDGUI
    bi_->type_ = type;
    bi_->oc_ref_ = NULL;
    bi_->save_action_ = NULL;
    bi_->save_pyact_ = NULL;
    bi_->o_ = NULL;
    bi_->keep_ref_ = NULL;
    bi_->dis_act_ = NULL;
    bi_->dis_pyact_ = NULL;
}

OcBox::~OcBox() {
    // printf("~OcBox\n");
    GlyphIndex i, cnt = bi_->ocglyph_list_->count();
    for (i = 0; i < cnt; ++i) {
        ((OcGlyph*) (bi_->ocglyph_list_->component(i)))->parents(false);
    }
    Resource::unref(bi_->ocglyph_list_);
    Resource::unref(bi_->box_);
    Resource::unref(bi_->ba_list_);
    hoc_obj_unref(bi_->oc_ref_);
    if (bi_->save_action_) {
        delete bi_->save_action_;
    }
    if (bi_->save_pyact_) {
        hoc_obj_unref(bi_->save_pyact_);
    }
    if (bi_->dis_act_) {
        delete bi_->dis_act_;
    }
    if (bi_->dis_pyact_) {
        hoc_obj_unref(bi_->dis_pyact_);
    }
    assert(!bi_->keep_ref_);
    delete bi_;
}

bool OcBox::full_request() {
    return bi_->full_request_;
}
void OcBox::full_request(bool b) {
    bi_->full_request_ = b;
}
bool OcBox::dismissing() {
    return bi_->dismissing_;
}
void OcBox::dismissing(bool d) {
    bi_->dismissing_ = d;
}

void OcGlyphContainer::intercept(bool b) {
    if (b) {
        parent_ = PrintableWindow::intercept(this);
    } else {
        PrintableWindow::intercept(parent_);
        parent_ = NULL;
    }
}

void OcBox::box_append(OcGlyph* g) {
    WidgetKit& wk = *WidgetKit::instance();
    LayoutKit& lk = *LayoutKit::instance();
    bi_->ocglyph_list_->append(g);
    g->parents(true);
    if (bi_->next_map_adjust_ > 0.) {
        BoxAdjust* ba = new BoxAdjust(this, bi_, g, bi_->next_map_adjust_);
        if (!bi_->ba_list_) {
            bi_->ba_list_ = new PolyGlyph(1);
            bi_->ba_list_->ref();
        }
        bi_->ba_list_->append(ba);
        bi_->box_->append(ba->ga_);
        bi_->box_->append(ba);
        bi_->next_map_adjust_ = -1.;
    } else {
        if (bi_->type_ == V) {
            bi_->box_->append(lk.hflexible(lk.vcenter(g, 1.)));
        } else {
            bi_->box_->append(lk.vflexible(lk.vcenter(g, 1.)));
        }
    }
}

void OcBox::premap() {
    if (bi_->ba_list_) {
        body(new Patch(body()));
    }
}

void OcBox::adjuster(Coord natural) {
    bi_->next_map_adjust_ = natural;
}

void OcBox::adjust(Coord natural, int index) {
    // printf("OcBox::adjust %g %d\n", natural, index);
    if (bi_->ba_list_ && index < bi_->ba_list_->count()) {
        BoxAdjust* ba = (BoxAdjust*) bi_->ba_list_->component(index);
        adjust(natural, ba);
    }
}
void OcBox::adjust(Coord natural, BoxAdjust* ba) {
    ba->fixlay_->span(natural);
    Box::full_request(true);
    bi_->box_->modified(0);
    //	((Patch*)body())->reallocate();
    ((Patch*) body())->redraw();
    Box::full_request(false);
}

NrnFixedLayout::NrnFixedLayout(const DimensionName d, Coord span) {
    dimension_ = d;
    span_ = span;
}

NrnFixedLayout::~NrnFixedLayout() {}

void NrnFixedLayout::request(GlyphIndex, const Requisition*, Requisition& result) {
    Requirement& r = result.requirement(dimension_);
    r.natural(span_);
    r.stretch(0.0);
    r.shrink(0.0);
}

void NrnFixedLayout::allocate(const Allocation&,
                              GlyphIndex,
                              const Requisition*,
                              Allocation* result) {
    Allotment& a = result[0].allotment(dimension_);
    a.span(span_);
}

void NrnFixedLayout::span(Coord s) {
    span_ = s;
}

BoxAdjust::BoxAdjust(OcBox* b, OcBoxImpl* bi, Glyph* g, Coord natural)
    : InputHandler(NULL, WidgetKit::instance()->style()) {
    b_ = b;
    bi_ = bi;
    LayoutKit& lk = *LayoutKit::instance();
    fixlay_ = new NrnFixedLayout(bi->type_ == OcBox::V ? Dimension_Y : Dimension_X, natural);
    ga_ = lk.vcenter(g, 1.);
    if (bi->type_ == OcBox::V) {
        ga_ = lk.hflexible(ga_);
        body(lk.vspace(10));
    } else {
        ga_ = lk.vflexible(ga_);
        body(lk.hspace(10));
    }
    ga_ = new Placement(ga_, fixlay_);
}

BoxAdjust::~BoxAdjust() {}

void BoxAdjust::press(const Event& e) {
    if (fixlay_->vertical()) {
        pstart_ = e.pointer_y();
    } else {
        pstart_ = e.pointer_x();
    }
    fstart_ = fixlay_->span();
}

void BoxAdjust::drag(const Event& e) {
    Coord d;
    if (fixlay_->vertical()) {
        d = e.pointer_y() - pstart_;
        d = fstart_ - d;
    } else {
        d = e.pointer_x() - pstart_;
        d = fstart_ + d;
    }
    if (d < 10.) {
        d = 10.;
    }
    b_->adjust(d, this);
}

void BoxAdjust::release(const Event& e) {
    drag(e);
}

void OcBox::save_action(const char* creat, Object* pyact) {
    if (bi_->o_) {
        // old endl cause great slowness on remote filesystem
        // with gcc version 3.3 20030226 (prerelease) (SuSE Linux)
        //*bi_->o_ << creat << std::endl;
        *bi_->o_ << creat << "\n";
    } else {
        if (pyact) {
            bi_->save_pyact_ = pyact;
            hoc_obj_ref(pyact);
        } else {
            bi_->save_action_ = new CopyString(creat);
        }
    }
}

void OcBox::dismiss_action(const char* act, Object* pyact) {
    if (pyact) {
        hoc_obj_ref(pyact);
        bi_->dis_pyact_ = pyact;
        if (bi_->dis_act_) {
            delete bi_->dis_act_;
            bi_->dis_act_ = NULL;
        }
    } else if (act) {
        if (bi_->dis_pyact_) {
            hoc_obj_unref(bi_->dis_pyact_);
            bi_->dis_pyact_ = NULL;
        }
        if (bi_->dis_act_) {
            *bi_->dis_act_ = act;
        } else {
            bi_->dis_act_ = new CopyString(act);
        }
    }
    if ((bi_->dis_act_ || bi_->dis_pyact_) && has_window()) {
        window()->replace_dismiss_action(
            new BoxDismiss(window(), bi_->dis_act_, this, bi_->dis_pyact_));
    }
}

void OcBox::save(std::ostream& o) {
    char buf[256];
    if (bi_->save_action_ || bi_->save_pyact_) {
        if (bi_->save_action_ && strcmp(bi_->save_action_->string(), "") == 0) {
            return;
        }
        if (has_window()) {
            Sprintf(buf, "\n//Begin %s", window()->name());
            o << buf << std::endl;
        }
        o << "{" << std::endl;
        bi_->o_ = &o;
        if (bi_->save_pyact_) {
            HocCommand hc(bi_->save_pyact_);
            hc.execute();
        } else {
            HocCommand hc(bi_->save_action_->string(), bi_->keep_ref_);
            hc.execute();
        }
        bi_->o_ = NULL;
    } else {
        if (bi_->type_ == H) {
            o << "{\nocbox_ = new HBox()" << std::endl;
        } else {
            o << "{\nocbox_ = new VBox()" << std::endl;
        }
        o << "ocbox_list_.prepend(ocbox_)" << std::endl;
        o << "ocbox_.intercept(1)\n}" << std::endl;
        long i, cnt = bi_->ocglyph_list_->count();
        for (i = 0; i < cnt; ++i) {
            ((OcGlyph*) bi_->ocglyph_list_->component(i))->save(o);
        }
        o << "{\nocbox_ = ocbox_list_.object(0)" << std::endl;
        o << "ocbox_.intercept(0)" << std::endl;
    }
    if (has_window()) {
#if defined(WIN32)
        const char* cp1;
        if (strchr(window()->name(), '"')) {
            cp1 = "Neuron";
        } else {
            cp1 = window()->name();
        }
        Sprintf(buf,
                "ocbox_.map(\"%s\", %g, %g, %g, %g)\n}",
                cp1,
#else
        Sprintf(buf,
                "ocbox_.map(\"%s\", %g, %g, %g, %g)\n}",
                window()->name(),
#endif
                window()->save_left(),
                window()->save_bottom(),
                window()->width(),
                window()->height());
        o << buf << std::endl;
    } else {
        o << "ocbox_.map()\n}" << std::endl;
    }
    if (bi_->oc_ref_) {
        Sprintf(buf, "%s = ocbox_", hoc_object_pathname(bi_->oc_ref_));
        o << buf << std::endl;
        o << "ocbox_list_.remove(0)" << std::endl;
    }
    o << "objref ocbox_" << std::endl;
    if (bi_->save_action_ && has_window()) {
        Sprintf(buf, "//End %s\n", window()->name());
        o << buf << std::endl;
    }
}


void OcBox::no_parents() {
    // printf("OcBox::no_parents()\n");
    if (bi_->keep_ref_) {
        // printf("OcBox::no_parents unreffing %s\n", hoc_object_name(bi_->keep_ref_));
        hoc_obj_unref(bi_->keep_ref_);
        bi_->keep_ref_ = NULL;
    }
}

void OcBox::keep_ref(Object* ob) {
    hoc_obj_ref(ob);
    if (bi_->keep_ref_) {
        hoc_obj_unref(bi_->keep_ref_);
    }
    bi_->keep_ref_ = ob;
}

Object* OcBox::keep_ref() {
    return bi_->keep_ref_;
}

#endif /* HAVE_IV */
