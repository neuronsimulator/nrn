#include <../../nrnconf.h>
#include "classreg.h"
#include "gui-redirect.h"
#include "ocnotify.h"

#if HAVE_IV

#include <InterViews/handler.h>
#include <InterViews/action.h>
#include <InterViews/style.h>
#include <InterViews/color.h>
#include <InterViews/polyglyph.h>
#include <IV-look/kit.h>
#include <InterViews/layout.h>
#include <InterViews/background.h>
#include <OS/math.h>

#include <stdio.h>
#include "scenepic.h"
#endif
#include "shapeplt.h"
#if HAVE_IV
#include "graph.h"
#include "ivoc.h"
#include "nrnoc2iv.h"
#include "rubband.h"
#include "symchoos.h"
#include "symdir.h"
#include "parse.hpp"
#include "utility.h"
#include "objcmd.h"
#include "idraw.h"

#define SelectVariable_ "PlotWhat PlotShape"
#define VariableScale_  "VariableScale PlotShape"
#define TimePlot_       "TimePlot PlotShape"
#define SpacePlot_      "SpacePlot PlotShape"
#define ShapePlot_      "ShapePlot PlotShape"
#define MoveText_       "MoveText PlotShape"

extern Symlist* hoc_built_in_symlist;
#endif  // HAVE_IV

extern int hoc_return_type_code;

void* (*nrnpy_get_pyobj)(Object* obj) = 0;
void (*nrnpy_decref)(void* pyobj) = 0;

// PlotShape class registration for oc
static double sh_flush(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("PlotShape.flush", v);
#if HAVE_IV
    IFGUI((ShapePlot*) v)->flush();
    ENDGUI
#endif
    return 1.;
}

static double fast_flush(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("PlotShape.fast_flush", v);
#if HAVE_IV
    IFGUI((ShapePlot*) v)->fast_flush();
    ENDGUI
#endif
    return 1.;
}

static double sh_begin(void* v) {  // a noop. Exists only because graphs and
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("PlotShape.begin", v);
    return 1.;  // shapes are often in same list
}

static double sh_scale(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("PlotShape.scale", v);
#if HAVE_IV
    IFGUI((ShapePlot*) v)->scale(float(*getarg(1)), float(*getarg(2)));
}
else {
    ((ShapePlotData*) v)->scale(float(*getarg(1)), float(*getarg(2)));
    ENDGUI
#else
    ((ShapePlotData*) v)->scale(float(*getarg(1)), float(*getarg(2)));
#endif
    return 1.;
}

#if HAVE_IV
void ShapePlot::has_iv_view(bool value) {
    has_iv_view_ = value;
}
#endif

static double sh_view(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("PlotShape.view", v);
#if HAVE_IV
    IFGUI
    ShapePlot* sh = (ShapePlot*) v;
    sh->has_iv_view(true);
    if (sh->varobj()) {
        hoc_execerror("InterViews only supports string variables.", NULL);
    }
    if (ifarg(8)) {
        Coord x[8];
        int i;
        for (i = 0; i < 8; ++i) {
            x[i] = *getarg(i + 1);
        }
        sh->view(x);
    }
    ENDGUI
#endif
    return 1.;
}

static double sh_variable(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("PlotShape.variable", v);
    ShapePlotData* spd = (ShapePlotData*) v;
#if HAVE_IV
    // NOTE: only sh OR spd is valid, but both may be safely set
    ShapePlot* sh = (ShapePlot*) v;
#endif
    if (hoc_is_object_arg(1) && nrnpy_get_pyobj) {
        // note that we only get inside here when Python is available
        void* py_var_ = nrnpy_get_pyobj(*hoc_objgetarg(1));
        if (!py_var_) {
            hoc_execerror("variable must be either a string or Python object", NULL);
        }
#if HAVE_IV
        IFGUI
        if (sh->has_iv_view()) {
            nrnpy_decref(py_var_);
            hoc_execerror("InterViews only supports string variables.", NULL);
        }
        nrnpy_decref(sh->varobj());
        sh->varobj(py_var_);
        return 1;
        ENDGUI
#endif
        nrnpy_decref(spd->varobj());
        spd->varobj(py_var_);
        return 1;
    }
    Symbol* s;
    s = hoc_table_lookup(gargstr(1), hoc_built_in_symlist);
    if (s) {
#if HAVE_IV
        IFGUI
        if (nrnpy_decref) {
            nrnpy_decref(sh->varobj());
        }

        sh->varobj(NULL);
        sh->variable(s);
    } else {
        if (nrnpy_decref) {
            nrnpy_decref(spd->varobj());
        }
        spd->varobj(NULL);
        spd->variable(s);
        ENDGUI
#else
        if (nrnpy_decref) {
            nrnpy_decref(spd->varobj());
        }
        spd->varobj(NULL);
        spd->variable(s);
#endif
    }
    return 1.;
}

static double sh_view_count(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("PlotShape.view_count", v);
    int n = 0;
#if HAVE_IV
    IFGUI
    n = ((ShapeScene*) v)->view_count();
    ENDGUI
#endif
    return double(n);
}

