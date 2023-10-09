#include <../../nrnconf.h>
#include "classreg.h"
#include "gui-redirect.h"

#if HAVE_IV  // to end of file

#define BEVELJOIN 1
#include <InterViews/display.h>
#include <InterViews/session.h>
#include <InterViews/background.h>
#include <InterViews/style.h>
#include <InterViews/window.h>
#include <InterViews/tformsetter.h>
#include <InterViews/brush.h>
#include <InterViews/action.h>
#include <InterViews/color.h>
#include <InterViews/hit.h>
#include <InterViews/handler.h>
#include <InterViews/event.h>
#include <InterViews/telltale.h>
#include <InterViews/layout.h>
#include <IV-look/kit.h>
#include <OS/list.h>
#include <cmath>
#include "mymath.h"
#include "apwindow.h"
// really only need colors from graph.h
#include "graph.h"
#include "shapeplt.h"
#include "rubband.h"
#include "scenepic.h"
#include "rot3band.h"
#include "nrniv_mf.h"
#include "nrnoc2iv.h"
#include "objcmd.h"
#include "idraw.h"
#include "hocmark.h"
#include "ocobserv.h"
#include "parse.hpp"
#include "ivoc.h"

#define Shape_Section_ "Section PlotShape"
#define Shape_Rotate_  "Rotate3D PlotShape"
#define Shape_Style_   "ShapeStyle PlotShape"

void nrn_define_shape();
extern int nrn_shape_changed_;
extern int section_count;
extern Section** secorder;
extern Object* (*nrnpy_seg_from_sec_x)(Section*, double);

#if BEVELJOIN
static long beveljoin_ = 0;
#endif

static ShapeScene* volatile_ptr_ref;

class ShapeChangeObserver: public Observer {
  public:
    ShapeChangeObserver(ShapeScene*);
    virtual ~ShapeChangeObserver();
    virtual void update(Observable*);
    void force();
    bool needs_update() {
        return (shape_changed_ != nrn_shape_changed_);
    }

  private:
    int shape_changed_;
    int struc_changed_;
    ShapeScene* s_;
};

static const Color* sec_sel_color() {
    static const Color* lt = NULL;
    if (!lt) {
        String c;
        Display* dis = Session::instance()->default_display();
        if (!dis->style()->find_attribute("section_select_color", c) ||
            (lt = Color::lookup(dis, c)) == NULL) {
            lt = Color::lookup(dis, "#ff0000");
        }
        lt->ref();
    }
    return lt;
}

static const Color* sec_adjacent_color() {
    static const Color* lt = NULL;
    if (!lt) {
        String c;
        Display* dis = Session::instance()->default_display();
        if (!dis->style()->find_attribute("section_adjacent_color", c) ||
            (lt = Color::lookup(dis, c)) == NULL) {
            lt = Color::lookup(dis, "#00ff00");
        }
        lt->ref();
    }
    return lt;
}

inline float norm(float x, float y) {
    return (x * x + y * y);
}

/* static */ class OcShape;
// must be append_fixed to OcShape or else...
/* static */ class PointMark: public MonoGlyph, public Observer {
  public:
    PointMark(OcShape*, Object*, const Color*, const char style = 'O', float size = 8.);
    virtual ~PointMark();
    virtual void update(Observable*);
    virtual void disconnect(Observable*);
    virtual void draw(Canvas*, const Allocation&) const;
    const Object* object() {
        return ob_;
    }
    virtual void set_loc(Section*, float x);
    bool everything_ok();

  private:
    GlyphIndex i_;
    Coord x_, y_;
    Object* ob_;
    OcShape* sh_;
    Section* sec_;
    float xloc_;
};

class OcShapeHandler;
/* static */ class OcShape: public ShapeScene {
  public:
    OcShape(SectionList* = NULL);
    virtual ~OcShape();
    virtual void select_section(Section*);
    virtual void handle_picked();
    virtual void selected(ShapeSection* s, Coord x, Coord y) {
        ShapeScene::selected(s, x, y);
    }
    virtual ShapeSection* selected() {
        return ShapeScene::selected();
    }
    virtual void set_select_action(const char*);
    virtual void set_select_action(Object*);
    virtual void save_phase1(std::ostream&);
    virtual PointMark* point_mark(Object*,
                                  const Color*,
                                  const char style = 'O',
                                  const float size = 8.);
    virtual PointMark* point_mark(Section*, float x, const Color*);
    virtual void point_mark_remove(Object* pp = NULL);
    virtual void transform3d(Rubberband* rb = NULL);
    virtual void erase_all();
    virtual void sel_color(ShapeSection* sold, ShapeSection* snew);

  private:
    HocCommand* select_;
    PolyGlyph* point_mark_list_;
    OcShapeHandler* osh_;
    ShapeSection* sold_;
    bool show_adjacent_selection_;
};

/*static*/ class OcShapeHandler: public SectionHandler {
  public:
    OcShapeHandler(OcShape*);
    virtual ~OcShapeHandler();
    virtual bool event(Event&);

  private:
    OcShape* s_;
};
OcShapeHandler::OcShapeHandler(OcShape* s) {
    s_ = s;
}
OcShapeHandler::~OcShapeHandler() {}
bool OcShapeHandler::event(Event&) {
    s_->handle_picked();
    return true;
}
#endif  // HAVE_IV