static double sh_save_name(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("PlotShape.save_name", v);
#if HAVE_IV
    IFGUI((ShapeScene*) v)->name(gargstr(1));
    ENDGUI
#endif
    return 1.;
}

static double sh_unmap(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("PlotShape.unmap", v);
#if HAVE_IV
    IFGUI
    ShapeScene* s = (ShapeScene*) v;
    s->dismiss();
    ENDGUI
#endif
    return 0.;
}

static double sh_printfile(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("PlotShape.printfile", v);
#if HAVE_IV
    IFGUI
    ShapeScene* s = (ShapeScene*) v;
    s->printfile(gargstr(1));
    ENDGUI
#endif
    return 1.;
}

static double sh_show(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("PlotShape.show", v);
    hoc_return_type_code = 1;
#if HAVE_IV
    IFGUI
    ShapeScene* s = (ShapeScene*) v;
    if (ifarg(1)) {
        s->shape_type(int(chkarg(1, 0., 2.)));
    } else {
        return s->shape_type();
    }
}
else {
    if (ifarg(1)) {
        ((ShapePlotData*) v)->set_mode(int(chkarg(1, 0., 2.)));
    } else {
        return ((ShapePlotData*) v)->get_mode();
    }
    ENDGUI
#else
    if (ifarg(1)) {
        ((ShapePlotData*) v)->set_mode(int(chkarg(1, 0., 2.)));
    } else {
        return ((ShapePlotData*) v)->get_mode();
    }
#endif
    return 1.;
}

extern double ivoc_gr_menu_action(void* v);

static double s_colormap(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("PlotShape.colormap", v);
#if HAVE_IV
    IFGUI
    ShapePlot* s = (ShapePlot*) v;
    if (ifarg(4)) {
        s->color_value()->colormap(int(chkarg(1, 0, 255)),
                                   int(chkarg(2, 0, 255)),
                                   int(chkarg(3, 0, 255)),
                                   int(chkarg(4, 0, 255)));
    } else {
        bool b = false;
        if (ifarg(2)) {
            b = (bool) chkarg(2, 0, 1);
        }
        s->color_value()->colormap(int(chkarg(1, 0, 1000)), b);
    }
    ENDGUI
#endif
    return 1.;
}

static double sh_hinton(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("PlotShape.hinton", v);
#if HAVE_IV
    IFGUI
    ShapeScene* ss = (ShapeScene*) v;
    neuron::container::data_handle<double> pd = hoc_hgetarg<double>(1);
    double xsize = chkarg(4, 1e-9, 1e9);
    double ysize = xsize;
    if (ifarg(5)) {
        ysize = chkarg(5, 1e-9, 1e9);
    }
    Hinton* h = new Hinton(pd, xsize, ysize, ss);
    ss->append(new FastGraphItem(h));
    ss->move(ss->count() - 1, *getarg(2), *getarg(3));
    ENDGUI
#endif
    return 1.;
}

static double exec_menu(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("PlotShape.exec_menu", v);
#if HAVE_IV
    IFGUI((Scene*) v)->picker()->exec_item(gargstr(1));
    ENDGUI
#endif
    return 0.;
}

extern double ivoc_gr_menu_tool(void*);
extern double ivoc_gr_mark(void*);
extern double ivoc_gr_size(void*);
extern double ivoc_gr_label(void*);
extern double ivoc_gr_line(void*);
extern double ivoc_gr_begin_line(void*);
extern double ivoc_gr_erase(void*);
extern double ivoc_gr_gif(void*);
extern double ivoc_erase_all(void*);
extern double nrniv_sh_observe(void*);
extern double nrniv_sh_rotate(void*);
extern double nrniv_sh_nearest(void*);
extern double nrniv_sh_push(void*);
extern double nrniv_sh_color(void*);
extern double nrniv_sh_color_all(void*);
extern double nrniv_sh_color_list(void*);
extern double nrniv_len_scale(void*);
extern Object** nrniv_sh_nearest_seg(void*);
extern Object** nrniv_sh_selected_seg(void*);

static Member_func sh_members[] = {{"hinton", sh_hinton},
                                   {"nearest", nrniv_sh_nearest},
                                   {"push_selected", nrniv_sh_push},
                                   {"scale", sh_scale},
                                   {"view", sh_view},
                                   {"size", ivoc_gr_size},
                                   {"view_count", sh_view_count},
                                   {"flush", sh_flush},
                                   {"fastflush", fast_flush},
                                   {"begin", sh_begin},
                                   {"variable", sh_variable},
                                   {"save_name", sh_save_name},
                                   {"unmap", sh_unmap},
                                   {"color", nrniv_sh_color},
                                   {"color_all", nrniv_sh_color_all},
                                   {"color_list", nrniv_sh_color_list},
                                   {"printfile", sh_printfile},
                                   {"show", sh_show},
                                   {"menu_action", ivoc_gr_menu_action},
                                   {"menu_tool", ivoc_gr_menu_tool},
                                   {"colormap", s_colormap},
                                   {"exec_menu", exec_menu},
                                   {"observe", nrniv_sh_observe},
                                   {"rotate", nrniv_sh_rotate},
                                   {"beginline", ivoc_gr_begin_line},
                                   {"line", ivoc_gr_line},
                                   {"label", ivoc_gr_label},
                                   {"mark", ivoc_gr_mark},
                                   {"erase", ivoc_gr_erase},
                                   {"erase_all", ivoc_erase_all},
                                   {"len_scale", nrniv_len_scale},
                                   {"gif", ivoc_gr_gif},
                                   {0, 0}};

static Member_ret_obj_func retobj_members[] = {{"nearest_seg", nrniv_sh_nearest_seg},
                                               {"selected_seg", nrniv_sh_selected_seg},
                                               {NULL, NULL}};

static void* sh_cons(Object* ho) {
    TRY_GUI_REDIRECT_OBJ("PlotShape", NULL);
    int i = 1;
    int iarg = 1;
    SectionList* sl = NULL;
    Object* ob = NULL;
    // first arg may be an object.
    if (ifarg(iarg)) {
        if (hoc_is_object_arg(iarg)) {
            ob = *hoc_objgetarg(iarg);
            check_obj_type(ob, "SectionList");
#if HAVE_IV
            IFGUI
            sl = new SectionList(ob);
            sl->ref();
            ENDGUI
#endif
            ++iarg;
        }
    }
    if (ifarg(iarg)) {
        i = int(chkarg(iarg, 0, 1));
    }
#if HAVE_IV
    IFGUI
    ShapePlot* sh = NULL;
    sh = new ShapePlot(NULL, sl);
    sh->has_iv_view(i ? true : false);
    sh->varobj(NULL);
    Resource::unref(sl);
    sh->ref();
    sh->hoc_obj_ptr(ho);
    if (i) {
        sh->view(200);
    }
    return (void*) sh;
}
else {
    ShapePlotData* spd = new ShapePlotData(NULL, ob);
    return (void*) spd;
    ENDGUI
#else
    ShapePlotData* sh = new ShapePlotData(NULL, ob);
    return (void*) sh;
#endif
}

static void sh_destruct(void* v) {
    TRY_GUI_REDIRECT_NO_RETURN("~PlotShape", v);
#if HAVE_IV
    IFGUI
    if (nrnpy_decref) {
        ShapePlot* sp = (ShapePlot*) v;
        nrnpy_decref(sp->varobj());
    }

    ((ShapeScene*) v)->dismiss();
}
else {
    if (nrnpy_decref) {
        ShapePlotData* sp = (ShapePlotData*) v;
        nrnpy_decref(sp->varobj());
    }
    ENDGUI
    Resource::unref((ShapeScene*) v);
#else
    if (nrnpy_decref) {
        ShapePlotData* sp = (ShapePlotData*) v;
        nrnpy_decref(sp->varobj());
    }

#endif
}
void PlotShape_reg() {
    //	printf("PlotShape_reg\n");
    class2oc("PlotShape", sh_cons, sh_destruct, sh_members, NULL, retobj_members, NULL);
}

void* ShapePlotData::varobj() const {
    return py_var_;
}

void ShapePlotData::varobj(void* obj) {
    py_var_ = obj;
}

#if HAVE_IV

/* static */ class ShapePlotImpl: public Observer {
  public:
    ShapePlotImpl(ShapePlot*, Symbol*);
    ~ShapePlotImpl();
    virtual void time();
    virtual void space();
    virtual void shape();
    virtual void show_shape_val(bool);
    virtual void select_variable();
    virtual void scale();
    virtual void colorbar();
    virtual void update(Observable*);

  public:
    ShapePlot* sp_;
    Symbol* sym_;
    GLabel* variable_;
    double graphid_;
    int colorid_;
    SectionHandler* time_sh_;
    bool showing_;
    Glyph* colorbar_;
    bool fast_;
};

/* static */ class MakeTimePlot: public SectionHandler {
  public:
    MakeTimePlot(ShapePlotImpl*);
    virtual ~MakeTimePlot();
    virtual bool event(Event&);

  private:
    ShapePlotImpl* spi_;
};

/* static */ class MakeSpacePlot: public RubberAction {
  public:
    MakeSpacePlot(ShapePlotImpl*);
    virtual ~MakeSpacePlot();
    virtual void execute(Rubberband*);

  private:
    ShapePlotImpl* spi_;
};

declareActionCallback(ShapePlotImpl);
implementActionCallback(ShapePlotImpl);

ShapePlot::ShapePlot(Symbol* sym, SectionList* sl)
    : ShapeScene(sl) {
    py_var_ = NULL;
    if (sl) {
        sl_ = sl->nrn_object();
    } else {
        sl_ = NULL;
    }
    if (sl_)
        ++sl_->refcount;
    spi_ = new ShapePlotImpl(this, sym);
    variable(spi_->sym_);
    picker()->add_menu("Plot What?",
                       new ActionCallback(ShapePlotImpl)(spi_, &ShapePlotImpl::select_variable));
    picker()->add_menu("Variable scale",
                       new ActionCallback(ShapePlotImpl)(spi_, &ShapePlotImpl::scale));
    picker()->add_radio_menu("Time Plot",
                             new ActionCallback(ShapePlotImpl)(spi_, &ShapePlotImpl::time));
    picker()->add_radio_menu("Space Plot",
                             new ActionCallback(ShapePlotImpl)(spi_, &ShapePlotImpl::space));
    picker()->add_radio_menu("Shape Plot",
                             new ActionCallback(ShapePlotImpl)(spi_, &ShapePlotImpl::shape));
    color_value()->attach(spi_);
    spi_->colorbar();
}
ShapePlot::~ShapePlot() {
    if (sl_)
        hoc_dec_refcount(&sl_);
    color_value()->detach(spi_);
    delete spi_;
}