// Shape class registration for oc
static double sh_view(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Shape.view", v);
#if HAVE_IV
    IFGUI
    OcShape* sh = (OcShape*) v;
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

static double sh_flush(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Shape.flush", v);
#if HAVE_IV
    IFGUI((ShapeScene*) v)->flush();
    ENDGUI
#endif
    return 1.;
}

static double sh_begin(void* v) {  // a noop. Exists only because graphs and
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Shape.begin", v);
    return 1.;  // shapes are often in same list
}

static double sh_save_name(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Shape.save_name", v);
#if HAVE_IV
    IFGUI((ShapeScene*) v)->name(gargstr(1));
    ENDGUI
#endif
    return 1.;
}

static double sh_select(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Shape.select", v);
#if HAVE_IV
    IFGUI
    Section* sec = chk_access();
    ((OcShape*) v)->select_section(sec);
    ENDGUI
#endif
    return 1.;
}
static double sh_select_action(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Shape.action", v);
#if HAVE_IV
    IFGUI
    if (hoc_is_object_arg(1)) {
        ((OcShape*) v)->set_select_action(*hoc_objgetarg(1));
    } else {
        ((OcShape*) v)->set_select_action(gargstr(1));
    }
    ENDGUI
#endif
    return 1.;
}

static double sh_view_count(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Shape.view_count", v);
    int n = 0;
#if HAVE_IV
    IFGUI
    n = ((ShapeScene*) v)->view_count();
    ENDGUI
#endif
    return double(n);
}

double nrniv_sh_nearest(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Shape.nearest", v);
    double d = 0.;
#if HAVE_IV
    IFGUI
    d = ((ShapeScene*) v)->nearest(*getarg(1), *getarg(2));
    ENDGUI
#endif
    return d;
}

double nrniv_sh_push(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Shape.push_seleced", v);
    double d = -1.;
#if HAVE_IV
    IFGUI
    ShapeScene* ss = (ShapeScene*) v;
    ShapeSection* s = ss->selected();
    if (s && s->good()) {
        nrn_pushsec(s->section());
        d = ss->arc_selected();
    }
    ENDGUI
#endif
    return d;
}

Object** nrniv_sh_nearest_seg(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_OBJ("Shape.nearest_seg", v);
    Object* obj = NULL;
#if HAVE_IV
    IFGUI
    ShapeScene* ss = (ShapeScene*) v;
    ShapeSection* ssec = NULL;
    double d = ss->nearest(*getarg(1), *getarg(2));
    ssec = ss->selected();
    if (d < 1e15 && nrnpy_seg_from_sec_x && ssec) {
        d = ss->arc_selected();
        obj = (*nrnpy_seg_from_sec_x)(ssec->section(), d);
    }
    --obj->refcount;
    ENDGUI
#endif
    return hoc_temp_objptr(obj);
}

Object** nrniv_sh_selected_seg(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_OBJ("Shape.selected_seg", v);
    Object* obj = NULL;
#if HAVE_IV
    IFGUI
    ShapeScene* ss = (ShapeScene*) v;
    ShapeSection* ssec = NULL;
    ssec = ss->selected();
    if (nrnpy_seg_from_sec_x && ssec) {
        double d = ss->arc_selected();
        obj = (*nrnpy_seg_from_sec_x)(ssec->section(), d);
    }
    --obj->refcount;
    ENDGUI
#endif
    return hoc_temp_objptr(obj);
}

double nrniv_sh_observe(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Shape.observe", v);
#if HAVE_IV
    IFGUI
    ShapeScene* s = (ShapeScene*) v;
    SectionList* sl = NULL;
    if (ifarg(1)) {
        Object* o = *hoc_objgetarg(1);
        check_obj_type(o, "SectionList");
        sl = new SectionList(o);
        sl->ref();
        s->observe(sl);
        sl->unref();
    } else {
        s->observe(NULL);
    }
    ENDGUI
#endif
    return 0.;
}

double nrniv_sh_rotate(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Shape.rotate", v);
#if HAVE_IV
    IFGUI
    ShapeScene* s = (ShapeScene*) v;
    if (!ifarg(1)) {
        // identity
        s->rotate();
    } else {
        // defines origin (absolute) and rotation
        // (relative to the current coord system)
        s->rotate(*getarg(1), *getarg(2), *getarg(3), *getarg(4), *getarg(5), *getarg(6));
    }
    ENDGUI
#endif
    return 0.;
}

static double sh_unmap(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Shape.unmap", v);
#if HAVE_IV
    IFGUI
    ShapeScene* s = (ShapeScene*) v;
    s->dismiss();
    ENDGUI
#endif
    return 0.;
}

double nrniv_sh_color(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Shape.color", v);
#if HAVE_IV
    IFGUI
    ShapeScene* s = (ShapeScene*) v;
    const Color* c = NULL;
    c = colors->color(int(*getarg(1)));
    if (ifarg(2)) {
        Section* sec;
        double x;
        nrn_seg_or_x_arg(2, &sec, &x);
        s->colorseg(sec, x, c);
    } else {
        s->color(chk_access(), c);
    }
    ENDGUI
#endif
    return 0.;
}

double nrniv_sh_color_all(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Shape.color_all", v);
#if HAVE_IV
    IFGUI
    ShapeScene* s = (ShapeScene*) v;
    const Color* c = NULL;
    c = colors->color(int(*getarg(1)));
    s->color(c);
    ENDGUI
#endif
    return 0.;
}

double nrniv_sh_color_list(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Shape.color_list", v);
#if HAVE_IV
    IFGUI
    ShapeScene* s = (ShapeScene*) v;
    const Color* c = NULL;
    c = colors->color(int(*getarg(2)));
    s->color(new SectionList(*hoc_objgetarg(1)), c);
    ENDGUI
#endif
    return 0.;
}

static double sh_point_mark(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Shape.point_mark", v);
#if HAVE_IV
    IFGUI
    OcShape* s = (OcShape*) v;
    char style = 'O';
    float size = 8.;
    if (hoc_is_object_arg(1)) {
        if (ifarg(3)) {
            if (hoc_is_str_arg(3)) {
                style = *gargstr(3);
            } else {
                style = char(chkarg(3, 0, 127));
            }
        }
        if (ifarg(4)) {
            size = float(chkarg(4, 1e-9, 1e9));
        }
        s->point_mark(*hoc_objgetarg(1), colors->color(int(*getarg(2))), style, size);
    } else {
        s->point_mark(chk_access(), chkarg(1, 0., 1.), colors->color(int(*getarg(2))));
    }
    ENDGUI
#endif
    return 0.;
}

static double sh_point_mark_remove(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Shape.point_mark_remove", v);
#if HAVE_IV
    IFGUI
    Object* o = NULL;
    OcShape* s = (OcShape*) v;
    if (ifarg(1)) {
        o = *hoc_objgetarg(1);
    }
    s->point_mark_remove(o);
    ENDGUI
#endif
    return 0.;
}

static double sh_printfile(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Shape.printfile", v);
#if HAVE_IV
    IFGUI
    ShapeScene* s = (ShapeScene*) v;
    s->printfile(gargstr(1));
    ENDGUI
#endif
    return 1.;
}

static double sh_show(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Shape.show", v);
#if HAVE_IV
    IFGUI
    ShapeScene* s = (ShapeScene*) v;
    s->shape_type(int(chkarg(1, 0., 2.)));
    ENDGUI
#endif
    return 1.;
}


extern double ivoc_gr_menu_action(void* v);

static double exec_menu(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Shape.exec_menu", v);
#if HAVE_IV
    IFGUI((Scene*) v)->picker()->exec_item(gargstr(1));
    ENDGUI
#endif
    return 0.;
}

double nrniv_len_scale(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Shape.len_scale", v);
#if HAVE_IV
    IFGUI
    ShapeScene* scene = (ShapeScene*) v;
    ShapeSection* ss = scene->shape_section(chk_access());
    if (ss) {
        if (ifarg(1)) {
            ss->scale(chkarg(1, 1e-9, 1e9));
            scene->force();
        }
        return ss->scale();
    }
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

static Member_func sh_members[] = {{"nearest", nrniv_sh_nearest},
                                   {"push_selected", nrniv_sh_push},
                                   {"view", sh_view},
                                   {"size", ivoc_gr_size},
                                   {"flush", sh_flush},
                                   {"begin", sh_begin},
                                   {"view_count", sh_view_count},
                                   {"select", sh_select},
                                   {"action", sh_select_action},
                                   {"save_name", sh_save_name},
                                   {"unmap", sh_unmap},
                                   {"color", nrniv_sh_color},
                                   {"color_all", nrniv_sh_color_all},
                                   {"color_list", nrniv_sh_color_list},
                                   {"point_mark", sh_point_mark},
                                   {"point_mark_remove", sh_point_mark_remove},
                                   {"point_mark_remove", sh_point_mark_remove},
                                   {"printfile", sh_printfile},
                                   {"show", sh_show},
                                   {"menu_action", ivoc_gr_menu_action},
                                   {"menu_tool", ivoc_gr_menu_tool},
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
    TRY_GUI_REDIRECT_OBJ("Shape", NULL);
#if HAVE_IV
    OcShape* sh = NULL;
    IFGUI
    int i = 1;
    int iarg = 1;
    SectionList* sl = NULL;
    // first arg may be an object.
    if (ifarg(iarg)) {
        if (hoc_is_object_arg(iarg)) {
            sl = new SectionList(*hoc_objgetarg(iarg));
            sl->ref();
            ++iarg;
        }
    }
    if (ifarg(iarg)) {
        i = int(chkarg(iarg, 0, 1));
    }
    sh = new OcShape(sl);
    Resource::unref(sl);
    sh->ref();
    sh->hoc_obj_ptr(ho);
    if (i) {
        sh->view(200);
    }
    ENDGUI
    return (void*) sh;
#endif
    return 0;
}
static void sh_destruct(void* v) {
    TRY_GUI_REDIRECT_NO_RETURN("~Shape", v);
#if HAVE_IV
    IFGUI((ShapeScene*) v)->dismiss();
    Resource::unref((OcShape*) v);
    ENDGUI
#endif
}
void Shape_reg() {
    //	printf("Shape_reg\n");
    class2oc("Shape", sh_cons, sh_destruct, sh_members, NULL, retobj_members, NULL);
}

#if HAVE_IV

OcShape::OcShape(SectionList* sl)
    : ShapeScene(sl) {
    select_ = NULL;
    point_mark_list_ = NULL;
    osh_ = new OcShapeHandler(this);
    osh_->ref();
    section_handler(osh_);
    sold_ = NULL;
    Display* dis = Session::instance()->default_display();
    show_adjacent_selection_ = dis->style()->value_is_on("show_adjacent_selection");
}
OcShape::~OcShape() {
    if (select_)
        delete select_;
    Resource::unref(point_mark_list_);
    osh_->unref();
    Resource::unref(sold_);
}

void OcShape::erase_all() {
    Resource::unref(point_mark_list_);
    point_mark_list_ = NULL;
    ShapeScene::erase_all();
}

PointMark* OcShape::point_mark(Object* ob, const Color* c, const char style, const float size) {
    if (!point_mark_list_) {
        point_mark_list_ = new PolyGlyph();
    }
    PointMark* g = new PointMark(this, ob, c, style, size);
    point_mark_list_->append(g);
    append_fixed(new GraphItem(g, 0));
    if (!g->everything_ok()) {
        point_mark_list_->remove(point_mark_list_->count() - 1);
        remove(glyph_index(g));
        return NULL;
    }
    return g;
}

PointMark* OcShape::point_mark(Section* sec, float x, const Color* c) {
    if (!point_mark_list_) {
        point_mark_list_ = new PolyGlyph();
    }
    PointMark* g = new PointMark(this, NULL, c);
    g->set_loc(sec, x);
    point_mark_list_->append(g);
    append_fixed(new GraphItem(g, 0));
    if (!g->everything_ok()) {
        point_mark_list_->remove(point_mark_list_->count() - 1);
        remove(glyph_index(g));
        return NULL;
    }
    return g;
}
void OcShape::point_mark_remove(Object* o) {
    if (point_mark_list_) {
        if (o) {
            GlyphIndex i, cnt = point_mark_list_->count();
            for (i = cnt - 1; i >= 0; --i) {
                PointMark* g = (PointMark*) point_mark_list_->component(i);
                if (g->object() == o) {
                    remove(glyph_index(g));
                    point_mark_list_->remove(i);
                    break;
                }
            }
        } else {
            while (point_mark_list_->count()) {
                remove(glyph_index(point_mark_list_->component(0)));
                point_mark_list_->remove(0);
            }
        }
    }
}

void OcShape::set_select_action(const char* s) {
    if (select_) {
        delete select_;
    }
    select_ = new HocCommand(s);
}

void OcShape::set_select_action(Object* pobj) {
    if (select_) {
        delete select_;
    }
    select_ = new HocCommand(pobj);
}

void OcShape::sel_color(ShapeSection* sold, ShapeSection* snew) {
    ShapeSection* ss;
    Section* s;
    const Color* c;
    if (sold) {
        c = Scene::default_foreground();
        s = sold->section();
        sold->setColor(c, this);
        if (show_adjacent_selection_) {
            ss = shape_section(s->parentsec);
            if (ss) {
                ss->setColor(c, this);
            }
            for (s = s->child; s; s = s->sibling) {
                ss = shape_section(s);
                if (ss) {
                    ss->setColor(c, this);
                }
            }
        }
    }
    if (snew) {
        c = sec_sel_color();
        snew->setColor(c, this);
        c = sec_adjacent_color();
        s = snew->section();
        if (show_adjacent_selection_) {
            ss = shape_section(s->parentsec);
            if (ss) {
                ss->setColor(c, this);
            }
            for (s = s->child; s; s = s->sibling) {
                ss = shape_section(s);
                if (ss) {
                    ss->setColor(c, this);
                }
            }
        }
    }
}
void OcShape::select_section(Section* sec) {
    ShapeSection* s1 = ShapeScene::selected();
    ShapeSection* ss;
    Section* s;
    const Color* c;
    ss = shape_section(sec);
    sel_color(s1, ss);
    if (ss) {
        ShapeScene::selected(ss);
        Resource::ref(ss);
        Resource::unref(sold_);
        sold_ = ss;
    }
}
void OcShape::handle_picked() {
    ShapeSection* s1 = ShapeScene::selected();
    if (!s1 || !s1->good()) {
        return;
    }
    sel_color(sold_, s1);
    if (sold_) {
        sold_->unref();
    }
    sold_ = s1;
    sold_->ref();
    if (select_) {
        nrn_pushsec(s1->section());
        hoc_ac_ = arc_selected();
        // printf("arc_selected %g\n", hoc_ac_);
        select_->execute();
        nrn_popsec();
    }
}

void OcShape::save_phase1(std::ostream& o) {
    o << "{" << std::endl;
    save_class(o, "Shape");
}

#if 1
// ShapeView

ShapeView::ShapeView(ShapeScene* s)
    : View((s->x1() + s->x2()) / 2,
           (s->y1() + s->y2()) / 2,
           std::max(s->x2() - s->x1(), s->y2() - s->y1()) * 1.1,
           s
           //	,150*(s->x2() - s->x1())/std::max(s->x2() - s->x1(), s->y2() - s->y1()),
           //	150*(s->y2() - s->y1())/std::max(s->x2() - s->x1(), s->y2() - s->y1())
      ) {}

ShapeView::ShapeView(ShapeScene* s, Coord* x)
    : View(x[0], x[1], x[2], x[3], s, x[6], x[7]) {
    Coord x1, y1, x2, y2;
    zout(x1, y1, x2, y2);
    size(x1, y1, x2, y2);
}

ShapeView::~ShapeView() {}

#endif

/* static */ class ShapeType: public Action {
  public:
    ShapeType(int);
    virtual ~ShapeType();
    virtual void execute();

  private:
    int shapetype_;
};
ShapeType::ShapeType(int st) {
    shapetype_ = st;
}
ShapeType::~ShapeType() {}
void ShapeType::execute() {
    if (Oc::helpmode()) {
        Oc::help(Shape_Style_);
    }
    ShapeScene::current_pick_scene()->shape_type(shapetype_);
}


declareRubberCallback(ShapeScene)
implementRubberCallback(ShapeScene)
declareActionCallback(ShapeScene)
implementActionCallback(ShapeScene)

void ShapeScene::observe(SectionList* sl) {
    GlyphIndex i, cnt;
    hoc_Item* qsec;
    Section* sec;
    ShapeSection* gl;
    while (sg_->count()) {
        gl = (ShapeSection*) sg_->component(sg_->count() - 1);
        i = glyph_index(gl);
        remove(i);
        sg_->remove(sg_->count() - 1);
    }
    if (sl) {  // haven't figured out a way to save this
        view_all_ = false;
        for (sec = sl->begin(); sec; sec = sl->next()) {
            gl = new ShapeSection(sec);
            append(new FastGraphItem(gl, 0));
            sg_->append(gl);
        }
    } else {
        view_all_ = true;
        // ForAllSections(sec)
        ITERATE(qsec, section_list) {
            Section* sec = hocSEC(qsec);
            gl = new ShapeSection(sec);
            append(new FastGraphItem(gl, 0));
            sg_->append(gl);
        }
    }
    recalc_diam();
    selected_ = NULL;
    volatile_ptr_ref = NULL;
    transform3d();
    if (shape_changed_) {
        force();
        flush();
    }
}

void ShapeScene::force() {
    shape_changed_->force();
}

ShapeScene::ShapeScene(SectionList* sl)
    : Graph(0) {
    nrn_define_shape();
    new_size(-100, -100, 100, 100);
    erase_axis();
    WidgetKit& wk = *WidgetKit::instance();
    sg_ = new PolyGlyph();
    sg_->ref();
    shape_changed_ = NULL;  // observe not ready for it yet
    r3b_ = new Rotate3Band(NULL, new RubberCallback(ShapeScene)(this, &ShapeScene::transform3d));
    r3b_->ref();
    observe(sl);
    wk.style()->find_attribute("shape_beveljoin", beveljoin_);

    MenuItem* mi;
    Menu* m;

    shape_type_ = ShapeScene::show_centroid;
    section_handler_ = NULL;

    selected_ = NULL;
    picker();
    picker()->remove_item("Crosshair");
    picker()->remove_item("Plot what?");
    picker()->remove_item("Pick Vector");
    picker()->remove_item("Color/Brush");
    picker()->remove_item("Keep Lines");
    picker()->remove_item("Family Label?");
    picker()->remove_item("Erase");
    picker()->remove_item("Remove");

    picker()->bind_select((OcHandler*) NULL);
    MenuItem* m2 = picker()->add_radio_menu("Section", (OcHandler*) NULL, SECTION);
    m2->state()->set(TelltaleState::is_chosen, true);
    picker()->add_radio_menu("3D Rotate", r3b_, 0, ROTATE);
    picker()->add_menu("Redraw Shape", new ActionCallback(ShapeScene)(this, &ShapeScene::flush));

    m = wk.pullright();
    mi = wk.menu_item("Show Diam");
    mi->action(new ShapeType(ShapeScene::show_diam));
    picker()->add_menu("Show Diam", mi, m);
    mi = wk.menu_item("Centroid");
    mi->action(new ShapeType(ShapeScene::show_centroid));
    picker()->add_menu("Centroid", mi, m);
    mi = wk.menu_item("Schematic");
    mi->action(new ShapeType(ShapeScene::show_schematic));
    picker()->add_menu("Schematic", mi, m);
    mi = wk.menu_item("Shape Style");
    mi->menu(m);
    picker()->add_menu(mi);

    Requisition req;
    Coord x1, y1, x2, y2;
    Coord xt1 = 0, yt1 = 0, xt2 = 0, yt2 = 0;
    GlyphIndex i, cnt = count();
    for (i = 0; i < cnt; ++i) {
        component(i)->request(req);
        MyMath::box(req, x1, y1, x2, y2);
        xt1 = std::min(x1, xt1);
        yt1 = std::min(y1, yt1);
        xt2 = std::max(x2, xt2);
        yt2 = std::max(y2, yt2);
    }
    Scene::new_size(xt1, yt1, xt2, yt2);
    color_value_ = new ColorValue();
    Resource::ref(color_value_);
    shape_changed_ = new ShapeChangeObserver(this);
}

void ShapeScene::help() {
    switch (tool()) {
    case SECTION:
        Oc::help(Shape_Section_);
        break;
    case ROTATE:
        Oc::help(Shape_Rotate_);
        break;
    default:
        Scene::help();
        break;
    }
}

#if 0
StandardPicker* ShapeScene::picker() { return picker_;}
#endif

ShapeScene::~ShapeScene() {
    volatile_ptr_ref = NULL;
    Resource::unref(section_handler_);
    Resource::unref(color_value_);
    Resource::unref(sg_);
    Resource::unref(r3b_);
    delete shape_changed_;
}

void ShapeScene::erase_all() {
    Resource::unref(sg_);
    sg_ = new PolyGlyph();
    sg_->ref();
    volatile_ptr_ref = NULL;
    Graph::erase_all();
}

void ShapeScene::rotate() {
    Rotation3d* rot = r3b_->rotation();
    rot->identity();
    transform3d();
}

void ShapeScene::rotate(Coord xorg, Coord yorg, Coord zorg, float xrad, float yrad, float zrad) {
    Rotation3d* rot = r3b_->rotation();
    rot->origin(xorg, yorg, zorg);
    rot->rotate_x(xrad);
    rot->rotate_y(yrad);
    rot->rotate_z(zrad);
    transform3d();
}

void ShapeScene::transform3d(Rubberband*) {
    Rotation3d* rot = r3b_->rotation();
    Coord x, y;
    //	rb->transformer().inverse_transform(rb->x_begin(), rb->y_begin(), x, y);
    //	printf("ShapeScene::transform3d %g %g\n", x, y);
    long i, n;
    for (i = 0; i < section_count; ++i) {
        ShapeSection* ss = shape_section(secorder[i]);
        if (ss) {
            ss->transform3d(rot);
        }
    }
    n = count();
    for (i = 0; i < n; ++i) {
        modified(i);
    }
}

void ShapeScene::flush() {
    if (shape_changed_->needs_update()) {
        shape_changed_->update(NULL);
    } else {
        damage_all();
    }
}

void ShapeScene::wholeplot(Coord& x1, Coord& y1, Coord& x2, Coord& y2) const {
    long i, j, n = sg_->count();
    Coord l, b, r, t;
    x1 = y1 = 1e9;
    x2 = y2 = -1e9;
    for (i = 0; i < n; ++i) {
        ((ShapeSection*) sg_->component(i))->size(l, b, r, t);
        x1 = std::min(x1, l);
        x2 = std::max(x2, r);
        y1 = std::min(y1, b);
        y2 = std::max(y2, t);
    }
    if (x1 >= x2 || y1 >= y2) {
        Scene::wholeplot(x1, y1, x2, y2);
    }
}

ColorValue* ShapeScene::color_value() {
    return color_value_;
}
PolyGlyph* ShapeScene::shape_section_list() {
    return sg_;
}

void ShapeScene::name(const char* s) {
    var_name_ = s;
}

void ShapeScene::save_phase2(std::ostream& o) {
    if (!var_name_.empty()) {
        if (var_name_.back() == '.') {
            o << var_name_ << "append(save_window_)" << std::endl;
        } else {
            o << var_name_ << " = save_window_" << std::endl;
        }
        o << "save_window_.save_name(\"" << var_name_ << "\")" << std::endl;
    }
    Graph::save_phase2(o);
}

void ShapeScene::view(Rubberband* rb) {
    Coord x1, y1, x2, y2, t, b, l, r;
    ((RubberRect*) rb)->get_rect_canvas(l, b, r, t);
    ((RubberRect*) rb)->get_rect(x1, y1, x2, y2);
    printf("new view with %g %g %g %g\n", x1, y1, x2, y2);
#if 0
	double d1, d2; int ntic;
	MyMath::round_range(x1, x2, d1, d2, ntic);
	x1 = d1;
	x2 = d2;
	MyMath::round_range(y1, y2, d1, d2, ntic);
	y1 = d1;
	y2 = d2;
#endif
    View* v;
    ViewWindow* w = new ViewWindow(v = new View((x2 + x1) / 2,
                                                (y1 + y2) / 2,
                                                x2 - x1,
                                                this,
                                                r - l,
                                                (r - l) * (y2 - y1) / (x2 - x1)),
                                   "Shape");
    const Event& e = rb->event();
    w->place(l + e.pointer_root_x() - e.pointer_x(), b + e.pointer_root_y() - e.pointer_y());
    w->map();
}

void ShapeScene::section_handler(SectionHandler* h) {
    Resource::ref(h);
    Resource::unref(section_handler_);
    section_handler_ = h;
}

SectionHandler* ShapeScene::section_handler(ShapeSection* ss) {
    if (section_handler_) {
        section_handler_->shape_section(ss);
    }
    return section_handler_;
}
SectionHandler* ShapeScene::section_handler() {
    return section_handler_;
}

void ShapeScene::shape_type(int s) {
    shape_type_ = s;
    damage_all();
}

float ShapeScene::nearest(Coord x, Coord y) {
    GlyphIndex i, cnt = sg_->count();
    float d2, d = 1e20;
    for (i = 0; i < cnt; ++i) {
        ShapeSection* ss = (ShapeSection*) sg_->component(i);
        if (ss->good()) {
            d2 = ss->how_near(x, y);
            if (d2 < d) {
                selected(ss, x, y);
                d = d2;
            }
        }
    }
    //	printf("nearest %s %g\n", hoc_section_pathname(selected()->section()), d);
    return d;
}

ShapeSection* ShapeScene::shape_section(Section* sec) {
    GlyphIndex i, cnt = sg_->count();
    if (this != volatile_ptr_ref) {
        volatile_ptr_ref = this;
        for (i = 0; i < section_count; ++i) {
            secorder[i]->volatile_ptr = NULL;
        }
        for (i = 0; i < cnt; ++i) {
            ShapeSection* ss = (ShapeSection*) sg_->component(i);
            if (ss->good()) {
                ss->section()->volatile_ptr = ss;
            }
        }
    }
    return sec ? (ShapeSection*) sec->volatile_ptr : NULL;
}

// color the shapesections

static bool par_helper(Section* sec) {
    /* decide whether a sec with 2 marks should be colored. yes if
    there are not two single marked children connected at same location
    or any double marked children */
    Section* csec;
    double y, x = -1.;
    for (csec = sec->child; csec; csec = csec->sibling) {
        switch (nrn_value_mark(csec)) {
        case 1:
            y = nrn_connection_position(csec);
            if (x == y) {
                return false;
            }
            x = y;
            break;
        case 2:
            return false;
        }
    }
    return true;
}

void ShapeScene::color(Section* sec1, Section* sec2, const Color* c) {
    nrn_clear_mark();
    Section* sec;
    for (sec = sec1; sec; sec = nrn_trueparent(sec)) {
        nrn_increment_mark(sec);
    }
    for (sec = sec2; sec; sec = nrn_trueparent(sec)) {
        nrn_increment_mark(sec);
    }
    GlyphIndex i, cnt = sg_->count();
    for (i = 0; i < cnt; ++i) {
        ShapeSection* ss = (ShapeSection*) sg_->component(i);
        if (ss->good()) {
            switch (nrn_value_mark(ss->section())) {
            case 1:
                ss->setColor(c, this);
                break;
            case 2:
                if (par_helper(ss->section())) {
                    ss->setColor(c, this);
                }
                break;
            }
        }
    }
}

void ShapeScene::colorseg(Section* sec, double x, const Color* c) {
    ShapeSection* ss = shape_section(sec);
    if (ss && ss->color() != c) {
        ss->setColorseg(c, x, this);
    }
}

void ShapeScene::color(Section* sec, const Color* c) {
    ShapeSection* ss = shape_section(sec);
    if (ss && ss->color() != c) {
        ss->setColor(c, this);
    }
}

void ShapeScene::color(const Color* c) {
    GlyphIndex i, cnt = sg_->count();
    for (i = 0; i < cnt; ++i) {
        ShapeSection* ss = (ShapeSection*) sg_->component(i);
        if (ss->color() != c && ss->good()) {
            ss->setColor(c, this);
        }
    }
}

void ShapeScene::color(SectionList* sl, const Color* c) {
    Resource::ref(sl);
    nrn_clear_mark();
    for (Section* sec = sl->begin(); sec; sec = sl->next()) {
        nrn_increment_mark(sec);
    }
    GlyphIndex i, cnt = sg_->count();
    for (i = 0; i < cnt; ++i) {
        ShapeSection* ss = (ShapeSection*) sg_->component(i);
        if (ss->color() != c && ss->good() && nrn_value_mark(ss->section())) {
            ss->setColor(c, this);
        }
    }
    Resource::unref(sl);
}

void ShapeScene::view(Coord) {
    ShapeView* v = new ShapeView(this);
    ViewWindow* w = new ViewWindow(v, "Shape");
    w->map();
}

void ShapeScene::view(Coord* x) {
    ShapeView* v = new ShapeView(this, x);
    ViewWindow* w = new ViewWindow(v, "Shape");
    w->xplace(int(x[4]), int(x[5]));
    w->map();
}

ShapeSection* ShapeScene::selected() {
    return selected_;
}

void ShapeScene::selected(ShapeSection* s, Coord x, Coord y) {
    selected_ = s;
    x_sel_ = x;
    y_sel_ = y;
}

float ShapeScene::arc_selected() {
    if (!selected() || x_sel_ == fil) {
        return .5;
    }
    return selected()->arc_position(x_sel_, y_sel_);
}

ShapeSection::ShapeSection(Section* sec) {
    sec_ = sec;
    section_ref(sec_);
    color_ = Scene::default_foreground();
    color_->ref();
    colorseg_ = NULL;
    colorseg_size_ = 0;
    scale(1.);

    if (sec_->npt3d == 0) {
        nrn_define_shape();
    }
    n_ = sec_->npt3d;
    assert(n_);
    x_ = new float[n_];
    y_ = new float[n_];
    //	Rotation3d rot;
    //	transform3d(&rot);
}

ShapeSection::~ShapeSection() {
    color_->unref();
    int n = sec_->npt3d - 1;
    delete[] x_;
    delete[] y_;
    clear_variable();
    section_unref(sec_);
}

void ShapeSection::transform3d(Rotation3d* rot) {
    int i;
    if (!good()) {
        return;
    }
    if (n_ != sec_->npt3d) {
        if (sec_->npt3d == 0) {
            nrn_define_shape();
        }
        n_ = sec_->npt3d;
        delete[] x_;
        delete[] y_;
        x_ = new float[n_];
        y_ = new float[n_];
    }
    float r[3];
    Coord x0, y0, xp, yp;
    r[0] = sec_->pt3d[0].x;
    r[1] = sec_->pt3d[0].y;
    r[2] = sec_->pt3d[0].z;
    rot->rotate(r, r);
    xp = x0 = r[0];
    yp = y0 = r[1];

    // needed for len_scale since each section has to be translated to
    // its connection point
    Section* ps = nrn_trueparent(sec_);
    if (ps && ps->volatile_ptr) {
        ShapeSection* pss = (ShapeSection*) ps->volatile_ptr;
        // need the connection position relative to the trueparent
        Section* sec;
        for (sec = sec_; sec->parentsec != ps; sec = sec->parentsec) {
            ;
        }
        pss->loc(nrn_connection_position(sec), xp, yp);
    }
    // but need to deal with the logical_connection which may exist
    // on any section between sec and trueparent. Just hope there is
    // no more than one.
    Coord xinc = 0;
    Coord yinc = 0;
    Pt3d* logic_con = NULL;
    if (ps) {
        for (Section* sec = sec_; sec != ps; sec = sec->parentsec) {
            logic_con = sec->logical_connection;
            if (logic_con) {
                break;
            }
        }
    }
    if (logic_con) {
        r[0] = logic_con->x;
        r[1] = logic_con->y;
        r[2] = logic_con->z;
        rot->rotate(r, r);
        xinc = x0 - r[0];
        yinc = y0 - r[1];
    }
    xp += xinc;
    yp += yinc;

    for (i = 0; i < n_; ++i) {
        r[0] = sec_->pt3d[i].x;
        r[1] = sec_->pt3d[i].y;
        r[2] = sec_->pt3d[i].z;
        rot->rotate(r, r);
        x_[i] = xp + len_scale_ * (r[0] - x0);  // *100./(100. - r[2]);
        y_[i] = yp + len_scale_ * (r[1] - y0);  // *100./(100. - r[2]);
    }
    Coord x = x_[0];
    Coord y = y_[0];
    Coord d2 = std::abs(sec_->pt3d[0].d) / 2 + 1;

    xmin_ = x - d2;
    xmax_ = x + d2;
    ymin_ = y - d2;
    ymax_ = y + d2;

    for (i = 1; i < n_; i++) {
        x = x_[i];
        y = y_[i];
        d2 = std::abs(sec_->pt3d[i].d) / 2 + 1;

        xmin_ = std::min(xmin_, x - d2);
        xmax_ = std::max(xmax_, x + d2);
        ymin_ = std::min(ymin_, y - d2);
        ymax_ = std::max(ymax_, y + d2);
    }
}

void ShapeSection::loc(double arc, Coord& x, Coord& y) {
    double a = arc0at0(sec_) ? arc : 1. - arc;
    double len = section_length(sec_);
    int i;
    if (a <= .0001) {
        i = 0;
    } else if (a >= .999) {
        i = sec_->npt3d - 1;
    } else {
        a *= len;
        for (i = 1; i < sec_->npt3d; ++i) {
#if 0
			// the nearest 3-d point
			if (a < (sec_->pt3d[i].arc + sec_->pt3d[i-1].arc)) {
				break;
			}
#else
            // above is not good if 3-d points are far apart and not near center of
            // a segment. So return the location at the center of the segment.
            if (a <= sec_->pt3d[i].arc) {
                float a1 = sec_->pt3d[i - 1].arc;
                float a2 = sec_->pt3d[i].arc;
                if (a2 > a1) {
                    float t1 = (a - a1) / (a2 - a1);
                    x = x_[i] * t1 + x_[i - 1] * (1. - t1);
                    y = y_[i] * t1 + y_[i - 1] * (1. - t1);
                    return;
                } else {
                    break;
                }
            }
#endif
        }
        i -= 1;
    }
    x = x_[i];
    y = y_[i];
}

#if 0
void ShapeSection::update(Observable* o) {
	TelltaleState* t = (TelltaleState*)o;
	printf("update %d %d\n", secname(section()), t->flags());
	if (t->test(TelltaleState::is_enabled_active)) {
		setColor(sec_sel_color());
		ShapeScene::current_pick_scene()->selected(this);
	}else if (t->test(TelltaleState::is_enabled)) {
		if (color_ == sec_sel_color()) {
//printf("setting to dark\n");
			setColor(Scene::default_foreground());
		}
		ShapeScene::current_pick_scene()->selected(NULL);
	}
}
#endif

void ShapeSection::request(Requisition& req) const {
    //	Requirement rx(xmax_ - xmin_, 0, 0, -xmin_/(xmax_ - xmin_));
    //	Requirement ry(ymax_ - ymin_, 0, 0, -ymin_/(ymax_ - ymin_));

    Requirement rx(-xmin_, -xmin_, -xmin_, xmax_, xmax_, xmax_);
    Requirement ry(-ymin_, -ymin_, -ymin_, ymax_, ymax_, ymax_);
    req.require(Dimension_X, rx);
    req.require(Dimension_Y, ry);
}

void ShapeSection::size(Coord& l, Coord& b, Coord& r, Coord& t) const {
    l = xmin_;
    r = xmax_;
    b = ymin_;
    t = ymax_;
}

void ShapeSection::allocate(Canvas* c, const Allocation& a, Extension& ext) {
    ext.set(c, a);
    //	Coord x = a.x();
    //	Coord y = a.y();
    //	ext.merge_xy(c, x + xmin_, x + xmax_, y + ymin_, y + ymax_);
    //	ext.set_xy(c, xmin_, xmax_, ymin_, ymax_);
}

void ShapeSection::selectMenu() {  // popup menu item selected
    const char* name = secname(sec_);
    printf("%s\n", name);
    Color* blue = (Color*) Color::lookup(Session::instance()->default_display(), "blue");
    ShapeScene* s = ShapeScene::current_pick_scene();
    setColor(blue, s);
    s->selected(this);
    Oc oc;
    hoc_ivpanel(name);
    char buf[200];
    hoc_ivmenu(name);
    Sprintf(buf, "%s nrnsecmenu(.5, 1)", name);
    hoc_ivbutton("Parameters", buf);
    Sprintf(buf, "%s nrnsecmenu(.5, 2)", name);
    hoc_ivbutton("Assigned", buf);
    Sprintf(buf, "%s nrnsecmenu(.5, 3)", name);
    hoc_ivbutton("States", buf);
    hoc_ivmenu(0);
    hoc_ivpanel(0);
}

Section* ShapeSection::section() const {
    return sec_;
}

bool ShapeSection::good() const {
    return sec_->prop != 0;
}

void ShapeSection::set_range_variable(Symbol* sym) {
    clear_variable();
    if (!good()) {
        return;
    }
    auto* const sec = section();
    auto const n = sec->nnode - 1;
    pvar_.clear();
    old_.clear();
    pvar_.resize(n);
    old_.resize(n);
    if (nrn_exists(sym, sec->pnode[0])) {
        for (int i = 0; i < n; ++i) {
            pvar_[i] = nrn_rangepointer(sec, sym, nrn_arc_position(sec, sec->pnode[i]));
        }
    }
}
void ShapeSection::clear_variable() {
    pvar_.clear();
    old_.clear();
    if (colorseg_) {
        for (int i = 0; i < colorseg_size_; ++i) {
            colorseg_[i]->unref();
        }
        delete[] colorseg_;
        colorseg_ = NULL;
        colorseg_size_ = 0;
    }
}
void ShapeSection::draw(Canvas* c, const Allocation& a) const {
    if (!good()) {
        return;
    }
    float e = 1e-2;
#if 0
// fails when length very long > 100000. If checking important
// then should use relative comparison
	if (!(
		Math::equal(xmin_, a.left(), e) &&
		Math::equal(xmax_, a.right(), e) &&
		Math::equal(ymin_, a.bottom(), e) &&
		Math::equal(ymax_, a.top(), e)
	)) {
printf("xmin_=%g a.left=%g ymin_=%g a.bottom=%g xmax_=%g a.right=%g\n",
xmin_, a.left(),ymin_,a.bottom(),xmax_,a.right());
	}
#endif
    Coord x = a.x(), y = a.y();
    fast_draw(c, x, y, true);
}

void ShapeSection::fast_draw(Canvas* c, Coord x, Coord y, bool b) const {
    Section* sec = section();
    IfIdraw(pict());
    if (!pvar_.empty() || (colorseg_ && colorseg_size_ == sec_->nnode - 1)) {
        const Color* color;
        ColorValue* cv;
        if (!pvar_.empty()) {
            cv = ShapeScene::current_draw_scene()->color_value();
        }
        if (sec->nnode == 2) {
            if (colorseg_) {
                color = colorseg_[0];
            } else {
                if (pvar_[0]) {
                    color = cv->get_color(*pvar_[0]);
                } else {
                    color = cv->no_value();
                }
                if (color != old_[0] || b) {
                    b = true;
                    const_cast<ShapeSection*>(this)->old_[0] = color;
                }
            }
            if (b) {
                draw_points(c, color, 0, sec_->npt3d);
            }
///////////////////////////////////////
#if FASTIDIOUS
        } else if (sec->npt3d > 2) {
            // draw each segment with proper color such that segment boundaries are
            // at least within 5% of their proper location and best possible relative to
            // actual points. i.e. if a section boundary is between 3d points such that
            // moving the boundary to the nearest point increases or decreases the
            // length of the segment by more than 5%, then draw the fractional
            // interval. Otherwise move the boundary to the nearest point.

            int iseg, i3d;
            double xbegin;  // end location already drawn
            double xend;    // location we'd like to draw to
            double dseg;    // accurate desired length of segment
            double a3dold;  // the arc length at i3d-1
            double a3dnew;  // the arc length at i3d
            double frac;    // fraction of a segment length of the a3dnew point from
                            // xend when a3dnew > xend

            // walk iseg=0 through sec->nnode-2
            // note that a3d points are totally arbitrary but segments are
            // all approximately dseg in length.
            // we don't want to draw anything < .05*dseg in length unless it is
            // a complete a3d interval. We might draw something up to 1.1*dseg in length.
            dseg = section_length(sec_) / double(sec_->nnode - 1);
            xbegin = 0.;
            a3dold = 0.;
            i3d = 1;
            for (iseg = 0; iseg < sec->nnode - 1; ++iseg) {
                if (colorseg_) {
                    color = colorseg_[iseg];
                } else {
                    if (pvar_[iseg]) {
                        color = cv->get_color(*pvar_[iseg]);
                    } else {
                        color = cv->no_value();
                    }
                    if (color != old_[iseg] || b) {
                        const_cast<ShapeSection*>(this)->old_[iseg] = color;
                        b = true;
                    }
                }
                xend = double(iseg + 1) * dseg;
                for (; i3d < sec->npt3d; ++i3d) {
                    a3dold = sec_->pt3d[i3d - 1].arc;
                    a3dnew = sec_->pt3d[i3d].arc;
                    if (a3dnew > xend) {
                        frac = (a3dnew - xend) / dseg;
                        // do we move to a3dnew or
                        // actually draw the fractional line
                        if (frac < .05) {  // draw to a3dnew
                            fastidious_draw(c, color, i3d, xbegin, a3dnew);
                            xbegin = a3dnew;
                            ++i3d;
                            break;  // on to next segment
                                    // and next i3d
                        } else {    // draw to xend
                            fastidious_draw(c, color, i3d, xbegin, xend);
                            xbegin = xend;
                            break;  // on to next segment
                                    // and reread i3d
                        }
                    } else {  // draw from xbegin to a3dnew
                        fastidious_draw(c, color, i3d, xbegin, a3dnew);
                        xbegin = a3dnew;
                    }
                }
            }
            assert(MyMath::eq(xend, sec_->pt3d[sec_->npt3d - 1].arc, 1e-6));
#endif  // FASTIDIOUS
        ///////////////////////////////////////
        } else {
            for (int iseg = 0; iseg < sec->nnode - 1; ++iseg) {
                if (colorseg_) {
                    color = colorseg_[iseg];
                } else {
                    if (pvar_[iseg]) {
                        color = cv->get_color(*pvar_[iseg]);
                    } else {
                        color = cv->no_value();
                    }
                    if (color != old_[iseg] || b) {
                        const_cast<ShapeSection*>(this)->old_[iseg] = color;
                        b = true;
                    }
                }
                if (b) {
                    draw_seg(c, color, iseg);
                }
            }
        }
    } else {
        draw_points(c, color_, 0, sec_->npt3d);
    }
    IfIdraw(end());
}

#if FASTIDIOUS
void ShapeSection::fastidious_draw(Canvas* c,
                                   const Color* color,
                                   int i1,
                                   float a1,
                                   float a2) const {
    int i;
    float len, f1, f2, d, x1, x2, y1, y2, a, aa;
    if (!color) {
        return;
    }
    i = i1 - 1;
    a = sec_->pt3d[i].arc;
    aa = sec_->pt3d[i1].arc;
    if ((aa - a) < 1e-5) {
        return;
    }
    f1 = (a1 - a) / (aa - a);
    f2 = (a2 - a) / (aa - a);
    d = x_[i1] - x_[i];
    x1 = f1 * d + x_[i];
    x2 = f2 * d + x_[i];
    d = y_[i1] - y_[i];
    y1 = f1 * d + y_[i];
    y2 = f2 * d + y_[i];
    switch (ShapeScene::current_draw_scene()->shape_type()) {
    case ShapeScene::show_diam:
        float d1, d2, t1, t2;
        t1 = std::abs(sec_->pt3d[i].d) / 2.;
        t2 = std::abs(sec_->pt3d[i1].d) / 2.;
        d1 = f1 * (t2 - t1) + t1;
        d2 = f2 * (t2 - t1) + t1;
        trapezoid(c, color, x1, y1, x2, y2, d1, d2);
        if (beveljoin_) {
            if (f1 < 1e-6) {
                bevel_join(c, color, i, t1);
            }
        }
        break;
    case ShapeScene::show_schematic:
    case ShapeScene::show_centroid:
        c->new_path();
        c->move_to(x1, y1);
        c->line_to(x2, y2);
        c->stroke(color, brushes->brush(0));
        IfIdraw(line(c, x1, y1, x2, y2, color));
        break;
    }
}

#endif

#if BEVELJOIN
void ShapeSection::bevel_join(Canvas* c, const Color* color, int i, float d) const {
    if (i == 0) {
        return;
    }
    float perp1[2], perp2[2], x, y;
    x = x_[i];
    y = y_[i];
    bool b = true;
    b &= MyMath::unit_normal(x - x_[i - 1], y - y_[i - 1], perp1);
    b &= MyMath::unit_normal(x_[i + 1] - x, y_[i + 1] - y, perp2);
    if (b && (perp1[0] != perp2[0] || perp1[1] != perp2[1])) {
        Coord xt[4], yt[4];
        xt[0] = x + d * perp1[0];
        yt[0] = y + d * perp1[1];
        xt[1] = x - d * perp2[0];
        yt[1] = y - d * perp2[1];
        xt[2] = x - d * perp1[0];
        yt[2] = y - d * perp1[1];
        xt[3] = x + d * perp2[0];
        yt[3] = y + d * perp2[1];
        int i;
        c->new_path();
        c->move_to(xt[0], yt[0]);
        for (i = 1; i < 4; ++i) {
            c->line_to(xt[i], yt[i]);
        }
        c->close_path();
        c->fill(color);
        if (OcIdraw::idraw_stream) {
            OcIdraw::polygon(c, 4, xt, yt, color, 0, true);
        }
    }
}
#else
void ShapeSection::bevel_join(Canvas*, const Color*, int, float) const {}
#endif

void ShapeSection::draw_seg(Canvas* c, const Color* color, int iseg) const {
    float darc = 1. / float(sec_->nnode - 1);
    float ds = darc * section_length(sec_);
    float x = ds * iseg;
    int i, j;
    if (sec_->nnode == 2) {
        i = 0;
        j = sec_->npt3d;
        draw_points(c, color, i, j);
    } else if (sec_->npt3d == 2) {
        float x1, x2, y1, y2;
        x1 = darc * iseg * (x_[1] - x_[0]) + x_[0];
        x2 = darc * (iseg + 1) * (x_[1] - x_[0]) + x_[0];
        y1 = darc * iseg * (y_[1] - y_[0]) + y_[0];
        y2 = darc * (iseg + 1) * (y_[1] - y_[0]) + y_[0];
        switch (ShapeScene::current_draw_scene()->shape_type()) {
        case ShapeScene::show_diam:
            float d1, d2, t1, t2;
            t1 = std::abs(sec_->pt3d[0].d) / 2.;
            t2 = std::abs(sec_->pt3d[1].d) / 2.;
            d1 = darc * iseg * (t2 - t1) + t1;
            d2 = darc * (iseg + 1) * (t2 - t1) + t1;
            trapezoid(c, color, x1, y1, x2, y2, d1, d2);
            break;
        case ShapeScene::show_schematic:
        case ShapeScene::show_centroid:
            c->new_path();
            c->move_to(x1, y1);
            c->line_to(x2, y2);
            c->stroke(color, brushes->brush(0));
            IfIdraw(line(c, x1, y1, x2, y2, color));
            break;
        }
    } else {
        for (i = 1; i < sec_->npt3d; i++) {
            if (sec_->pt3d[i].arc > x) {
                break;
            }
        }
        i--;
        x += ds * 1.0001;
        for (j = i + 1; j < sec_->npt3d; j++) {
            if (sec_->pt3d[j].arc > x) {
                break;
            }
        }
        draw_points(c, color, i, j);
    }
}

void ShapeSection::draw_points(Canvas* c, const Color* color, int i, int j) const {
    switch (ShapeScene::current_draw_scene()->shape_type()) {
    case ShapeScene::show_diam:
        while (++i < j) {
            trapezoid(c, color, i);
#if BEVELJOIN
            if (beveljoin_) {
                bevel_join(c, color, i - 1, std::abs(sec_->pt3d[i - 1].d) / 2);
            }
#endif
        }
        break;
    case ShapeScene::show_centroid:
        IfIdraw(mline(c, j - i, x_ + i, y_ + i, color));
        c->new_path();
        c->move_to(x_[i], y_[i]);
        while (++i < j) {
            c->line_to(x_[i], y_[i]);
        }
        c->stroke(color, brushes->brush(0));
        break;
    case ShapeScene::show_schematic:
        IfIdraw(line(c, x_[i], y_[i], x_[j - 1], y_[j - 1], color));
        c->new_path();
        c->line(x_[i], y_[i], x_[j - 1], y_[j - 1], color, 0);
        break;
    }
}

void ShapeSection::trapezoid(Canvas* c, const Color* color, int i) const {
    trapezoid(c,
              color,
              x_[i - 1],
              y_[i - 1],
              x_[i],
              y_[i],
              std::abs(sec_->pt3d[i - 1].d) / 2.,
              std::abs(sec_->pt3d[i].d) / 2.);
}

void ShapeSection::trapezoid(Canvas* c,
                             const Color* color,
                             float x1,
                             float y1,
                             float x2,
                             float y2,
                             float d1,
                             float d2) const {
    float x, y, rx, ry, d, norm;
    x = x2 - x1;
    y = y2 - y1;
    norm = sqrt(x * x + y * y);

    if (norm > .0001) {
        rx = y / norm;
        ry = -x / norm;
    } else {
        return;
    }

    d = d1;
    c->new_path();
    c->move_to(x1 + rx * d, y1 + ry * d);
    c->line_to(x1 - rx * d, y1 - ry * d);
    d = d2;
    c->line_to(x2 - rx * d, y2 - ry * d);
    c->line_to(x2 + rx * d, y2 + ry * d);
    c->close_path();
    c->fill(color);
    if (OcIdraw::idraw_stream) {
        Coord xt[4], yt[4];
        d = d1;
        xt[0] = x1 + rx * d;
        yt[0] = y1 + ry * d;
        xt[1] = x1 - rx * d;
        yt[1] = y1 - ry * d;
        d = d2;
        xt[2] = x2 - rx * d;
        yt[2] = y2 - ry * d;
        xt[3] = x2 + rx * d;
        yt[3] = y2 + ry * d;
        OcIdraw::polygon(c, 4, xt, yt, color, 0, true);
    }
}

void ShapeSection::setColorseg(const Color* color, double x, ShapeScene* s) {
    if (x <= 0.0 || x >= 1.0) {
        return;
    }
    if (colorseg_size_ != sec_->nnode - 1) {
        clear_variable();
    }
    if (!colorseg_) {
        colorseg_size_ = sec_->nnode - 1;
        colorseg_ = new const Color*[colorseg_size_];
        for (int i = 0; i < colorseg_size_; ++i) {
            colorseg_[i] = color_;
            color_->ref();
        }
    }
    int i = int(x * colorseg_size_);
    color->ref();
    colorseg_[i]->unref();
    colorseg_[i] = color;
    damage(s);
}

void ShapeSection::setColor(const Color* color, ShapeScene* s) {
    clear_variable();
    color->ref();
    color_->unref();
    color_ = color;
    damage(s);
}

void ShapeSection::pick(Canvas*, const Allocation&, int depth, Hit& h) {
    if (!good() || !h.event() || h.event()->type() != Event::down)
        return;
    Coord x = h.left();
    Coord y = h.bottom();
    if (!near_section(x, y, XYView::current_pick_view()->x_pick_epsilon()))
        return;
    if (h.event()->pointer_button() == Event::left) {
        ShapeScene* s = ShapeScene::current_pick_scene();
        // printf("section %s x=%g y=%g\n", secname(sec_), x, y);
        if (h.any()) {  // maybe one already found is closer
            Coord x2 = how_near(x, y);
            if (s->selected()) {
                Coord x1 = s->selected()->how_near(x, y);
                // printf("%s at %g and %s at %g\n", secname(s->selected()->section()), x1,
                // secname(section()), x2);
                if (x1 < x2) {
                    return;
                }
            }
        }
        s->selected(this, x, y);
        if (s->section_handler()) {
            h.target(depth, this, 0, (s->section_handler(this)));
        }
    }
}

ShapeScene* ShapeScene::current_pick_scene() {
    return (ShapeScene*) XYView::current_pick_view()->scene();
}

ShapeScene* ShapeScene::current_draw_scene() {
    return (ShapeScene*) XYView::current_draw_view()->scene();
}

bool ShapeSection::near_section(Coord x, Coord y, Coord mineps) const {
    int n = sec_->npt3d;
    for (int i = 1; i < n; ++i) {
        if (MyMath::near_line_segment(x,
                                      y,
                                      x_[i - 1],
                                      y_[i - 1],
                                      x_[i],
                                      y_[i],
                                      std::max(float(std::abs(sec_->pt3d[i - 1].d) / 2.),
                                               float(mineps)))) {
            return true;
        }
    }
    return false;
}

float ShapeSection::how_near(Coord x, Coord y) const {
    int n = sec_->npt3d;
    float d2, d = 1e20;
    for (int i = 1; i < n; ++i) {
        d2 = MyMath::distance_to_line_segment(x, y, x_[i - 1], y_[i - 1], x_[i], y_[i]);
        if (d2 < d) {
            d = d2;
        }
    }
    return d;
}

float ShapeSection::arc_position(Coord x, Coord y) const {
    int ic, n = sec_->npt3d;
    float d2, d = 1e20;
    float darc, len, dlen1;
    for (int i = 1; i < n; ++i) {
        d2 = MyMath::distance_to_line_segment(x, y, x_[i - 1], y_[i - 1], x_[i], y_[i]);
        if (d2 < d) {
            d = d2;
            ic = i - 1;
        }
    }

    d *= d;
    len = MyMath::norm2(x_[ic] - x_[ic + 1], y_[ic] - y_[ic + 1]);
    dlen1 = MyMath::norm2(x - x_[ic], y - y_[ic]);
    if (dlen1 <= d + 1e-2) {
        darc = 0.;
    } else if (len <= d + 1e-2) {
        darc = sqrt(len);
    } else {
        darc = sqrt(dlen1 - d);
    }

    d = sec_->pt3d[ic].arc + darc;
    d /= section_length(sec_);
    d = (d < 0.) ? 0. : d;
    d = (d > 1.) ? 1. : d;
    d = (nrn_section_orientation(sec_) == 1.) ? 1. - d : d;
    // round to nearest segment point
    float dx = 1. / (sec_->nnode - 1);
    if (d < dx / 4.) {
        d = 0.;
    } else if (d > 1. - dx / 4.) {
        d = 1.;
    } else {
        d = (int(d * (sec_->nnode - 1)) + .5) * dx;
    }
    return d;
}

int ShapeSection::get_coord(double a, Coord& x, Coord& y) const {
    int i, n = sec_->npt3d;
    double arc = (nrn_section_orientation(sec_) == 1.) ? (1. - a) : a;
    arc *= section_length(sec_);
    for (i = 0; i < n; ++i) {
        if (arc < sec_->pt3d[i].arc) {
            break;
        }
    }
    if (i == n) {
        i -= 1;
        x = x_[i];
        y = y_[i];
    } else {
        double frac = (arc - sec_->pt3d[i - 1].arc) / (sec_->pt3d[i].arc - sec_->pt3d[i - 1].arc);
        x = x_[i - 1] * (1 - frac) + x_[i] * frac;
        y = y_[i - 1] * (1 - frac) + y_[i] * frac;
        i = (i > 0 && frac < .5) ? i - 1 : i;
    }
    return i;
}

void ShapeSection::damage(ShapeScene* s) {
    // printf("ShapeSection::damage %s\n", secname(sec_));
    s->damage(xmin_, ymin_, xmax_, ymax_);
}

SectionHandler::SectionHandler() {
    ss_ = NULL;
}
SectionHandler::~SectionHandler() {
    shape_section(NULL);
}
bool SectionHandler::event(Event&) {
    return true;
}
void SectionHandler::shape_section(ShapeSection* ss) {
    Resource::ref(ss);
    Resource::unref(ss_);
    ss_ = ss;
}
ShapeSection* SectionHandler::shape_section() {
    return ss_;
}

PointMark::PointMark(OcShape* sh, Object* ob, const Color* c, const char style, const float size)
    : MonoGlyph(NULL) {
    sh_ = sh;  // don't ref
    ob_ = ob;
    if (ob_) {
        ObjObservable::Attach(ob, this);
    }
    body(HocMark::instance(style, size, c, NULL));
    i_ = 0;
    sec_ = NULL;
    xloc_ = 0.;
}
PointMark::~PointMark() {
    // printf("~PointMark\n");
    if (ob_) {
        Object* ob = ob_;
        ob_ = NULL;
        // printf("Detach\n");
        ObjObservable::Detach(ob, this);
    }
}
void PointMark::disconnect(Observable*) {
    // printf("PointMark::disconnect\n");
    if (ob_) {
        Object* ob = ob_;
        ob_ = NULL;
        // printf("point_mark_remove\n");
        sh_->point_mark_remove(ob);
    }
}
void PointMark::update(Observable*) {
    everything_ok();
}

void PointMark::draw(Canvas* c, const Allocation& a0) const {
    const Transformer& tv = XYView::current_draw_view()->s2o();
    Allocation a(a0);
    Coord x, y;
    tv.inverse_transform(x_, y_, x, y);
    a.x_allotment().origin(x);
    a.y_allotment().origin(y);
    MonoGlyph::draw(c, a);
}

void PointMark::set_loc(Section* sec, float x) {
    sec_ = sec;
    xloc_ = x;
}

bool PointMark::everything_ok() {
    sec_ = NULL;
    if (ob_) {
        Point_process* pnt = ob2pntproc_0(ob_);
        if (pnt && pnt->sec) {
            sec_ = pnt->sec;
            xloc_ = nrn_arc_position(pnt->sec, pnt->node);
        }
    }
    if (!sec_ || !sec_->prop) {
        return false;
    }
    ShapeSection* ss = sh_->shape_section(sec_);
    if (!ss) {
        return false;
    }
    ss->get_coord(xloc_, x_, y_);
    if (i_ >= sh_->count() || sh_->component(i_) != (Glyph*) this) {
        i_ = sh_->glyph_index(this);
    }
    if (i_ < 0)
        return false;
    sh_->move(i_, x_, y_);

    return true;
}

void OcShape::transform3d(Rubberband* rot) {
    ShapeScene::transform3d(rot);
    if (point_mark_list_) {
        GlyphIndex i, cnt = point_mark_list_->count();
        for (i = 0; i < cnt; ++i) {
            ((PointMark*) point_mark_list_->component(i))->update(NULL);
        }
    }
}

ShapeChangeObserver::ShapeChangeObserver(ShapeScene* s) {
    s_ = s;  // do not ref
    shape_changed_ = nrn_shape_changed_;
    struc_changed_ = structure_change_cnt;
    Oc oc;
    oc.notify_attach(this);
}
ShapeChangeObserver::~ShapeChangeObserver() {
    Oc oc;
    oc.notify_detach(this);
}
void ShapeChangeObserver::update(Observable*) {
    if (shape_changed_ != nrn_shape_changed_) {
        // printf("ShapeChangeObserver::update shape_changed%p nrn_shape_changed=%d\n", this,
        // nrn_shape_changed_);
        shape_changed_ = nrn_shape_changed_;
        nrn_define_shape();
        volatile_ptr_ref = NULL;
        if (struc_changed_ != structure_change_cnt) {
            struc_changed_ = structure_change_cnt;
            // printf("ShapeChangeObserver::update structure_changed%p\n", this);
            if (s_->view_all()) {
                s_->observe();
            }
            shape_changed_ = 0;
        } else {
            s_->transform3d();
            shape_changed_ = nrn_shape_changed_;
            s_->flush();
        }
    }
}
void ShapeChangeObserver::force() {
    shape_changed_ = 0;
}
#endif  // HAVE_IV