float ShapePlot::low() {
    return color_value()->low();
}
float ShapePlot::high() {
    return color_value()->high();
}

Object* ShapePlot::neuron_section_list() {
    return sl_;
}

void ShapePlot::observe(SectionList* sl) {
    //	printf("ShapePlot::observe\n");
    if (sl_)
        hoc_dec_refcount(&sl_);
    sl_ = sl ? sl->nrn_object() : NULL;
    if (sl_)
        ++sl_->refcount;
    ShapeScene::observe(sl);
    if (spi_->showing_) {
        PolyGlyph* pg = shape_section_list();
        GlyphIndex i, cnt = pg->count();
        for (i = 0; i < cnt; ++i) {
            ShapeSection* ss = (ShapeSection*) pg->component(i);
            ss->set_range_variable(spi_->sym_);
        }
        damage_all();
    }
}

void ShapePlot::erase_all() {
    Resource::unref(spi_->colorbar_);
    spi_->colorbar_ = NULL;
    ShapeScene::erase_all();
}
void ShapePlotImpl::update(Observable*) {
    colorbar();
    sp_->damage_all();
}

void ShapePlot::variable(Symbol* sym) {
    GlyphIndex i;
    spi_->sym_ = sym;
    i = glyph_index(spi_->variable_);
    GLabel* g = new GLabel(spi_->sym_->name, colors->color(1), 1, 1, .5, .5);
    if (i >= 0) {
        damage(i);
        replace(i, new GraphItem(g, 0));
        damage(i);
    } else {
        append_fixed(new GraphItem(g, 0));
    }
    Resource::unref(spi_->variable_);
    Resource::ref(g);
    spi_->variable_ = g;
    if (spi_->showing_) {
        spi_->showing_ = false;
        spi_->show_shape_val(true);
    }
    scale(-80, 40);
}
const char* ShapePlot::varname() const {
    return spi_->sym_->name;
}

void* ShapePlot::varobj() const {
    return py_var_;
}

void ShapePlot::varobj(void* obj) {
    py_var_ = obj;
}


void ShapePlot::scale(float min, float max) {
    color_value()->set_scale(min, max);
}


void ShapePlot::save_phase1(std::ostream& o) {
    o << "{" << std::endl;
    save_class(o, "PlotShape");
    char buf[256];
    Sprintf(buf, "save_window_.variable(\"%s\")", spi_->sym_->name);
    o << buf << std::endl;
}

void ShapePlot::shape_plot() {}
void ShapePlot::make_time_plot(Section*, float) {}
void ShapePlot::make_space_plot(Section*, float, Section*, float) {}
void ShapePlot::flush() {
    spi_->fast_ = false;
    if (tool() == SHAPE) {
        damage_all();
    }
}
void ShapePlot::fast_flush() {
    if (tool() == SHAPE) {
        int i, cnt = view_count();
        spi_->fast_ = true;
        // fast for only one view
        for (i = 0; i < cnt; ++i) {
            XYView* v = sceneview(i);
            Coord x = v->left(), y = v->bottom();
#if defined(WIN32)
            // if x,y is not on the screen then use right, top. Otherwise InvalidateRect
            // will not take effect.
            Window* w = v->canvas() ? v->canvas()->window() : NULL;
            if (w) {
                if (w->left() < 0) {
                    x = v->right();
                }
                if (w->bottom() < 0) {
                    y = v->top();
                }
            }
#endif
            v->damage(x, y, x, y);
        }
    }
}
#if defined(WIN32)
extern void* mswin_setclip(Canvas*, int, int, int, int);
extern void mswin_delete_object(void*);
#endif

void ShapePlot::draw(Canvas* c, const Allocation& a) const {
    if (spi_->fast_) {
#if defined(WIN32)
        // win32 clipping is much more strict than X11 clipping even though the
        // implementations seem to agree that clipping is the intersection of
        // all clip requests on the clip stack in canvas. Clipping is originally
        // set to the damage area and thus nothing actually gets drawn to
        // the buffer. Furthermore, when copying from buffer to screen, only
        // the original damage area is copied. For some reason in X11 neither
        // seems to prevent the correct appearance although I wonder if it
        // will work on all x11 implementations. However, for WIN32, both cause
        // problems and thus the implementation of Canvas16::clip was changed to
        // not do an intersection and the "damage_all" below along with a change
        // to the window.cpp paint function in which the damage area is reread before
        // the bit block transfer solves the problem.
        // At any rate this is fast only for the first view since the fast_draw's
        // only work once.

        XYView* v = XYView::current_draw_view();
        c->push_clipping(true);
#if defined(WIN32)
        // Consider the commit message:
        // -------
        // 28)  Branch: MAIN Date: 2004-05-21 06:30 Commit by: hines
        // canvas16.cpp
        // MSWin PolyGlyphs inside a ScrollBox occasionally do not repair all the damage
        // This is apparently a long standing problem dating from 1999 which just
        // now became very noticable with the MultipleRunFitter Parameter panel
        // when there are many parameters (so that it is a scrollbox). This has
        // been traced to a #if 0 fragment in the clipping mechanism which turned
        // off the intersection calculation when a new clipping region was requested
        // We have turned this section back on. There was no reason mentioned in
        // the 1999 log message when the #if 0 was introduced. The major reason for
        // the larger change in that file was drawing dashed brush lines. If a
        // problem with clipping exists then it will have to be fixed in another way.
        // -------
        // Well, here is the reason for the #if 0. Without it, the fast_flush movie
        // is not drawn for shapeplots. (Strangely, the neurondemo movie works for
        // the stylized neuron but not the pyramidal cell.
        // So as not to ruin the MAIN 28 commit of canvas16.cpp
        // we force the clipping to the entire canvas for the fast_flush.
        // Feeble attempts to do everything here, foundered on my inability to
        // include <windows.h> in this file. So the implementation is placed in
        // ivocwin.cpp. Note that the clip region must be deleted after use.
        //
        void* new_clip = mswin_setclip(c, 0, 0, c->pwidth(), c->pheight());
#endif
#endif
        GlyphIndex i, cnt = count();
        for (i = 0; i < cnt; ++i) {
            GraphItem* gi = (GraphItem*) component(i);
            if (gi->is_fast()) {
                Coord x, y;
                location(i, x, y);
                ((FastShape*) (gi->body()))->fast_draw(c, x, y, false);
            }
        }
#if defined(WIN32)
        c->pop_clipping();
        mswin_delete_object(new_clip);
        v->damage_all();
#endif
        spi_->fast_ = false;
    } else {
        ShapeScene::draw(c, a);
    }
}

ShapePlotImpl::ShapePlotImpl(ShapePlot* sp, Symbol* sym) {
    sp_ = sp;
    colorid_ = 0;
    graphid_ = 0;
    showing_ = false;
    fast_ = false;
    colorbar_ = NULL;
    if (sym) {
        sym_ = sym;
    } else {
        sym_ = hoc_table_lookup("v", hoc_built_in_symlist);
    }
    variable_ = NULL;
    time_sh_ = new MakeTimePlot(this);
    time_sh_->ref();
}

ShapePlotImpl::~ShapePlotImpl() {
    Resource::unref(variable_);
    time_sh_->unref();
    Resource::unref(colorbar_);
}

void ShapePlotImpl::colorbar() {
    bool showing;
    if (colorbar_) {
        int i = sp_->glyph_index(colorbar_);
        Resource::unref(colorbar_);
        showing = sp_->showing(i);
        sp_->remove(i);
    } else {
        showing = false;
    }
    colorbar_ = sp_->color_value()->make_glyph();
    colorbar_->ref();
    sp_->append_fixed(new GraphItem(colorbar_, 0));
    sp_->show(sp_->count() - 1, showing);
    if (showing) {
        XYView* v = XYView::current_pick_view();
        sp_->move(sp_->count() - 1, v->left(), v->top());
    }
}

void ShapePlotImpl::select_variable() {
    if (Oc::helpmode()) {
        Oc::help(SelectVariable_);
    }
    SymChooser* sc;
    Oc oc;
    Style* style = new Style(Session::instance()->style());
    style->attribute("caption", "Variable in the shape domain");
    sc = new SymChooser(new SymDirectory(RANGEVAR), WidgetKit::instance(), style, NULL, 1);
    sc->ref();
    while (sc->post_for(XYView::current_pick_view()->canvas()->window())) {
        Symbol* s;
        s = hoc_table_lookup(sc->selected().c_str(), hoc_built_in_symlist);
        if (s) {
            sp_->variable(s);
            break;
        }
    }
    sc->unref();
}

void ShapePlotImpl::scale() {
    if (Oc::helpmode()) {
        Oc::help(VariableScale_);
    }
    Coord x, y;
    x = sp_->color_value()->low();
    y = sp_->color_value()->high();
    if (var_pair_chooser(
            "Variable range scale", x, y, XYView::current_pick_view()->canvas()->window())) {
        sp_->scale(x, y);
    }
}

void ShapePlotImpl::time() {
    if (Oc::helpmode()) {
        Oc::help(TimePlot_);
        return;
    }
    sp_->tool(ShapePlot::TIME);
    graphid_ = 0;
    colorid_ = 2;
    sp_->color(colors->color(1));
    sp_->section_handler(time_sh_);
    show_shape_val(false);
    sp_->picker()->bind_select((OcHandler*) NULL);
}

void ShapePlotImpl::space() {
    if (Oc::helpmode()) {
        Oc::help(SpacePlot_);
        return;
    }
    sp_->tool(ShapePlot::SPACE);
    graphid_ = 0;
    colorid_ = 1;
    sp_->color(colors->color(1));
    sp_->section_handler((SectionHandler*) NULL);
    show_shape_val(false);
    sp_->picker()->bind_select(new RubberLine(new MakeSpacePlot(this)));
}

void ShapePlotImpl::shape() {
    if (Oc::helpmode()) {
        Oc::help(ShapePlot_);
        return;
    }
    sp_->tool(ShapePlot::SHAPE);
    // printf("shape\n");
    sp_->section_handler((SectionHandler*) NULL);
    sp_->picker()->bind_select((OcHandler*) NULL);
    show_shape_val(true);
}

void ShapePlotImpl::show_shape_val(bool show) {
    if (show != showing_) {
        PolyGlyph* pg = sp_->shape_section_list();
        GlyphIndex i, cnt = pg->count();
        if (show) {
            for (i = 0; i < cnt; ++i) {
                ShapeSection* ss = (ShapeSection*) pg->component(i);
                ss->set_range_variable(sym_);
            }
        } else {
            for (i = 0; i < cnt; ++i) {
                ShapeSection* ss = (ShapeSection*) pg->component(i);
                ss->clear_variable();
            }
        }
        if (colorbar_) {
            int i = sp_->glyph_index(colorbar_);
            sp_->show(i, show);
            if (show) {
                XYView* v = XYView::current_pick_view();
                sp_->move(i, v->left(), v->top());
            }
        }
        sp_->damage_all();
        showing_ = show;
    }
}

MakeTimePlot::MakeTimePlot(ShapePlotImpl* spi) {
    spi_ = spi;
}
MakeTimePlot::~MakeTimePlot() {}

bool MakeTimePlot::event(Event&) {
    Oc oc;
    ShapeSection* ss = shape_section();
    Section* sec = ss->section();
    if (spi_->sp_->tool() != ShapePlot::TIME) {
        return false;
    }
    if (spi_->graphid_ == 0) {
        oc.run("newPlotV()\n");
        oc.run("hoc_ac_ = object_id(graphItem)\n");
        spi_->graphid_ = hoc_ac_;
    }
    oc.run("hoc_ac_ = object_id(graphItem)\n");
    // printf("graphid_=%g hoc_ac_=%g\n", graphid_, hoc_ac_);
    float x = spi_->sp_->arc_selected();
    x = nrn_arc_position(sec, node_exact(sec, x));
    x = (nrn_section_orientation(sec) == 0.) ? x : 1. - x;
    if (spi_->graphid_ == hoc_ac_) {
        char buf[200];
        Sprintf(buf, "{graphItem.color(%d)}\n", spi_->colorid_);
        oc.run(buf);
        Sprintf(buf,
                "{graphItem.addvar(\"%s.%s(%g)\")}\n",
                hoc_section_pathname(sec),
                spi_->sp_->varname(),
                x);
        oc.run(buf);
        ss->setColor(colors->color(spi_->colorid_), ShapeScene::current_pick_scene());
        ++spi_->colorid_;
    } else {
        spi_->graphid_ = 0;
    }
    return true;
}

MakeSpacePlot::MakeSpacePlot(ShapePlotImpl* spi) {
    spi_ = spi;
}
MakeSpacePlot::~MakeSpacePlot() {}

void MakeSpacePlot::execute(Rubberband* rb) {
    ShapePlot* sp = spi_->sp_;
    RubberLine* rl = (RubberLine*) rb;
    Coord x1, y1, x2, y2;
    float a1, a2;
    rl->get_line(x1, y1, x2, y2);
    // printf("MakeSpacePlot %g %g %g %g\n", x1, y1, x2, y2);
    char buf[256];
    Oc oc;
    oc.run("objectvar rvp_\n");
    Section *sec1, *sec2;
    sp->nearest(x1, y1);
    sec1 = sp->selected()->section();
    a1 = sp->arc_selected();
    a1 = (a1 < .5) ? 0. : 1.;
    sp->nearest(x2, y2);
    sec2 = sp->selected()->section();
    a2 = sp->arc_selected();
    a2 = (a2 < .5) ? 0. : 1.;
    if (sec1 == sec2 && a1 == a2) {
        printf("Null path for space plot: ignored\n");
        return;
    }
    oc.run("hoc_ac_ = object_id(graphItem)\n");
    // printf("graphid_=%g hoc_ac_=%g\n", graphid_, hoc_ac_);
    if (spi_->graphid_ == 0. || spi_->graphid_ != hoc_ac_) {
        oc.run("graphItem = new Graph()\n");
        oc.run("hoc_ac_ = object_id(graphItem)\n");
        spi_->graphid_ = hoc_ac_;
        oc.run("{graphItem.save_name(\"flush_list.\")}\n");
        oc.run("{flush_list.append(graphItem)}\n");
        spi_->colorid_ = 1;
    }
    ++spi_->colorid_;
    ColorValue* cv = sp->color_value();
    Sprintf(buf, "rvp_ = new RangeVarPlot(\"%s\")\n", sp->varname());
    oc.run(buf);
    Sprintf(buf, "%s rvp_.begin(%g)\n", hoc_section_pathname(sec1), a1);
    oc.run(buf);
    Sprintf(buf, "%s rvp_.end(%g)\n", hoc_section_pathname(sec2), a2);
    oc.run(buf);
    oc.run("{rvp_.origin(rvp_.d2root)}\n");
    Sprintf(buf, "{graphItem.size(rvp_.left(), rvp_.right(), %g, %g)}\n", cv->low(), cv->high());
    oc.run(buf);
    Sprintf(buf, "{graphItem.addobject(rvp_, %d, 1) graphItem.yaxis()}\n", spi_->colorid_);
    oc.run(buf);
    sp->color(sec1, sec2, colors->color(spi_->colorid_));
}


static const Color* gray;
static int csize;
static const Color** crange;

static int spec[] = {95,  0,   95, 111, 0,   111, 127, 0,   143, 143, 0,   127, 159, 0,   111,
                     175, 0,   95, 191, 0,   79,  207, 0,   63,  207, 31,  47,  223, 47,  47,
                     239, 63,  31, 255, 79,  15,  255, 95,  7,   255, 111, 0,   255, 127, 0,
                     255, 143, 0,  255, 159, 0,   255, 175, 0,   255, 191, 0,   255, 207, 0,
                     255, 223, 0,  255, 239, 0,   255, 247, 0,   255, 255, 0,   -1};


ColorValue::ColorValue() {
    if (!gray) {
        Style* s = Session::instance()->style();
        CopyString name;
        csize = 0;
        if (s->find_attribute("shape_scale_file", name)) {
            name = expand_env_var(name.string());
            // printf("ColorValue %s\n", name.string());
            FILE* f;
            if ((f = fopen(name.string(), "r")) == 0) {
#ifdef WIN32
#else
                printf("Cannot open %s: Using built-in colormap for shapeplot\n", name.string());
#endif
            } else {
                int r, g, b;
                while (fscanf(f, "%d %d %d", &r, &g, &b) == 3) {
                    ++csize;
                }
                if (csize) {
                    crange = new const Color*[csize];
                    rewind(f);
                    csize = 0;
                    while (fscanf(f, "%d %d %d", &r, &g, &b) == 3) {
                        crange[csize] = new Color(ColorIntensity(r / 256.),
                                                  ColorIntensity(g / 256.),
                                                  ColorIntensity(b / 256.));
                        Resource::ref(crange[csize]);
                        ++csize;
                    }
                }
                fclose(f);
            }
        }
        if (csize == 0) {
            for (csize = 0; spec[csize * 3] != -1; csize++) {
            }
            crange = new const Color*[csize];
            for (csize = 0; spec[csize * 3] != -1; csize++) {
                crange[csize] = new Color(ColorIntensity(spec[csize * 3] / 256.),
                                          ColorIntensity(spec[csize * 3 + 1] / 256.),
                                          ColorIntensity(spec[csize * 3 + 2] / 256.));
                Resource::ref(crange[csize]);
            }
        }
        gray = Color::lookup(Session::instance()->default_display(), "gray");
        Resource::ref(gray);
    }
    csize_ = 0;
    crange_ = NULL;
    set_scale(0, 1);
}

ColorValue::~ColorValue() {
    int i;
    if (csize_) {  // delete the local
        for (i = 0; i < csize_; ++i) {
            crange_[i]->unref();
        }
        delete[] crange_;
    }
}

void ColorValue::set_scale(float low, float high) {
    if (low < high) {
        low_ = low;
        high_ = high;
    }
    notify();
}

const Color* ColorValue::get_color(float val) const {
    float x = (val - low_) / (high_ - low_);
    if (csize_) {
        if (x > .99)
            return crange_[csize_ - 1];
        else if (x < 0)
            return crange_[0];
        else
            return crange_[int(csize_ * x)];
    } else {
        if (x > .99)
            return crange[csize - 1];
        else if (x < 0)
            return crange[0];
        else
            return crange[int(csize * x)];
    }
}

const Color* ColorValue::no_value() const {
    return gray;
}

class ColorValueGlyphItem: public MonoGlyph {
  public:
    ColorValueGlyphItem(const char*, const Color*);
    virtual ~ColorValueGlyphItem();
    virtual void draw(Canvas*, const Allocation&) const;

  private:
    CopyString label_;
    const Color* color_;
};

ColorValueGlyphItem::ColorValueGlyphItem(const char* buf, const Color* c) {
    body(new Background(WidgetKit::instance()->label(buf), c));
    label_ = buf;
    color_ = c;
}

ColorValueGlyphItem::~ColorValueGlyphItem() {}

void ColorValueGlyphItem::draw(Canvas* c, const Allocation& a) const {
    body()->draw(c, a);
    if (OcIdraw::idraw_stream) {
        OcIdraw::pict();
        OcIdraw::rect(c, a.left(), a.bottom(), a.right(), a.top(), color_, NULL, true);
        Transformer t;
        t.translate(a.left(), a.bottom());
        OcIdraw::text(c, label_.string(), t);
        OcIdraw::end();
    }
}

Glyph* ColorValue::make_glyph() {
    LayoutKit& lk = *LayoutKit::instance();
    WidgetKit& wk = *WidgetKit::instance();
    PolyGlyph* box = lk.vbox(csize + 2);
    int c = csize_ ? csize_ : csize;
    for (int i = c - 1; i >= 0; --i) {
        char buf[50];
        float x = low_ + i * (high_ - low_) / (c - 1);
        Sprintf(buf, "%5g", x);
        box->append(new ColorValueGlyphItem(buf, get_color(x)));
        //		box->append(new Background(wk.label(buf), get_color(x)));
    }
    return box;
}

void ColorValue::colormap(int size, bool global) {
    int i;
    if (csize_) {
        for (i = 0; i < csize_; ++i) {
            crange_[i]->unref();
        }
        delete[] crange_;
        crange_ = NULL;
        csize_ = 0;
    }
    if (global) {
        if (csize) {
            for (i = 0; i < csize; ++i) {
                crange[i]->unref();
            }
            delete[] crange;
        }
        csize = (size > 1) ? size : 2;
        crange = new const Color*[csize];
        for (i = 0; i < csize; ++i) {
            crange[i] = gray;
            crange[i]->ref();
        }
    } else {
        csize_ = (size > 1) ? size : 2;
        crange_ = new const Color*[csize_];
        for (i = 0; i < csize_; ++i) {
            crange_[i] = gray;
            crange_[i]->ref();
        }
    }
}
void ColorValue::colormap(int i, int r, int g, int b) {
    if (crange_) {
        if (i >= 0 && i < csize_ && r < 256 && g < 256 && b < 256) {
            crange_[i]->unref();
            crange_[i] = new Color(ColorIntensity(r / 256.),
                                   ColorIntensity(g / 256.),
                                   ColorIntensity(b / 256.));
            Resource::ref(crange_[i]);
        }
    } else {
        if (i >= 0 && i < csize && r < 256 && g < 256 && b < 256) {
            crange[i]->unref();
            crange[i] = new Color(ColorIntensity(r / 256.),
                                  ColorIntensity(g / 256.),
                                  ColorIntensity(b / 256.));
            Resource::ref(crange[i]);
        }
    }
}

FastGraphItem::FastGraphItem(FastShape* g, bool s, bool p)
    : GraphItem(g, s, p) {}

FastShape::FastShape() {}
FastShape::~FastShape() {}

Hinton::Hinton(neuron::container::data_handle<double> pd,
               Coord xsize,
               Coord ysize,
               ShapeScene* ss) {
    pd_ = pd;
    old_ = NULL;  // not referenced
    xsize_ = xsize / 2;
    ysize_ = ysize / 2;
    ss_ = ss;
    neuron::container::notify_when_handle_dies(pd_, this);
}
Hinton::~Hinton() {
    Oc oc;
    oc.notify_pointer_disconnect(this);
}
void Hinton::update(Observable*) {
    pd_ = {};
    ss_->remove(ss_->glyph_index(this));
}
void Hinton::request(Requisition& req) const {
    assert(this);
    Requirement rx(2. * xsize_, 0, 0, .5);
    Requirement ry(2. * ysize_, 0, 0, .5);
    req.require_x(rx);
    req.require_y(ry);
}

void Hinton::allocate(Canvas* c, const Allocation& a, Extension& ext) {
    ext.set(c, a);
}

void Hinton::draw(Canvas* c, const Allocation& a) const {
    if (pd_) {
        Coord x = a.x();
        Coord y = a.y();
        const Color* color = ss_->color_value()->get_color(*pd_);
        c->fill_rect(x - xsize_, y - ysize_, x + xsize_, y + ysize_, color);
        ((Hinton*) this)->old_ = color;
        IfIdraw(rect(c, x - xsize_, y - ysize_, x + xsize_, y + ysize_, color, NULL, true));
    }
}
void Hinton::fast_draw(Canvas* c, Coord x, Coord y, bool) const {
    if (pd_) {
        const Color* color = ss_->color_value()->get_color(*pd_);
        if (color != old_) {
            c->fill_rect(x - xsize_, y - ysize_, x + xsize_, y + ysize_, color);
            ((Hinton*) this)->old_ = color;
        }
    }
}

bool ShapePlot::has_iv_view() {
    return has_iv_view_;
}

#endif  // HAVE_IV

ShapePlotData::ShapePlotData(Symbol* sym, Object* sl) {
    sym_ = sym;
    sl_ = sl;
    if (sl_) {
        ++sl_->refcount;
    }
    varobj(NULL);
    show_mode = 1;
}

ShapePlotData::~ShapePlotData() {
    if (sl_) {
        hoc_dec_refcount(&sl_);
    }
}

bool ShapePlotData::has_iv_view() {
    return false;
}

float ShapePlotData::low() {
    return lo;
}

float ShapePlotData::high() {
    return hi;
}

int ShapePlotData::get_mode() {
    return show_mode;
}

void ShapePlotData::set_mode(int mode) {
    show_mode = mode;
}

void ShapePlotData::scale(float min, float max) {
    lo = min;
    hi = max;
}

void ShapePlotData::variable(Symbol* sym) {
    sym_ = sym;
    scale(-80, 40);
}

const char* ShapePlotData::varname() const {
    if (sym_ == NULL) {
        return "";
    }
    return sym_->name;
}

Object* ShapePlotData::neuron_section_list() {
    return sl_;
}
