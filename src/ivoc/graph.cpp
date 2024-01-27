#include <../../nrnconf.h>

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

extern int hoc_return_type_code;

#if HAVE_IV
#include <InterViews/glyph.h>
#include <InterViews/hit.h>
#include <InterViews/event.h>
#include <InterViews/color.h>
#include <InterViews/brush.h>
#include <InterViews/window.h>
#include <InterViews/printer.h>
#include <InterViews/label.h>
#include <InterViews/font.h>
#include <InterViews/background.h>
#include <InterViews/style.h>
#include <InterViews/telltale.h>
#include <OS/string.h>
#include <InterViews/image.h>
extern Image* gif_image(const char*);


#include <IV-look/kit.h>

#include "apwindow.h"
#include "ivoc.h"
#include "graph.h"
#include "axis.h"
#include "hocmark.h"
#include "mymath.h"
#include "idraw.h"
#include "symchoos.h"
#include "scenepic.h"
#include "oc_ansi.h"
#include "oc2iv.h"
#include "objcmd.h"
#include "ocjump.h"
#include "utility.h"
#include "cbwidget.h"
#include "xmenu.h"
#include "ivocvect.h"
#endif /* HAVE_IV */

#include "classreg.h"
#include "gui-redirect.h"

#if HAVE_IV
#define Graph_Crosshair_           "Crosshair Graph"
#define Graph_Change_label_        "ChangeText Graph"
#define Graph_keep_lines_toggle_   "KeepLines Graph"
#define Graph_erase_axis_          "AxisType Graph"
#define Graph_new_axis_            "NewAxis AxisType Graph"
#define Graph_view_axis_           "ViewAxis AxisType Graph"
#define Graph_view_box_            "ViewBox AxisType Graph"
#define Graph_erase_lines_         "Erase Graph"
#define Graph_choose_sym_          "PlotWhat Graph"
#define Graph_choose_family_label_ "FamilyLabel Graph"
#define Graph_choose_rvp_          "PlotRange Graph"

bool GraphItem::is_polyline() {
    return false;
}
bool GPolyLineItem::is_polyline() {
    return true;
}
bool GraphItem::is_mark() {
    return false;
}


/*static*/ class GraphLabelItem: public GraphItem {
  public:
    GraphLabelItem(Glyph* g)
        : GraphItem(g) {}
    virtual ~GraphLabelItem(){};
    virtual void save(std::ostream& o, Coord x, Coord y) {
        ((GLabel*) body())->save(o, x, y);
    }
    virtual void erase(Scene* s, GlyphIndex i, int type) {
        if ((type & GraphItem::ERASE_LINE) && ((GLabel*) body())->erase_flag()) {
            s->remove(i);
        }
    };
};

/*static*/ class GraphAxisItem: public GraphItem {
  public:
    GraphAxisItem(Glyph* g)
        : GraphItem(g) {}
    virtual ~GraphAxisItem(){};
    virtual void save(std::ostream& o, Coord, Coord) {
        ((Axis*) body())->save(o);
    }
    virtual void erase(Scene* s, GlyphIndex i, int type) {
        if (type & GraphItem::ERASE_AXIS) {
            s->remove(i);
        }
    }
};

/*static*/ class GraphMarkItem: public GraphItem {
  public:
    GraphMarkItem(Glyph* g)
        : GraphItem(g) {}
    virtual ~GraphMarkItem(){};
    virtual void erase(Scene* s, GlyphIndex i, int type) {
        if (type & GraphItem::ERASE_LINE) {
            s->remove(i);
        }
    }
    virtual bool is_mark() {
        return true;
    }
};

/*static*/ class VectorLineItem: public GPolyLineItem {
  public:
    VectorLineItem(Glyph* g)
        : GPolyLineItem(g) {}
    virtual ~VectorLineItem(){};
    virtual void erase(Scene* s, GlyphIndex i, int type) {}
    virtual bool is_graphVector() {
        return true;
    }
};

/*static*/ class LineExtension: public Glyph {
  public:
    LineExtension(GPolyLine*);
    virtual ~LineExtension();

    void begin();
    void extend();
    void damage(Graph*);

    virtual void request(Requisition&) const;
    virtual void allocate(Canvas*, const Allocation&, Extension&);
    virtual void draw(Canvas*, const Allocation&) const;

    DataVec* xd() const {
        return (DataVec*) gp_->x_data();
    }
    DataVec* yd() const {
        return (DataVec*) gp_->y_data();
    }

  private:
    GPolyLine* gp_;
    int start_;
    int previous_;
};

/*static*/ class NewLabelHandler: public Handler {
  public:
    NewLabelHandler(Graph*, Coord, Coord);
    ~NewLabelHandler();
    virtual bool event(Event&);

  private:
    Graph* g_;
    Coord x_, y_;
};

NewLabelHandler::NewLabelHandler(Graph* g, Coord x, Coord y) {
    //	printf("NewLabelHandler\n");
    g_ = g;
    x_ = x;
    y_ = y;
}

NewLabelHandler::~NewLabelHandler() {
    //	printf("~NewLabelHandler\n");
}
bool NewLabelHandler::event(Event& e) {
    char buf[200];
    buf[0] = '\0';
    GLabel* gl = g_->new_proto_label();
    gl->ref();
    if (Graph::label_chooser("Enter new label", buf, gl, e.pointer_root_x(), e.pointer_root_y())) {
        if (gl->fixed()) {
            g_->fixed(gl->scale());
        } else {
            g_->vfixed(gl->scale());
        }
        if (g_->labeltype() == 2) {
            XYView::current_pick_view()->s2o().inverse_transform(x_, y_);
            XYView::current_pick_view()->ratio_view(x_, y_, x_, y_);
        }
        g_->label(x_, y_, buf);
    }
    gl->unref();
    return true;
}

// Graph registration for oc
static void gr_axis(Graph* g, DimensionName d) {
    IFGUI
    int ntic = -1;
    int nminor = 0;
    int invert = 0;
    bool number = true;
    float x1 = 0;
    float x2 = -1;
    float pos = 0.;
    if (!ifarg(2)) {
        int i = 0;
        if (ifarg(1)) {
            i = int(chkarg(1, 0, 3));
        }
        switch (i) {
        case 0:
            g->view_axis();
            break;
        case 1:
            g->erase_axis();
            g->axis(Dimension_X, x1, x2, pos, ntic, nminor, invert, number);
            g->axis(Dimension_Y, x1, x2, pos, ntic, nminor, invert, number);
            break;
        case 2:
            g->view_box();
            break;
        case 3:
            g->erase_axis();
            break;
        }
        return;
    }
    if (ifarg(3)) {
        pos = *getarg(3);
    }
    if (ifarg(4)) {
        ntic = int(chkarg(4, -1, 100));
    }
    if (ifarg(2)) {
        x1 = *getarg(1);
        x2 = *getarg(2);
    }
    if (ifarg(5)) {
        nminor = int(chkarg(5, 0, 100));
    }
    if (ifarg(6)) {
        invert = int(chkarg(6, -1, 1));
    }
    if (ifarg(7) && !int(chkarg(7, 0, 1))) {
        number = false;
    }
    g->axis(d, x1, x2, pos, ntic, nminor, invert, number);
    ENDGUI
}
#endif /* HAVE_IV */

static double gr_xaxis(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Graph.xaxis", v);
#if HAVE_IV
    gr_axis((Graph*) v, Dimension_X);
    return 1.;
#else
    return 0.;
#endif /* HAVE_IV  */
}
static double gr_yaxis(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Graph.yaxis", v);
#if HAVE_IV
    gr_axis((Graph*) v, Dimension_Y);
    return 1.;
#else
    return 0.;
#endif /* HAVE_IV  */
}
static double gr_save_name(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Graph.save_name", v);
#if HAVE_IV
    IFGUI
    Graph* g = (Graph*) v;
    g->name(gargstr(1));
    if (ifarg(2) && (chkarg(2, 0, 1) == 1.) && Oc::save_stream) {
        char buf[80];
        *Oc::save_stream << "{\nsave_window_=" << gargstr(1) << std::endl;
        *Oc::save_stream << "save_window_.size(" << g->x1() << "," << g->x2() << "," << g->y1()
                         << "," << g->y2() << ")\n";
        long i = Scene::scene_list_index(g);
        Sprintf(buf, "scene_vector_[%ld] = save_window_", i);
        *Oc::save_stream << buf << std::endl;
        g->save_phase2(*Oc::save_stream);
        g->Scene::mark(true);
    }
    ENDGUI
    return 1.;
#else
    return 0.;
#endif /* HAVE_IV  */
}
#if HAVE_IV

static void move_label(Graph* g, const GLabel* lab, int ioff = 0) {
    float x, y;
    if (ifarg(4 + ioff) && lab) {
        x = *getarg(4 + ioff);
        y = *getarg(5 + ioff);
        g->move(g->glyph_index(lab), x, y);
    }
}
#endif /* HAVE_IV  */

static double gr_family(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Graph.family", v);
#if HAVE_IV
    IFGUI
    if (hoc_is_str_arg(1)) {
        ((Graph*) v)->family(gargstr(1));
    } else {
        ((Graph*) v)->family(int(chkarg(1, 0, 1)));
    }
    ENDGUI
    return 1.;
#else
    return 0.;
#endif /* HAVE_IV  */
}

double ivoc_gr_menu_action(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Graph.menu_action", v);
#if HAVE_IV
    IFGUI
    HocCommand* hc;
    if (hoc_is_object_arg(2)) {
        hc = new HocCommand(*hoc_objgetarg(2));
    } else {
        hc = new HocCommand(gargstr(2));
    }
    ((Scene*) v)->picker()->add_menu(gargstr(1), new HocCommandAction(hc));
    ENDGUI
    return 1.;
#else
    return 0.;
#endif /* HAVE_IV  */
}

double ivoc_gr_menu_tool(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Graph.menu_tool", v);
#if HAVE_IV
    IFGUI
    if (hoc_is_object_arg(2)) {  // python style
        HocPanel::paneltool(gargstr(1),
                            NULL,
                            NULL,
                            ((Scene*) v)->picker(),
                            *hoc_objgetarg(2),
                            ifarg(3) ? *hoc_objgetarg(3) : NULL);
    } else {
        HocPanel::paneltool(gargstr(1),
                            gargstr(2),
                            ifarg(3) ? gargstr(3) : NULL,
                            ((Scene*) v)->picker());
    }
    ENDGUI
    return 1.;
#else
    return 0.;
#endif /* HAVE_IV  */
}

double ivoc_view_info(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Graph.view_info", v);
#if HAVE_IV
    IFGUI
    int i;
    Scene* s = (Scene*) v;
    XYView* view;
    if (!ifarg(1)) {
        view = XYView::current_pick_view();
        for (i = 0; i < s->view_count(); ++i) {
            if (s->sceneview(i) == view) {
                return double(i);
            }
        }
        return -1.;
    }
    view = s->sceneview(int(chkarg(1, 0, s->view_count() - 1)));
    float x1, y1, x2, y2;
    i = int(chkarg(2, 1, 15));
    switch (i) {
    case 1:  // width
        return view->width();
    case 2:  // height
        return view->height();
    case 3:  // point width
        view->view_ratio(0., 0., x1, y1);
        view->view_ratio(1., 1., x2, y2);
        return x2 - x1;
    case 4:  // point height
        view->view_ratio(0., 0., x1, y1);
        view->view_ratio(1., 1., x2, y2);
        return y2 - y1;
    case 5:  // left
        return view->left();
    case 6:  // right
        return view->right();
    case 7:  // bottom
        return view->bottom();
    case 8:  // top
        return view->top();
    case 9:  // model x distance for one point
        view->view_ratio(0., 0., x1, y1);
        view->view_ratio(1., 1., x2, y2);
        if (x2 > x1) {
            return view->width() / (x2 - x1);
        } else {
            return 1.;
        }
    case 10:  // model y distance for one point
        view->view_ratio(0., 0., x1, y1);
        view->view_ratio(1., 1., x2, y2);
        if (y2 > y1) {
            return view->height() / (y2 - y1);
        } else {
            return 1.;
        }
    case 11:  // relative x location (from x model coord)
        return (*getarg(3) - view->left()) / view->width();
    case 12:  // relative y location (from y model coord)
        return (*getarg(3) - view->bottom()) / view->height();
    case 13:  // points from left (from x model coord)
        x1 = (*getarg(3) - view->left()) / view->width();
        view->view_ratio(x1, 1., x2, y2);
        view->view_ratio(0., 1., x1, y1);
        return x2 - x1;
    case 14:  // points from top (from y model coord)
        y1 = (*getarg(3) - view->bottom()) / view->height();
        view->view_ratio(1., y1, x2, y2);
        view->view_ratio(1., 1., x1, y1);
        return y1 - y2;
    case 15:  // label height in points
        // return WidgetKit::instance()->font()->size();
        {
            FontBoundingBox b;
            WidgetKit::instance()->font()->font_bbox(b);
            return b.ascent() + b.descent();
        }
    }
    ENDGUI
#endif /* HAVE_IV  */
    return -1.;
}

double ivoc_view_size(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Graph.view_size", v);
#if HAVE_IV
    IFGUI
    int i;
    Scene* s = (Scene*) v;
    XYView* view;
    view = s->sceneview(int(chkarg(1, 0, s->view_count() - 1)));
    view->size(*getarg(2), *getarg(4), *getarg(3), *getarg(5));
    view->damage_all();
    ENDGUI
#endif /* HAVE_IV  */
    return 0.;
}

double gr_line_info(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Graph.line_info", v);
#if HAVE_IV
    IFGUI
    Graph* g = (Graph*) v;
    GlyphIndex i, cnt;
    double* p;
    cnt = g->count();
    i = (int) chkarg(1, -1, cnt);
    if (i < 0 || i > cnt - 1) {
        i = -1;
    }
    Vect* x = vector_arg(2);
    for (i += 1; i < cnt; ++i) {
        GraphItem* gi = (GraphItem*) g->component(i);
        if (gi->is_polyline()) {
            GPolyLine* gpl = (GPolyLine*) gi->body();
            x->resize(5);
            p = vector_vec(x);
            p[0] = colors->color(gpl->color());
            p[1] = brushes->brush(gpl->brush());
            if (gpl->label()) {
                Coord a, b;
                x->label(gpl->label()->text());
                g->location(g->glyph_index(gpl->label()), a, b);
                p[2] = a;
                p[3] = b;
                p[4] = gpl->label()->fixtype();
            }
            return (double) i;
        }
    }
    ENDGUI
#endif /* HAVE_IV  */
    return -1.;
}

#if HAVE_IV
static void gr_add(void* v, bool var) {
    IFGUI
    Graph* g = (Graph*) v;
    GraphLine* gl;
    Object* obj = NULL;
    char* lab = NULL;
    char* expr = NULL;
    int ioff = 0;  // deal with 0, 1, or 2 optional arguments after first
    // pointer to varname if second arg is varname string
    neuron::container::data_handle<double> pd{};
    int fixtype = g->labeltype();
    // organize args for backward compatibility and the new
    // addexpr("label, "expr", obj,.... style
    if (ifarg(2)) {
        if (var) {  // if string or address then variable and 1 was label
            expr = hoc_gargstr(1);
            if (hoc_is_str_arg(2)) {
                pd = hoc_val_handle(hoc_gargstr(2));
                ioff += 1;
            } else if (hoc_is_pdouble_arg(2)) {
                pd = hoc_hgetarg<double>(2);
                ioff += 1;
            }
        } else if (hoc_is_str_arg(2)) {  // 1 label, 2 expression
            lab = hoc_gargstr(1);
            expr = hoc_gargstr(2);
            ioff += 1;
            if (ifarg(3) && hoc_is_object_arg(3)) {  // object context
                obj = *hoc_objgetarg(3);
                ioff += 1;
            }
        } else if (hoc_is_object_arg(2)) {  // 1 expr, 2 object context
            expr = hoc_gargstr(1);
            obj = *hoc_objgetarg(2);
            ioff += 1;
        } else {
            expr = hoc_gargstr(1);
        }
    } else {
        expr = hoc_gargstr(1);
    }
    if (ifarg(3 + ioff)) {
        if (ifarg(6 + ioff)) {
            fixtype = int(chkarg(6 + ioff, 0, 2));
        } else if (ifarg(4 + ioff)) {
            // old versions did not have the fixtype and for
            // backward compatibility it must therefore be
            // fixed.
            fixtype = 1;
        }
        gl = g->add_var(expr,
                        colors->color(int(*getarg(2 + ioff))),
                        brushes->brush(int(*getarg(3 + ioff))),
                        var,
                        fixtype,
                        pd,
                        lab,
                        obj);
    } else {
        gl = g->add_var(expr, g->color(), g->brush(), var, fixtype, pd, lab, obj);
    }
    move_label(g, gl->label(), ioff);
    ENDGUI
}
#endif /* HAVE_IV */
static double gr_addvar(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Graph.addvar", v);
#if HAVE_IV
    gr_add(v, 1);
    return 1.;
#else
    return 0.;
#endif /* HAVE_IV  */
}
static double gr_addexpr(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Graph.addexpr", v);
#if HAVE_IV
    gr_add(v, 0);
    return 1.;
#else
    return 0.;
#endif /* HAVE_IV  */
}
static double gr_addobject(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Graph.addobject", v);
#if HAVE_IV
    IFGUI
    Graph* g = (Graph*) v;
    Object* obj = *hoc_objgetarg(1);
    if (is_obj_type(obj, "RangeVarPlot")) {
        GraphVector* gv = (GraphVector*) obj->u.this_pointer;
        if (ifarg(3)) {
            gv->color(colors->color(int(*getarg(2))));
            gv->brush(brushes->brush(int(*getarg(3))));
        } else {
            gv->color(g->color());
            gv->brush(g->brush());
        }
        g->append(new VectorLineItem(gv));
        GLabel* glab = g->label(gv->name());
        gv->label(glab);
        ((GraphItem*) g->component(g->glyph_index(glab)))->save(false);
        g->see_range_plot(gv);
        move_label(g, glab);
    } else {
        hoc_execerror("Don't know how to plot this object type", 0);
    }
    ENDGUI
    return 1.;
#else
    return 0.;
#endif /* HAVE_IV  */
}

static double gr_vector(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Graph.vector", v);
#if HAVE_IV
    IFGUI
    Graph* g = (Graph*) v;
    int n = int(chkarg(1, 1., 1.e5));
    double* x = hoc_pgetarg(2);
    auto y_handle = hoc_hgetarg<double>(3);
    GraphVector* gv = new GraphVector("");
    if (ifarg(4)) {
        gv->color(colors->color(int(*getarg(4))));
        gv->brush(brushes->brush(int(*getarg(5))));
    } else {
        gv->color(g->color());
        gv->brush(g->brush());
    }
    for (int i = 0; i < n; ++i) {
        gv->add(x[i], y_handle.next_array_element(i));
    }
    //	GLabel* glab = g->label(gv->name());
    //	((GraphItem*)g->component(g->glyph_index(glab)))->save(false);
    g->append(new GPolyLineItem(gv));
    ENDGUI
    return 1.;
#else
    return 0.;
#endif /* HAVE_IV  */
}

static double gr_xexpr(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Graph.xexpr", v);
#if HAVE_IV
    IFGUI
    Graph* g = (Graph*) v;
    int i = 0;
    if (ifarg(2)) {
        i = int(chkarg(2, 0, 1));
    }
    g->x_expr(gargstr(1), i);
    ENDGUI
    return 1.;
#else
    return 0.;
#endif /* HAVE_IV  */
}

static double gr_begin(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Graph.begin", v);
#if HAVE_IV
    IFGUI((Graph*) v)->begin();
    ENDGUI
    return 1.;
#else
    return 0.;
#endif /* HAVE_IV  */
}

static double gr_plot(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Graph.plot", v);
#if HAVE_IV
    IFGUI((Graph*) v)->plot(*getarg(1));
    ENDGUI
    return 1.;
#else
    return 0.;
#endif /* HAVE_IV  */
}

static double gr_simgraph(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Graph.simgraph", v);
#if HAVE_IV
    IFGUI((Graph*) v)->simgraph();
    ENDGUI
    return 1.;
#else
    return 0.;
#endif /* HAVE_IV  */
}

double ivoc_gr_begin_line(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Graph.beginline", v);
#if HAVE_IV
    IFGUI
    Graph* g = (Graph*) v;
    int i = 1;
    char* s = NULL;
    if (ifarg(i) && hoc_is_str_arg(1)) {
        s = gargstr(i++);
    }
    if (ifarg(i)) {
        g->begin_line(colors->color(int(*getarg(i))), brushes->brush(int(*getarg(i + 1))), s);
    } else {
        g->begin_line(s);
    }
    ENDGUI
    return 1.;
#else
    return 0.;
#endif /* HAVE_IV  */
}
double ivoc_gr_line(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Graph.line", v);
#if HAVE_IV
    IFGUI((Graph*) v)->line(*getarg(1), *getarg(2));
    ENDGUI
    return 1.;
#else
    return 0.;
#endif /* HAVE_IV  */
}
static double gr_flush(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Graph.flush", v);

#if HAVE_IV
    IFGUI((Graph*) v)->flush();
    ENDGUI
    return 1.;
#else
    return 0.;
#endif /* HAVE_IV  */
}
static double gr_fast_flush(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Graph.fast_flush", v);

#if HAVE_IV
    IFGUI((Graph*) v)->fast_flush();
    ENDGUI
    return 1.;
#else
    return 0.;
#endif /* HAVE_IV  */
}
double ivoc_gr_erase(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Graph.erase", v);
#if HAVE_IV
    IFGUI((Graph*) v)->erase_lines();
    ENDGUI
    return 1.;
#else
    return 0.;
#endif /* HAVE_IV  */
}

double ivoc_erase_all(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Graph.erase_all", v);
#if HAVE_IV
    IFGUI((Graph*) v)->erase_all();
    ENDGUI
    return 1.;
#else
    return 0.;
#endif /* HAVE_IV  */
}

double ivoc_gr_gif(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Graph.gif", v);
#if HAVE_IV
    IFGUI
    Graph* g = (Graph*) v;
    Glyph* i = gif_image(gargstr(1));
    if (i) {
        Transformer t;
        if (ifarg(4)) {
            Coord x = *getarg(4);
            Coord y = *getarg(5);
            Requisition r;
            i->request(r);
            t.scale(x / r.x_requirement().natural(), y / r.y_requirement().natural());
            i = new TransformSetter(i, t);
        }
        if (!ifarg(2)) {  // resize if smaller than gif
            Requisition r;
            i->request(r);
            if (r.x_requirement().natural() > (g->x2() - g->x1()) ||
                r.y_requirement().natural() > (g->y2() - g->y1())) {
                g->new_size(0, 0, r.x_requirement().natural(), r.y_requirement().natural());
            }
        }
        g->append(new GraphItem(i, false, false));
        if (ifarg(2)) {
            g->move(g->count() - 1, *getarg(2), *getarg(3));
        }
        return 1.;
    }
    ENDGUI
#endif /* HAVE_IV  */
    return 0.;
}

double ivoc_gr_size(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Graph.size", v);
#if HAVE_IV
    IFGUI
    Coord x1, y1, x2, y2;
    Graph* g = (Graph*) v;
    XYView* view = g->sceneview(0);
    if (ifarg(2)) {
        g->new_size(*getarg(1), *getarg(3), *getarg(2), *getarg(4));
    }
    if (hoc_is_pdouble_arg(1)) {
        g->wholeplot(x1, y1, x2, y2);
        double* x = hoc_pgetarg(1);
        x[0] = x1;
        x[1] = x2;
        x[2] = y1;
        x[3] = y2;
        return 0.;
    }
    if (!view) {
        return 0.;
    }

    if (ifarg(2)) {
        view->zout(x1, y1, x2, y2);
        view->size(x1, y1, x2, y2);
        return 1.;
    } else {
        view->zin(x1, y1, x2, y2);
        double x = 0.;
        switch (int(chkarg(1, 1., 4.))) {
        case 1:
            x = x1;
            break;
        case 2:
            x = x2;
            break;
        case 3:
            x = y1;
            break;
        case 4:
            x = y2;
            break;
        }
        return x;
    }
    ENDGUI
    return 0.;
#else
    return 0.;
#endif /* HAVE_IV  */
}

double ivoc_gr_label(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Graph.label", v);
#if HAVE_IV
    IFGUI
    Graph* g = (Graph*) v;
    if (ifarg(8)) {
        g->label(*getarg(1),
                 *getarg(2),
                 gargstr(3),
                 int(*getarg(4)),
                 *getarg(5),
                 *getarg(6),
                 *getarg(7),
                 colors->color(int(*getarg(8))));
    } else if (ifarg(2)) {
        char* s = NULL;
        if (ifarg(3)) {
            s = gargstr(3);
        }
        g->label(*getarg(1), *getarg(2), s);
    } else {
        g->label(gargstr(1));
    }
    ENDGUI
    return 1.;
#else
    return 0.;
#endif /* HAVE_IV  */
}
static double gr_fixed(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Graph.fixed", v);
#if HAVE_IV
    IFGUI
    float scale = 1;
    if (ifarg(1)) {
        scale = chkarg(1, .01, 100);
    }
    ((Graph*) v)->fixed(scale);
    ENDGUI
    return 1.;
#else
    return 0.;
#endif /* HAVE_IV  */
}
static double gr_vfixed(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Graph.vfixed", v);
#if HAVE_IV
    IFGUI
    float scale = 1;
    if (ifarg(1)) {
        scale = chkarg(1, .01, 100);
    }
    ((Graph*) v)->vfixed(scale);
    ENDGUI
    return 1.;
#else
    return 0.;
#endif /* HAVE_IV  */
}
static double gr_relative(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Graph.relative", v);
#if HAVE_IV
    IFGUI
    float scale = 1.;
    if (ifarg(1)) {
        scale = chkarg(1, .01, 100.);
    }
    ((Graph*) v)->relative(scale);
    ENDGUI
    return 1.;
#else
    return 0.;
#endif /* HAVE_IV  */
}
static double gr_align(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Graph.align", v);
#if HAVE_IV
    IFGUI
    float x = 0, y = 0;
    if (ifarg(1)) {
        x = chkarg(1, -10, 10);
    }
    if (ifarg(2)) {
        y = chkarg(2, -10, 10);
    }
    ((Graph*) v)->align(x, y);
    ENDGUI
    return 1.;
#else
    return 0.;
#endif /* HAVE_IV  */
}
static double gr_color(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Graph.color", v);
#if HAVE_IV
    IFGUI
    int i = 1;
    if (ifarg(2)) {
        colors->color(int(chkarg(1, 2, ColorPalette::COLOR_SIZE - 1)), gargstr(2));
        return 1.;
    }
    if (ifarg(1)) {
        i = int(chkarg(1, -1, ColorPalette::COLOR_SIZE - 1));
    }
    ((Graph*) v)->color(i);
    ENDGUI
    return 1.;
#else
    return 0.;
#endif /* HAVE_IV  */
}
static double gr_brush(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Graph.brush", v);
#if HAVE_IV
    IFGUI
    int i = 0;
    if (ifarg(3)) {
        brushes->brush(int(chkarg(1, 1, BrushPalette::BRUSH_SIZE - 1)),
                       int(*getarg(2)),
                       float(chkarg(3, 0, 1000)));
        return 1.;
    }
    if (ifarg(1)) {
        i = int(chkarg(1, -1, 20));
    }
    ((Graph*) v)->brush(i);
    ENDGUI
    return 1.;
#else
    return 0.;
#endif /* HAVE_IV  */
}

static double gr_view(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Graph.view", v);
#if HAVE_IV
    IFGUI
    Graph* g = (Graph*) v;
    if (ifarg(8)) {
        Coord x[8];
        int i;
        for (i = 0; i < 8; ++i) {
            x[i] = *getarg(i + 1);
        }
        XYView* view = new XYView(x[0], x[1], x[2], x[3], g, x[6], x[7]);
        Coord x1, x2, y1, y2;
        view->zout(x1, y1, x2, y2);
        view->size(x1, y1, x2, y2);
        // printf("%g %g %g %g\n", view->left(), view->bottom(), view->right(), view->top());
        ViewWindow* w = new ViewWindow(view, hoc_object_name(g->hoc_obj_ptr()));
        w->xplace(int(x[4]), int(x[5]));
        w->map();
    } else if (ifarg(1) && *getarg(1) == 2.) {
        View* view = new View(g);
        ViewWindow* w = new ViewWindow(view, hoc_object_name(g->hoc_obj_ptr()));
        w->map();
    }
    ENDGUI

    return 1.;
#else
    return 0.;
#endif /* HAVE_IV  */
}

double ivoc_gr_mark(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Graph.mark", v);
#if HAVE_IV
    IFGUI
    Graph* g = (Graph*) v;
    Coord x = *getarg(1);
    Coord y = *getarg(2);
    char style = '+';
    if (ifarg(3)) {
        if (hoc_is_str_arg(3)) {
            style = *gargstr(3);
        } else {
            style = char(chkarg(3, 0, 10));
        }
    }
    if (!ifarg(4)) {
        g->mark(x, y, style);
    } else if (!ifarg(5)) {
        g->mark(x, y, style, chkarg(4, .1, 100.), g->color(), g->brush());
    } else {
        g->mark(x,
                y,
                style,
                chkarg(4, .1, 100.),
                colors->color(int(*getarg(5))),
                brushes->brush(int(*getarg(6))));
    }
    ENDGUI
    return 1.;
#else
    return 0.;
#endif /* HAVE_IV  */
}

static double gr_view_count(void* v) {
    hoc_return_type_code = 1;  // integer
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Graph.view_count", v);
#if HAVE_IV
    int n = 0;
    IFGUI
    n = ((Scene*) v)->view_count();
    ENDGUI
    return double(n);
#else
    return 0.;
#endif /* HAVE_IV  */
}

static double gr_unmap(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Graph.unmap", v);
#if HAVE_IV
    IFGUI
    Graph* g = (Graph*) v;
    g->dismiss();
    ENDGUI
    return 0.;
#else
    return 0.;
#endif /* HAVE_IV  */
}

static double gr_set_cross_action(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Graph.crosshair_action", v);
#if HAVE_IV
    IFGUI
    Graph* g = (Graph*) v;
    bool vector_copy = false;
    if (ifarg(2)) {
        vector_copy = int(chkarg(2, 0, 1));
    }
    if (hoc_is_str_arg(1)) {
        g->set_cross_action(gargstr(1), NULL, vector_copy);
    } else {
        g->set_cross_action(NULL, *hoc_objgetarg(1), vector_copy);
    }
    ENDGUI
    return 0.;
#else
    return 0.;
#endif /* HAVE_IV  */
}

static double gr_printfile(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Graph.printfile", v);
#if HAVE_IV
    IFGUI
    Graph* g = (Graph*) v;
    g->printfile(gargstr(1));
    ENDGUI
    return 1.;
#else
    return 0.;
#endif /* HAVE_IV  */
}

static double exec_menu(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Graph.exec_menu", v);
#if HAVE_IV
    IFGUI((Scene*) v)->picker()->exec_item(gargstr(1));
    ENDGUI
#endif
    return 0.;
}

double ivoc_gr_menu_remove(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Graph.menu_remove", v);
#if HAVE_IV
    IFGUI((Scene*) v)->picker()->remove_item(gargstr(1));
    ENDGUI
#endif
    return 0.;
}

#if HAVE_IV
extern double gr_getline(void*);
#endif
extern double gr_addglyph(void*);

static Member_func gr_members[] = {{"plot", gr_plot},
                                   {"fastflush", gr_fast_flush},
                                   {"flush", gr_flush},
                                   {"xaxis", gr_xaxis},
                                   {"yaxis", gr_yaxis},
                                   {"addvar", gr_addvar},
                                   {"addexpr", gr_addexpr},
                                   {"addobject", gr_addobject},
                                   {"glyph", gr_addglyph},
                                   {"vector", gr_vector},
                                   {"xexpr", gr_xexpr},
                                   {"begin", gr_begin},
                                   {"erase", ivoc_gr_erase},
                                   {"size", ivoc_gr_size},
                                   {"label", ivoc_gr_label},
                                   {"fixed", gr_fixed},
                                   {"vfixed", gr_vfixed},
                                   {"relative", gr_relative},
                                   {"align", gr_align},
                                   {"color", gr_color},
                                   {"brush", gr_brush},
                                   {"view", gr_view},
                                   {"save_name", gr_save_name},
                                   {"beginline", ivoc_gr_begin_line},
                                   {"line", ivoc_gr_line},
                                   {"mark", ivoc_gr_mark},
                                   {"simgraph", gr_simgraph},
                                   {"view_count", gr_view_count},
                                   {"erase_all", ivoc_erase_all},
                                   {"unmap", gr_unmap},
                                   {"crosshair_action", gr_set_cross_action},
                                   {"printfile", gr_printfile},
                                   {"family", gr_family},
                                   {"menu_action", ivoc_gr_menu_action},
                                   {"menu_tool", ivoc_gr_menu_tool},
                                   {"view_info", ivoc_view_info},
                                   {"view_size", ivoc_view_size},
#if HAVE_IV
                                   {"getline", gr_getline},
#endif
                                   {"exec_menu", exec_menu},
                                   {"gif", ivoc_gr_gif},
                                   {"menu_remove", ivoc_gr_menu_remove},
                                   {"line_info", gr_line_info},
                                   {0, 0}};

static void* gr_cons(Object* ho) {
    TRY_GUI_REDIRECT_OBJ("Graph", NULL);
#if HAVE_IV
    Graph* g = NULL;
    IFGUI
    int i = 1;
    if (ifarg(1)) {
        i = (int) chkarg(1, 0, 1);
    }
    g = new Graph(i);
    g->ref();
    g->hoc_obj_ptr(ho);
    ENDGUI
    return (void*) g;
#else
    return (void*) 0;
#endif /* HAVE_IV  */
}
static void gr_destruct(void* v) {
    TRY_GUI_REDIRECT_NO_RETURN("~Graph", v);
#if HAVE_IV
    IFGUI
    Graph* g = (Graph*) v;
    g->dismiss();
    Resource::unref(g);
    ENDGUI
#endif /* HAVE_IV */
}
void Graph_reg() {
    // printf("Graph_reg\n");
    class2oc("Graph", gr_cons, gr_destruct, gr_members, NULL, NULL, NULL);
#if HAVE_IV
    IFGUI
    colors = new ColorPalette();
    brushes = new BrushPalette();
    ENDGUI
#endif
}
#if HAVE_IV
static const char* colorname[] =
    {"white", "black", "red", "blue", "green", "orange", "brown", "violet", "yellow", "gray", 0};

ColorPalette::ColorPalette() {
    int i;
    for (i = 0; i < COLOR_SIZE && colorname[i]; ++i) {
        color_palette[i] = NULL;
        color(i, colorname[i]);
    }

    color(0, Scene::default_background());
    color(1, Scene::default_foreground());

    int j;
    for (j = 0; i < COLOR_SIZE; ++i, ++j) {
        //		color_palette[i] = color_palette[j];
        // ZFM fix to allow more colors (COLOR_SIZE > 20)
        color_palette[i] = color_palette[j % 10];
        Resource::ref(color_palette[i]);
    }
};
ColorPalette::~ColorPalette() {
    for (int i = 0; i < COLOR_SIZE; ++i) {
        Resource::unref(color_palette[i]);
    }
}

const Color* ColorPalette::color(int i) const {
    IFGUI
    if (i < 0)
        i = 1;
    i = i % COLOR_SIZE;
    return color_palette[i];
    ENDGUI else return NULL;
}
const Color* ColorPalette::color(int i, const char* name) {
    const Color* c = Color::lookup(Session::instance()->default_display(), name);
    if (c == NULL) {
        printf(
            "couldn't lookup color \"%s\", you must be missing the\n\
colormap.ini file or else the name isn't in it\n",
            name);
    }
    return color(i, c);
}
const Color* ColorPalette::color(int i, const Color* c) {
    if (c) {
        Resource::ref(c);
        Resource::unref(color_palette[i]);
        color_palette[i] = c;
    }
    return color_palette[i];
}

int ColorPalette::color(const Color* c) const {
    for (int i = 0; i < COLOR_SIZE; ++i) {
        if (color_palette[i] == c) {
            return i;
        }
    }
    return 1;
}

static int brush_pattern[] = {0x0, 0xcccc, 0xfff0, 0xff00, 0xf000};

BrushPalette::BrushPalette() {
    int i;
    for (i = 0; i < BRUSH_SIZE; ++i) {
        brush_palette[i] = NULL;
    }
    i = 0;
    for (int j = 0; j < 5; ++j) {
        for (int k = 0; k < 5; ++k) {
            if (i < BRUSH_SIZE) {
                brush(i++, brush_pattern[j], k);
            }
        }
    }
}
BrushPalette::~BrushPalette() {
    for (int i = 0; i < BRUSH_SIZE; ++i) {
        Resource::unref(brush_palette[i]);
    }
}

const Brush* BrushPalette::brush(int i) const {
    IFGUI
    if (i < 0)
        i = 1;
    i = i % BRUSH_SIZE;
    return brush_palette[i];
    ENDGUI else return NULL;
}
const Brush* BrushPalette::brush(int i, int pattern, Coord width) {
    Brush* b;
    if (pattern) {
        b = new Brush(pattern, width);
    } else {
        b = new Brush(width);
    }
    Resource::ref(b);
    Resource::unref(brush_palette[i]);
    brush_palette[i] = b;
    return b;
}

int BrushPalette::brush(const Brush* b) const {
    for (int i = 0; i < BRUSH_SIZE; ++i) {
        if (brush_palette[i] == b) {
            return i;
        }
    }
    return 0;
}


ColorPalette* colors;
BrushPalette* brushes;

GraphItem::GraphItem(Glyph* g, bool s, bool p)
    : MonoGlyph(g) {
    save_ = s;
    pick_ = p;
}
GraphItem::~GraphItem() {}
void GraphItem::save(std::ostream&, Coord, Coord) {}
void GraphItem::erase(Scene*, GlyphIndex, int) {}
void GraphItem::pick(Canvas* c, const Allocation& a, int depth, Hit& h) {
    if (pick_) {
        MonoGlyph::pick(c, a, depth, h);
    }
}

// Graph
declareActionCallback(Graph);
implementActionCallback(Graph);

#define XSCENE 300.
#define YSCENE 200.
Graph::Graph(bool b)
    : Scene(0, 0, XSCENE, YSCENE) {
    loc_ = 0;
    x_expr_ = NULL;
    x_pval_ = {};
    rvp_ = NULL;
    cross_action_ = NULL;
    vector_copy_ = false;
    family_on_ = false;
    family_label_ = NULL;
    family_cnt_ = 0;
    current_polyline_ = NULL;
    label_fixtype_ = 2;
    label_scale_ = 1.;
    label_x_align_ = 0;
    label_y_align_ = 0;
    label_x_ = 0;
    label_y_ = 1;
    label_n_ = 0;
    picker();
    picker()->bind_select((OcHandler*) NULL);
    MenuItem* mi = picker()->add_radio_menu("Crosshair", (OcHandler*) NULL, CROSSHAIR);
    mi->state()->set(TelltaleState::is_chosen, true);
    tool(CROSSHAIR);
    picker()->add_menu("Plot what?", new ActionCallback(Graph)(this, &Graph::choose_sym));
    picker()->add_radio_menu("Pick Vector", (OcHandler*) NULL, PICK);
    picker()->add_radio_menu("Color/Brush", new ActionCallback(Graph)(this, &Graph::change_prop));
#if 1
    WidgetKit& wk = *WidgetKit::instance();
    Menu* m = wk.pullright();
    picker()->add_menu("View Axis", new ActionCallback(Graph)(this, &Graph::view_axis), m);
    picker()->add_menu("New Axis", new ActionCallback(Graph)(this, &Graph::new_axis), m);
    picker()->add_menu("View Box", new ActionCallback(Graph)(this, &Graph::view_box), m);
    picker()->add_menu("Erase Axis", new ActionCallback(Graph)(this, &Graph::erase_axis), m);
    mi = K::menu_item("Axis Type");
    mi->menu(m);
    picker()->add_menu(mi);
#else
    picker()->add_menu("New Axis", new ActionCallback(Graph)(this, &Graph::new_axis));
#endif
    mi = WidgetKit::instance()->check_menu_item("Keep Lines");
    mi->action(new ActionCallback(Graph)(this, &Graph::keep_lines_toggle));
    keep_lines_toggle_ = mi->state();
    keep_lines_toggle_->ref();
    picker()->add_menu("Keep Lines", mi);
    picker()->add_menu("Family Label?",
                       new ActionCallback(Graph)(this, &Graph::family_label_chooser));
    picker()->add_menu("Erase", new ActionCallback(Graph)(this, &Graph::erase_lines));
    picker()->add_radio_menu("Move Text", (OcHandler*) NULL, Scene::MOVE);
    picker()->add_radio_menu("Change Text", (OcHandler*) NULL, Graph::CHANGELABEL);
    picker()->add_radio_menu("Delete", (OcHandler*) NULL, Scene::DELETE);
    sc_ = NULL;
    if (!colors) {
        colors = new ColorPalette();
    }
    if (!brushes) {
        brushes = new BrushPalette();
    }
    color_ = NULL;
    color(1);
    brush_ = NULL;
    brush(1);
    x_ = new DataVec(200);
    x_->ref();
    extension_flushed_ = true;
    symlist_ = NULL;
    if (b) {
        XYView* v = new XYView((Scene*) this, XSCENE, YSCENE);
        Window* w = new ViewWindow(v, "Graph");
        w->map();
    }
    long i = 3;
    if (WidgetKit::instance()->style()->find_attribute("graph_axis_default", i)) {
        switch (i) {
        case 0:
            view_axis();
            break;
        case 2:
            view_box();
            break;
        }
    }
}

Graph::~Graph() {
    // printf("~Graph\n");
    for (auto& item: line_list_) {
        Resource::unref(item);
    }
    Resource::unref(keep_lines_toggle_);
    Resource::unref(x_);
    hoc_free_list(&symlist_);
    Resource::unref(color_);
    Resource::unref(brush_);
    Resource::unref(rvp_);
    Resource::unref(sc_);
    Resource::unref(current_polyline_);
    Resource::unref(family_label_);
    if (cross_action_) {
        delete cross_action_;
    }
}

void Graph::name(char* s) {
    var_name_ = s;
}

void Graph::help() {
    switch (tool()) {
    case CROSSHAIR:
        Oc::help(Graph_Crosshair_);
        break;
    case CHANGELABEL:
        Oc::help(Graph_Change_label_);
        break;
    default:
        Scene::help();
        break;
    }
}

void Graph::delete_label(GLabel* glab) {
    GraphLine* glin = nullptr;
    auto it = std::find_if(line_list_.begin(), line_list_.end(), [&](const auto& e) {
        return e->label() == glab;
    });
    if (it != line_list_.end()) {
        glin = *it;
        line_list_.erase(it);
        glin->unref();
        GlyphIndex index = glyph_index(glin);
        remove(index);
    }
    if (!glin) {  // but possibly a vector line
        for (GlyphIndex index = 0; index < count(); ++index) {
            GraphItem* gi = (GraphItem*) component(index);
            if (gi->is_polyline()) {
                GPolyLine* gpl = (GPolyLine*) gi->body();
                if (gpl->label() == glab) {
                    remove(index);
                    break;
                }
            }
        }
    }
    GlyphIndex index = glyph_index(glab);
    remove(index);
}

GLabel* Graph::new_proto_label() const {
    return new GLabel("", color_, label_fixtype_, label_scale_, label_x_align_, label_y_align_);
}

bool Graph::change_label(GLabel* glab, const char* text, GLabel* gl) {
    if (strcmp(glab->text(), text)) {
        for (auto& line: line_list_) {
            if (line->label() == glab) {
                if (!line->change_expr(text, &symlist_)) {
                    return false;
                }
            }
        }
        glab->text(text);
    }
    GlyphIndex i = glyph_index(glab);
    if (glab->fixtype() != gl->fixtype()) {
        if (gl->fixed()) {
            glab->fixed(gl->scale());
            change_to_fixed(i, XYView::current_pick_view());
        } else {
            glab->vfixed(gl->scale());
            change_to_vfixed(i, XYView::current_pick_view());
        }
    }
    change(i);
    return true;
}

void Graph::change_label_color(GLabel* glab) {
    glab->color(color());
    damage(glyph_index(glab));
    if (glab->labeled_line()) {
        glab->labeled_line()->brush(brush());
        damage(glyph_index(glab->labeled_line()));
    }
}

void Graph::change_line_color(GPolyLine* glin) {
    glin->color(color());
    glin->brush(brush());
    damage(glyph_index(glin));
    if (glin->label()) {
        damage(glyph_index(glin->label()));
    }
}

GlyphIndex Graph::glyph_index(const Glyph* gl) {
    GlyphIndex cnt = count();
    for (GlyphIndex i = 0; i < cnt; ++i) {
        Glyph* g = ((GraphItem*) component(i))->body();
        if (g == gl) {
            return i;
        }
    }
    return -1;
}

std::ostream* Graph::ascii_;
SymChooser* Graph::fsc_;

void Graph::ascii(std::ostream* o) {
    ascii_ = o;
}
std::ostream* Graph::ascii() {
    return ascii_;
}

void Graph::draw(Canvas* c, const Allocation& a) const {
    // if (!extension_flushed_) {
    Scene::draw(c, a);
    //}
    if (extension_flushed_) {
        for (auto& item: line_list_) {
            item->extension()->draw(c, a);
        }
    }
    if (ascii_) {
        ascii_save(*ascii_);
    }
}

void Graph::ascii_save(std::ostream& o) const {
    long line, lcnt = line_list_.size();
    int i, dcnt;
    if (lcnt == 0 || !x_ || family_label_) {
        // tries to print in matrix form is labels and each line the same
        // size.
        o << "PolyLines" << std::endl;
        if (x_expr_) {
            o << "x expression: " << x_expr_->name;
        }
        if (lcnt) {
            o << lcnt << " addvar/addexpr lines:";
            for (const auto& line: line_list_) {
                o << " " << line->name();
            }
            o << std::endl;
        }


        lcnt = count();
        // check to see if all y_data has same count and a label.
        // If so print as matrix. (Assumption that all x_data same is
        // dangerous.)
        bool matrix_form = true;
        int col = 0;
        int xcnt = 0;
        const DataVec* xvec = NULL;
        for (i = 0; i < lcnt; ++i) {
            GraphItem* gi = (GraphItem*) component(i);
            if (gi->is_polyline()) {
                GPolyLine* gpl = (GPolyLine*) gi->body();
                if (gpl->label() && (xcnt == 0 || gpl->x_data()->count() == xcnt)) {
                    xcnt = gpl->x_data()->count();
                    xvec = gpl->x_data();
                    if (gpl->y_data()->count() == xcnt) {
                        ++col;
                    }
                } else {
                    matrix_form = false;
                    break;
                }
            }
        }
        if (matrix_form) {
            if (x_expr_) {
                o << x_expr_->name;
            } else {
                o << "x";
            }
            for (i = 0; i < lcnt; ++i) {
                GraphItem* gi = (GraphItem*) component(i);
                if (gi->is_polyline()) {
                    GPolyLine* gpl = (GPolyLine*) gi->body();
                    if (gpl->y_data()->count() == xcnt) {
                        o << " " << gpl->label()->text();
                    }
                }
            }
            o << std::endl;
            o << xcnt << " rows, " << col + 1 << " columns" << std::endl;
            int j;
            for (j = 0; j < xcnt; ++j) {
                o << xvec->get_val(j);
                for (i = 0; i < lcnt; ++i) {
                    GraphItem* gi = (GraphItem*) component(i);
                    if (gi->is_polyline()) {
                        GPolyLine* gpl = (GPolyLine*) gi->body();
                        if (gpl->y_data()->count() == xcnt) {
                            o << "\t" << gpl->y(j);
                        }
                    }
                }
                o << std::endl;
            }
        }
        if (!matrix_form) {
            o << "Line Manifest:" << std::endl;
            for (i = 0; i < lcnt; ++i) {
                GraphItem* gi = (GraphItem*) component(i);
                if (gi->is_polyline()) {
                    GPolyLine* gpl = (GPolyLine*) gi->body();
                    int j, jcnt;
                    jcnt = gpl->y_data()->count();
                    if (jcnt && family_label_ && gpl->label()) {
                        o << jcnt << "  " << family_label_->text() << "=" << gpl->label()->text()
                          << std::endl;
                    } else {
                        o << jcnt << std::endl;
                    }
                }
            }
            o << "End of Line Manifest" << std::endl;
            for (i = 0; i < lcnt; ++i) {
                GraphItem* gi = (GraphItem*) component(i);
                if (gi->is_polyline()) {
                    GPolyLine* gpl = (GPolyLine*) gi->body();
                    int j, jcnt;
                    jcnt = gpl->y_data()->count();
                    if (jcnt && family_label_ && gpl->label()) {
                        o << jcnt << "  " << family_label_->text() << "=" << gpl->label()->text()
                          << std::endl;
                    } else {
                        o << jcnt << std::endl;
                    }
                    for (j = 0; j < jcnt; ++j) {
                        o << gpl->x(j) << "\t" << gpl->y(j) << "\n";
                    }
                }
            }
        }
        return;
    }
    o << "Graph addvar/addexpr lines" << std::endl;
    o << lcnt << " " << x_->count() << std::endl;
    if (x_expr_) {
        o << x_expr_->name;
    } else {
        o << "x";
    }
    for (const auto& item: line_list_) {
        o << " " << item->name();
    }
    o << std::endl;
    dcnt = x_->count();
    for (i = 0; i < dcnt; ++i) {
        o << x_->get_val(i);
        for (const auto& item: line_list_) {
            o << "\t" << item->y(i);
        }
        o << std::endl;
    }
    // print the remaining unlabeled polylines. i.e. saved with KeepLines
    lcnt = count();
    int n = 0;
    for (i = 0; i < lcnt; ++i) {
        GraphItem* gi = (GraphItem*) component(i);
        if (gi->is_polyline()) {
            GPolyLine* gpl = (GPolyLine*) gi->body();
            if (!gpl->label()) {
                ++n;
            }
        }
    }
    o << n << " unlabeled lines" << std::endl;
    for (i = 0; i < lcnt; ++i) {
        GraphItem* gi = (GraphItem*) component(i);
        if (gi->is_polyline()) {
            GPolyLine* gpl = (GPolyLine*) gi->body();
            if (!gpl->label()) {
                int n = gpl->x_data()->count();
                o << n << std::endl;
                int j;
                for (j = 0; j < n; ++j) {
                    o << gpl->x(j) << "\t" << gpl->y(j) << std::endl;
                }
            }
        }
    }
}

void Graph::pick(Canvas* c, const Allocation& a, int depth, Hit& h) {
#if 1
    Scene::pick(c, a, depth, h);
    if (tool() == CHANGELABEL && !menu_picked() && h.event() && h.event()->type() == Event::down &&
        h.event()->pointer_button() == Event::left && h.count() < 2) {
        h.target(depth, this, 0, new NewLabelHandler(this, h.left(), h.bottom()));
    }
#else
    if (h.event() && h.event()->type() == Event::down) {
        if (h.event()->pointer_button() == Event::right) {
            h.target(depth, this, 0, new RubberRect(c, new NewView(this)));
        } else if (h.event()->pointer_button() == Event::middle) {
            choose_sym(c->window());
        } else {
            Scene::pick(c, a, depth, h);
        }
    } else {
        Scene::pick(c, a, depth, h);
    }
#endif
}

void Graph::new_size(Coord x1, Coord y1, Coord x2, Coord y2) {
    Scene::new_size(x1, y1, x2, y2);
    if (label_fixtype_ == 1) {
        label_x_ = x2 - .2 * (x2 - x1);
        label_y_ = y2 - .1 * (y2 - y1);
    } else if (label_fixtype_ == 2) {
        label_x_ = .8;
        label_y_ = .9;
    }
    label_n_ = 0;
}

void Graph::wholeplot(Coord& l, Coord& b, Coord& r, Coord& t) const {
    GlyphIndex i, cnt;
    GraphLine* gl;
    l = b = 1e9;
    r = t = -1e9;
    cnt = count();
    for (i = 0; i < cnt; ++i) {
        GraphItem* gi = (GraphItem*) component(i);
        if (gi->is_polyline()) {
            GPolyLine* gpl = (GPolyLine*) gi->body();
            if (gpl->x_data()->count() > 1) {
                l = std::min(l, gpl->x_data()->min());
                b = std::min(b, gpl->y_data()->min());
                r = std::max(r, gpl->x_data()->max());
                t = std::max(t, gpl->y_data()->max());
            }
        }
        if (gi->is_mark()) {
            Coord x, y;
            location(i, x, y);
            l = std::min(l, x);
            b = std::min(b, y);
            r = std::max(r, x);
            t = std::max(t, y);
        }
    }
    if (l >= r || b >= t) {
        Coord x1, y1, x2, y2;
        Scene::wholeplot(x1, y1, x2, y2);
        if (l >= r) {
            l = x1;
            r = x2;
        }
        if (b >= t) {
            b = y1;
            t = y2;
        }
    }
    if (t > 1e30) {
        t = 1e30;
    }
    if (b < -1e30) {
        t = -1e30;
    }
    return;
}

void Graph::axis(DimensionName d,
                 float x1,
                 float x2,
                 float pos,
                 int ntic,
                 int nminor,
                 int invert,
                 bool number) {
    // printf("%d %g %g %g %d %d %d %d %g\n", d, x1, x2, pos, ntic, nminor, invert, number);
    Axis* a;
    if (x2 < x1) {
        a = new Axis(this, d);
    } else if (ntic < 0) {
        a = new Axis(this, d, x1, x2);
    } else {
        a = new Axis(this, d, x1, x2, pos, ntic, nminor, invert, number);
    }
    append_fixed(new GraphAxisItem(a));
}
void Graph::color(int i) {
    const Color* color = colors->color(i);
    Resource::ref(color);
    Resource::unref(color_);
    color_ = color;
}
void Graph::brush(int i) {
    const Brush* b = brushes->brush(i);
    Resource::ref(b);
    Resource::unref(brush_);
    brush_ = b;
}
GraphLine* Graph::add_var(const char* expr,
                          const Color* color,
                          const Brush* brush,
                          bool usepointer,
                          int fixtype,
                          neuron::container::data_handle<double> pd,
                          const char* lab,
                          Object* obj) {
    GraphLine* gl = new GraphLine(expr, x_, &symlist_, color, brush, usepointer, pd, obj);
    GLabel* glab;
    if (lab) {
        glab = label(lab, fixtype);
    } else {
        glab = label(expr, fixtype);
    }
    GlyphIndex i = glyph_index(glab);
    ((GraphItem*) component(i))->save(false);
    glab->color(color);
    gl->label(glab);
    line_list_.push_back(gl);
    gl->ref();
    Scene::append(new GPolyLineItem(gl));
    return gl;
}

void Graph::add_polyline(GPolyLine* gp) {
    Scene::append(new GPolyLineItem(gp));
}

void Graph::add_graphVector(GraphVector* gv) {
    Scene::append(new VectorLineItem(gv));
}

void Graph::x_expr(const char* expr, bool usepointer) {
    Oc oc;
    x_expr_ = oc.parseExpr(expr, &symlist_);
    if (!x_expr_) {
        hoc_execerror(expr, "not an expression");
    }
    if (usepointer) {
        x_pval_ = hoc_val_handle(expr);
        if (!x_pval_) {
            hoc_execerror(expr, "is invalid left hand side of assignment statement");
        }
    } else {
        x_pval_ = {};
    }
}

extern int hoc_execerror_messages;
void Graph::begin() {
    if (keep_lines_toggle_->test(TelltaleState::is_chosen)) {
        keep_lines();
        family_value();
    }
    int hem = hoc_execerror_messages;
    for (auto& gl: line_list_) {
        gl->erase();
        if (family_on_) {
            ((GPolyLine*) gl)->color(color());
            ((GPolyLine*) gl)->brush(brush());
        }
        hoc_execerror_messages = false;
        if (gl->valid(true) == false) {
            printf("Graph:: presently invalid expression: %s\n", gl->name());
        }
    }
    hoc_execerror_messages = hem;
    x_->erase();
    extension_start();
}
void Graph::plot(float x) {
    if (extension_flushed_) {
        extension_continue();
    }
    if (x_expr_) {
        if (x_pval_) {
            x_->add(float(*x_pval_));
        } else {
            Oc oc;
            x_->add(oc.runExpr(x_expr_));
        }
    } else {
        x_->add(x);
    }
    for (auto& item: line_list_) {
        item->plot();
    }
}
void Graph::begin_line(const char* s) {
    begin_line(color_, brush_, s);
}
void Graph::begin_line(const Color* c, const Brush* b, const char* s) {
    Resource::unref(current_polyline_);
    current_polyline_ = new GPolyLine(new DataVec(2), c, b);
    Resource::ref(current_polyline_);
    if (s && s[0]) {
        GLabel* glab = label(s);
        current_polyline_->label(glab);
        ((GraphItem*) component(glyph_index(glab)))->save(false);
    }
    Scene::append(new GPolyLineItem(current_polyline_));
}
void Graph::line(Coord x, Coord y) {
    if (!current_polyline_) {
        begin_line();
    }
    current_polyline_->plot(x, y);
}
void Graph::flush() {
    extension_start();
    long i, cnt = count();
    for (i = 0; i < cnt; ++i) {
        modified(i);
    }
    //	damage_all();//too conservative. plots everything every time
}
void Graph::fast_flush() {
    for (auto& item: line_list_) {
        item->extension()->damage(this);
    }
    extension_flushed_ = true;
}

void Graph::extension_start() {
    x_->running_start();
    for (auto& item: line_list_) {
        item->extension_start();
    }
    extension_flushed_ = false;
}
void Graph::extension_continue() {
    x_->running_start();
    for (auto& item: line_list_) {
        item->extension_continue();
    }
    extension_flushed_ = false;
}

void Graph::mark(Coord x, Coord y, char style, float size, const Color* c, const Brush* b) {
    HocMark* m = HocMark::instance(style, size, c, b);
    append_fixed(new GraphMarkItem(m));
    move(Scene::count() - 1, x, y);
}

void Graph::set_cross_action(const char* cp, Object* pyact, bool vector_copy) {
    if (cross_action_) {
        delete cross_action_;
        cross_action_ = NULL;
    }
    if (cp && strlen(cp) > 0) {
        cross_action_ = new HocCommand(cp);
    } else if (pyact) {
        cross_action_ = new HocCommand(pyact);
    }
    vector_copy_ = vector_copy;
}

void Graph::cross_action(char c, GPolyLine* gpl, int i) {
    if (cross_action_) {
        char buf[256];
        if (vector_copy_) {
            Object* op1 = *(gpl->x_data()->new_vect());
            Object* op2 = *(gpl->y_data()->new_vect(gpl->label()));
            hoc_pushx(double(i));
            hoc_pushx(double(c));
            hoc_push_object(op1);
            hoc_push_object(op2);
            cross_action_->func_call(4);
            hoc_obj_unref(op1);
            hoc_obj_unref(op2);
        } else {
            hoc_pushx(double(gpl->x_data()->get_val(i)));
            hoc_pushx(double(gpl->y_data()->get_val(i)));
            hoc_pushx(double(c));
            cross_action_->func_call(3);
        }
    } else {
        printf("{x=%g y=%g}\n", gpl->x(i), gpl->y(i));
    }
}

void Graph::cross_action(char c, Coord x, Coord y) {
    if (cross_action_) {
        char buf[256];
        if (vector_copy_) {
        } else {
            Sprintf(buf, "%s(%g, %g, %d)", cross_action_->name(), x, y, c);
            cross_action_->execute(buf);
        }
    } else {
        printf("{x=%g y=%g}\n", x, y);
    }
}
void Graph::erase() {
    for (auto& item: line_list_) {
        item->erase();
    }
    damage_all();
}

void Graph::erase_all() {
    for (int i = count() - 1; i >= 0; --i) {
        remove(i);
    }
    for (auto& item: line_list_) {
        Resource::unref(item);
    }
    line_list_.clear();
    line_list_.shrink_to_fit();
    label_n_ = 0;
}
void Graph::family_value() {
    if (family_label_) {
        char buf[256];
        Sprintf(buf, "hoc_ac_ = %s\n", family_label_->text());
        Oc oc;
        oc.run(buf);
        family_val_ = hoc_ac_;
    }
}

void Graph::keep_lines_toggle() {
    if (Oc::helpmode()) {
        Oc::help(Graph_keep_lines_toggle_);
        keep_lines_toggle_->set(TelltaleState::is_chosen,
                                !keep_lines_toggle_->test(TelltaleState::is_chosen));
        return;
    }
    family_value();
    if (!keep_lines_toggle_->test(TelltaleState::is_chosen)) {  // keep the ones already there
        keep_lines();
    }
}

void Graph::keep_lines() {
    char buf[256];
    int fi;
    Coord x, y;
    GLabel* f = family_label_;
    if (f) {
        fi = glyph_index(f);
        location(fi, x, y);
        Sprintf(buf, "%g", family_val_);
    }
    long lcnt = count();
    for (long i = lcnt - 1; i >= 0; --i) {
        GraphItem* gi = (GraphItem*) component(i);
        if (gi->is_polyline()) {
            GPolyLine* gpl = (GPolyLine*) gi->body();
            if (gpl->keepable() && gpl->y_data()->count() > 1) {
                GPolyLine* g2 = new GPolyLine(new DataVec(gpl->x_data()),
                                              new DataVec(gpl->y_data()),
                                              gpl->color(),
                                              gpl->brush());

                if (f) {
                    GLabel* gl =
                        label(x, y, buf, f->fixtype(), f->scale(), 0, family_cnt_, gpl->color());
                    family_cnt_++;
                    g2->label(gl);
                    ((GraphItem*) component(glyph_index(gl)))->save(false);
                }
                Scene::insert(i, new GPolyLineItem(g2));
                modified(i);
                gpl->erase();
            }
        }
    }
    flush();
}

void Graph::family(bool i) {
    if (i) {
        erase_lines();
        family_on_ = true;
        keep_lines_toggle_->set(TelltaleState::is_chosen, true);
    } else {
        family_on_ = false;
        keep_lines_toggle_->set(TelltaleState::is_chosen, false);
        for (auto& gl: line_list_) {
            gl->color(gl->save_color());
            gl->brush(gl->save_brush());
        }
    }
}

void Graph::family(const char* s) {
    if (family_label_) {
        if (s && s[1]) {
            family_label_->text(s);
            modified(glyph_index(family_label_));
        } else {
            remove(glyph_index(family_label_));
            family_label_->unref();
            family_label_ = NULL;
        }
    } else if (s && s[1]) {
        family_label_ = label(.95, .95, s, 2, 1, 1, 0, color_);
        family_label_->ref();
        ((GraphItem*) component(glyph_index(family_label_)))->save(false);
    }
}

void Graph::erase_axis() {
    if (Oc::helpmode()) {
        Oc::help(Graph_erase_axis_);
        return;
    }
    GlyphIndex i, cnt;
    cnt = count();
    for (i = cnt - 1; i >= 0; --i) {
        ((GraphItem*) component(i))->erase(this, i, GraphItem::ERASE_AXIS);
    }
    Scene::background();
    damage_all();
}

void Graph::new_axis() {
    if (Oc::helpmode()) {
        Oc::help(Graph_new_axis_);
        return;
    }
    XYView* v = XYView::current_pick_view();
    erase_axis();
    Coord x1{}, x2{}, y1{}, y2{};
    if (v) {
        v->zin(x1, y1, x2, y2);
    }
    Axis* a = new Axis(this, Dimension_X, x1, x2);
    append_fixed(new GraphAxisItem(a));
    a = new Axis(this, Dimension_Y, y1, y2);
    append_fixed(new GraphAxisItem(a));
}

void Graph::view_axis() {
    if (Oc::helpmode()) {
        Oc::help(Graph_view_axis_);
        return;
    }
    erase_axis();
    Scene::background(new AxisBackground());
    damage_all();
}

void Graph::view_box() {
    if (Oc::helpmode()) {
        Oc::help(Graph_view_box_);
        return;
    }
    erase_axis();
    Scene::background(new BoxBackground());
    damage_all();
}

#if 0
void Graph::spec_axis() {
	XYView* v = XYView::current_pick_view();
	Coord x1, x2, y1, y2;
	v->zin(x1, y1, x2, y2);
	bool bx = var_pair_chooser("X-Axis", x1, x2);
	bool by = var_pair_chooser("Y-Axis", y1, y2);
	v->size(x1, y1, x2, y2);
	erase_axis();
	if (bx) {
		Axis* a = new Axis(this, Dimension_X, x1, x2);
		append_fixed(new GraphAxisItem(a));
	}	
	if (by) {
		Axis* a = new Axis(this, Dimension_Y, y1, y2);
		append_fixed(new GraphAxisItem(a));
	}	
}
#endif

void Graph::erase_lines() {
    if (Oc::helpmode()) {
        Oc::help(Graph_erase_lines_);
        return;
    }
    // when labels are attached to lines some get erased and some do not
    // the issue invalid pointers is also a problem (delete when removed
    // from scene which screws up the GlyphIndex iterators. For this reason
    // we just unshow all gpolyline labels,
    // then show the labels for line list and GraphVector then
    // remove all unshow.
    GlyphIndex i, cnt = count();
    for (i = 0; i < cnt; ++i) {
        GraphItem* gi = (GraphItem*) component(i);
        if (gi->is_polyline() && !gi->is_graphVector()) {
            GLabel* gl = ((GPolyLine*) (gi->body()))->label();
            if (gl) {
                gl->erase_flag(true);
            }
        }
    }
    for (auto& gl: line_list_) {
        gl->label()->erase_flag(false);
    }
    cnt = count();
    for (i = cnt - 1; i >= 0; --i) {
        ((GraphItem*) component(i))->erase(this, i, GraphItem::ERASE_LINE);
    }
    for (auto& gl: line_list_) {
        Scene::append(new GPolyLineItem(gl));
    }
    erase();
    if (family_label_) {
        family_cnt_ = 0;
    }
}
GLabel* Graph::label(float x,
                     float y,
                     const char* s,
                     int fixtype,
                     float scale,
                     float x_align,
                     float y_align,
                     const Color* color) {
    GLabel* l = new GLabel(s, color, fixtype, scale, x_align, y_align);
    if (fixtype == 1) {
        append_fixed(new GraphLabelItem(l));
    } else if (fixtype == 2) {
        append_viewfixed(new GraphLabelItem(l));
    } else if (fixtype == 0) {
        append(new GraphLabelItem(l));
    }
    move(count() - 1, x, y);
    return l;
}
GLabel* Graph::label(float x, float y, const char* s, float n, int fixtype) {
    label_x_ = x;
    label_y_ = y;
    label_n_ = n;
    if (!s) {
        return NULL;
    }
    return label(x,
                 y,
                 s,
                 (fixtype != -1) ? fixtype : label_fixtype_,
                 label_scale_,
                 label_x_align_,
                 label_y_align_ + label_n_,
                 color_);
}
GLabel* Graph::label(const char* s, int fixtype) {
    label_n_ += 1.;
    return label(label_x_, label_y_, s, label_n_, fixtype);
}
void Graph::fixed(float scale) {
    label_fixtype_ = 1;
    label_scale_ = scale;
}
void Graph::vfixed(float scale) {
    label_fixtype_ = 2;
    label_scale_ = scale;
}
void Graph::relative(float fraction) {
    label_fixtype_ = 0;
    label_scale_ = fraction;
}
void Graph::align(float x, float y) {
    label_x_align_ = x;
    label_y_align_ = y;
}

void Graph::see_range_plot(GraphVector* rvp) {
    if (rvp_) {
        rvp_->unref();
    }
    rvp_ = rvp;
    Resource::ref(rvp);
}

void Graph::save_phase1(std::ostream& o) {
    o << "{" << std::endl;
    save_class(o, "Graph");
}

static Graph* current_save_graph;

void Graph::save_phase2(std::ostream& o) {
    char buf[256];
    if (family_label_) {
        Sprintf(buf, "save_window_.family(\"%s\")", family_label_->text());
        o << buf << std::endl;
    }
    if (!var_name_.empty()) {
        if (var_name_.back() == '.') {
            Sprintf(buf, "%sappend(save_window_)", var_name_.c_str());
        } else {
            Sprintf(buf, "%s = save_window_", var_name_.c_str());
        }
        o << buf << std::endl;
        Sprintf(buf, "save_window_.save_name(\"%s\")", var_name_.c_str());
        o << buf << std::endl;
    }
    if (x_expr_) {
        Sprintf(buf, "save_window_.xexpr(\"%s\", %d)", x_expr_->name, x_pval_ ? 1 : 0);
        o << buf << std::endl;
    }
    long cnt = count();
    current_save_graph = this;
    for (long i = 0; i < cnt; ++i) {
        GraphItem* g = (GraphItem*) component(i);
        Coord x, y;
        location(i, x, y);
        if (g->save()) {
            g->save(o, x, y);
        }
    }
    o << "}" << std::endl;
}


bool GraphVector::choose_sym(Graph*) {
    // it is a range variable plot where we get a different result
    return false;
}

void Graph::choose_sym() {
    Oc oc;
    if (Oc::helpmode()) {
        if (rvp_) {
            Oc::help(Graph_choose_rvp_);
        } else {
            Oc::help(Graph_choose_sym_);
        }
    }
    if (rvp_ && rvp_->choose_sym((Graph*) this)) {
        return;
    }
    if (!sc_) {
        Style* style = new Style(Session::instance()->style());
        style->attribute("caption", "Variable to graph");
        sc_ = new SymChooser(NULL, WidgetKit::instance(), style);
        sc_->ref();
    }
    Window* w = NULL;
    XYView* v = XYView::current_pick_view();
    if (!v || v->scene() != (Scene*) this || !v->canvas() || !v->canvas()->window()) {
        if (view_count() > 0 && sceneview(0)->canvas() && sceneview(0)->canvas()->window()) {
            w = sceneview(0)->canvas()->window();
        }
    } else {
        w = v->canvas()->window();
    }
    while ((w && sc_->post_for_aligned(w, .5, 1.)) || (!w && sc_->post_at(300, 300))) {
        char buf[256];
        double* pd = sc_->selected_var();
        neuron::container::data_handle<double> pd_handle{pd};

        if (sc_->selected_vector_count()) {
            Sprintf(buf, "%s", sc_->selected().c_str());
            GraphVector* gv = new GraphVector(buf);
            gv->color(color());
            gv->brush(brush());
            int n = sc_->selected_vector_count();
            for (int i = 0; i < n; ++i) {
                gv->add(double(i), pd_handle.next_array_element(i));
            }
            GLabel* glab = label(gv->name());
            ((GraphItem*) component(glyph_index(glab)))->save(false);
            gv->label(glab);
            append(new VectorLineItem(gv));
            flush();
            break;
        } else if (pd) {
            add_var(sc_->selected().c_str(), color(), brush(), 1, 2);
            break;
        } else {
            auto s = sc_->selected();
            // above required due to bug in mswindows version in which
            // sc_->selected seems volatile under some kinds of hoc
            // executions.
            Sprintf(buf, "hoc_ac_ = %s\n", s.c_str());
            if (oc.run(buf) == 0) {
                add_var(s.c_str(), color(), brush(), 0, 2);
                break;
            }
            hoc_warning(s.c_str(), "is not an expression.");
        }
    }
    //	sc_->unref();
}

void Graph::family_label_chooser() {
    Oc oc;
    if (Oc::helpmode()) {
        Oc::help(Graph_choose_family_label_);
    }
    if (!fsc_) {
        Style* style = new Style(Session::instance()->style());
        style->attribute("caption", "Family label variable");
        fsc_ = new SymChooser(NULL, WidgetKit::instance(), style);
        fsc_->ref();
    }
    while (fsc_->post_for_aligned(XYView::current_pick_view()->canvas()->window(), .5, 1.)) {
        char buf[256];
        Sprintf(buf, "hoc_ac_ = %s\n", fsc_->selected().c_str());
        if (oc.run(buf) == 0) {
            family(fsc_->selected().c_str());
            break;
        }
        hoc_warning(sc_->selected().c_str(), "is not an expression.");
    }
}

// GraphLine and GPolyLine
GraphLine::GraphLine(const char* expr,
                     DataVec* x,
                     Symlist** symlist,
                     const Color* c,
                     const Brush* b,
                     bool usepointer,
                     neuron::container::data_handle<double> pd,
                     Object* obj)
    : GPolyLine(x, c, b) {
    Oc oc;
    valid_ = true;
    obj_ = NULL;
    simgraph_x_sav_ = NULL;
    if (usepointer) {
        if (pd) {
            // char buf[256];
            // Sprintf(buf, "%s", expr);
            // expr_ = oc.parseExpr(buf, symlist);
            expr_ = nullptr;
            pval_ = pd;
        } else {
            expr_ = oc.parseExpr(expr, symlist);
            pval_ = hoc_val_handle(expr);
            if (!pval_) {
                hoc_execerror(expr, "is invalid left hand side of assignment statement");
            }
        }
        neuron::container::notify_when_handle_dies(pval_, this);
    } else {
        if (obj) {
            obj_ = obj;
            oc.notify_when_freed((void*) obj_, this);
            ObjectContext objc(obj_);
            expr_ = oc.parseExpr(expr, symlist);
        } else {
            expr_ = oc.parseExpr(expr, symlist);
        }
        pval_ = {};
    }
    if (!pval_ && !expr_) {
        hoc_execerror(expr, "not an expression");
    }
    save_color_ = c;
    Resource::ref(c);
    save_brush_ = b;
    Resource::ref(b);
    extension_ = new LineExtension(this);
    extension_->ref();
    keepable_ = true;
}

GPolyLine::GPolyLine(DataVec* x, const Color* c, const Brush* b) {
    init(x, new DataVec(x->size()), c, b);
}

GPolyLine::GPolyLine(DataVec* x, DataVec* y, const Color* c, const Brush* b) {
    init(x, y, c, b);
}

GPolyLine::GPolyLine(GPolyLine* gp) {
    init(new DataVec(gp->x_data()), new DataVec(gp->y_data()), gp->color(), gp->brush());
}

void GPolyLine::init(DataVec* x, DataVec* y, const Color* c, const Brush* b) {
    keepable_ = false;
    glabel_ = NULL;
    x_ = x;
    x_->ref();
    y_ = y;
    y_->ref();
    color_ = NULL;
    color(c);
    brush_ = NULL;
    brush(b);
    Resource::ref(b);
}

void GPolyLine::pick_vector() {
    Object* op1 = *(x_data()->new_vect());
    Object* op2 = *(y_data()->new_vect(label()));
    hoc_obj_set(1, op1);
    hoc_obj_set(0, op2);
    hoc_obj_unref(op1);
    hoc_obj_unref(op2);
}

extern void graphLineRecDeleted(GraphLine*);

GraphLine::~GraphLine() {
    // expr_ deleted when its symlist is deleted
    //	printf("~GraphLine %s\n", name());
    simgraph_activate(false);
    graphLineRecDeleted(this);
    Resource::unref(extension_);
    Oc oc;
    if (pval_ || obj_) {
        // printf("~graphline notify disconnect\n");
        oc.notify_pointer_disconnect(this);
    }
}

void GraphLine::simgraph_activate(bool act_) {
    if (act_) {
        if (!simgraph_x_sav_) {
            simgraph_x_sav_ = x_;
            x_ = new DataVec(x_->size());
            Resource::ref(x_);
            // printf("simgraph activate %s x_->size = %d\n", name(), x_->size());
        }
    } else {
        if (simgraph_x_sav_) {
            Resource::unref(x_);
            x_ = simgraph_x_sav_;
            simgraph_x_sav_ = NULL;
            // printf("simgraph inactivate %s x_->size = %d\n", name(), x_->size());
        }
    }
}

void GraphLine::simgraph_init() {
    x_->erase();
    erase();
}

void GraphLine::simgraph_continuous(double tt) {
    x_->add(tt);
    plot();
}

void GraphLine::update(Observable*) {  // *pval_ has been freed
                                       // printf("GraphLine::update pval_ has been freed\n");
    pval_ = {};
    if (obj_) {
        expr_ = NULL;
    }
    obj_ = NULL;
}

bool GraphLine::change_expr(const char* expr, Symlist** symlist) {
    Oc oc;
    if (pval_ || obj_) {
        printf("Can't change.\n");
        return false;
    }
    Symbol* sym = oc.parseExpr(expr, symlist);
    if (sym) {
        expr_ = sym;
        if (pval_) {
            // we are no longer interested in updates to pval_
            nrn_notify_pointer_disconnect(this);
            pval_ = {};
        }
        return true;
    } else {
        return false;
    }
}

void GPolyLine::label(GLabel* l) {
    Resource::ref(l);
    if (l && l->gpl_ && l->gpl_->label()) {
        l->gpl_->label(NULL);
    }
    if (glabel_) {
        glabel_->gpl_ = NULL;
    }
    Resource::unref(glabel_);
    glabel_ = l;
    if (glabel_) {
        glabel_->color(color());
        glabel_->gpl_ = (GPolyLine*) this;
    }
}

GPolyLine::~GPolyLine() {
    Resource::unref(color_);
    Resource::unref(brush_);
    Resource::unref(x_);
    Resource::unref(y_);
    label(NULL);
}
void GPolyLine::erase_line(Scene* s, GlyphIndex i) {
    GLabel* gl = label();
    s->remove(i);
    if (gl) {
        s->remove(s->glyph_index(gl));
    }
}
const char* GraphLine::name() const {
    Oc oc;
    if (label()) {
        return label()->text();
    } else if (expr_) {
        return oc.name(expr_);
    } else {
        return "no name";
    }
}

void GraphLine::extension_start() {
    extension_->begin();
}

void GraphLine::extension_continue() {
    extension_->extend();
}

void GraphLine::save(std::ostream& o) {
    char buf[256];
    float x, y;
    if (!label()) {
        return;
    }
    GlyphIndex i = current_save_graph->glyph_index(label());
    current_save_graph->location(i, x, y);
    if (pval_) {
        Sprintf(buf,
                "save_window_.addvar(\"%s\", %d, %d, %g, %g, %d)",
                name(),
                colors->color(color_),
                brushes->brush(brush_),
                x,
                y,
                label()->fixtype());
    } else {
        // following is not exactly correct if the label or object args were
        // used but it is expected that in that case the graph is
        // encapsulated in an object and this info is incorrect anyway.
        // Can revisit later if this is a problem.
        Sprintf(buf,
                "save_window_.addexpr(\"%s\", %d, %d, %g, %g, %d)",
                name(),
                colors->color(color_),
                brushes->brush(brush_),
                x,
                y,
                label()->fixtype());
    }
    o << buf << std::endl;
}

void GPolyLine::save(std::ostream&) {}

void GPolyLine::label_loc(Coord& x, Coord& y) const {
    if (label()) {
        GlyphIndex i = current_save_graph->glyph_index(label());
        current_save_graph->location(i, x, y);
    } else {
        x = 0.;
        y = 0.;
    }
}

void GPolyLine::request(Requisition& req) const {
    // printf("GPolyLine::request\n");
    Coord x, span;
    const float eps = 1e-4;
    x = x_->min();
    span = x_->max() - x + eps;
    x = (span > 0) ? x / span : 0;
    Requirement rx(span, 0, 0, -x);
    x = y_->min();
    span = y_->max() - x + eps;
    x = (span > 0) ? x / span : 0;
    Requirement ry(span, 0, 0, -x);
    req.require_x(rx);
    req.require_y(ry);
}
void GPolyLine::allocate(Canvas* c, const Allocation& a, Extension& e) {
    // printf("GPolyLine::allocate\n");
    e.set(c, a);
    MyMath::extend(e, brush()->width() / 2 + 1);
}
void GPolyLine::draw(Canvas* c, const Allocation& a) const {
    draw_specific(c, a, 0, y_->count());
}

void GPolyLine::draw_specific(Canvas* c, const Allocation&, int begin, int end) const {
    // printf("GPolyLine::draw %d %g %g\n", y_->count(), a.x(), a.y());
    int i, cnt = end;
    if (cnt - begin < 2) {
        return;
    }
#if 0
	Coord x1, y1, x2, y2;
	XYView::current_draw_view()->damage_area(x1, y1, x2, y2);

#define GPIN(arg) MyMath::inside(x_->get_val(arg), y_->get_val(arg), x1, y1, x2, y2)

	/* this works most of the time in preventing extraneous lines during
		very large zoom but can fail */
	for (i=begin; i > 0; --i) { // begin plotting outside of damage
		if (!GPIN(i)) {
			break;
		}
	}
	for (; i < cnt; ++i) {
		if (GPIN(i)) {
			if (i > 0) {
				--i;
			}
			break;
		}
	}
	int j;
	for (j=cnt-1 ; i < j; --j) {
		if (GPIN(j)) {
			if (j < cnt-1) {
				++j;
			}
			break;
		}
	}
	cnt = j + 1;
	if (cnt - i < 2) {
		return;
	}
#else
    i = begin;
#endif

    // xwindows limited to 65000 point polylines and mswindows
    // limited even more. So split into max 8000 point polylines for drawing
    // with large fonts on windows 98 there is a 6000 point limit so
    // change to 4000. If the problem recurs we will need to have a property
    // option
    long cnt1;
    while (i < cnt) {
#ifdef WIN32
        cnt1 = i + 4000;
#else
        cnt1 = i + 8000;
#endif
        if (cnt1 > cnt - 2) {  // the -2 prevents a one point final polyline
            cnt1 = cnt;
        }
        c->new_path();
        c->move_to(x_->get_val(i), y_->get_val(i));
        for (++i; i < cnt1; ++i) {
            c->line_to(x_->get_val(i), y_->get_val(i));
        }
        c->stroke(color_, brush_);
    }
    IfIdraw(mline(c, cnt, x_->vec(), y_->vec(), color_, brush_));
}

void GPolyLine::print(Printer* c, const Allocation&) const {
    int i, cnt = y_->count();
    if (cnt < 2) {
        return;
    }
#if 1
    float xmax, xmin, ymax, ymin;
    XYView* v = XYView::current_draw_view();
    xmax = v->right();
    xmin = v->left();
    ymax = v->top();
    ymin = v->bottom();

    /* this works most of the time in preventing extraneous lines during
        very large zoom but can fail */
    for (i = 0; i < cnt; ++i) {
        if (MyMath::inside(x_->get_val(i), y_->get_val(i), xmin, ymin, xmax, ymax)) {
            if (i > 0) {
                --i;
            }
            break;
        }
    }
    int j;
    for (j = cnt - 1; i < j; --j) {
        if (MyMath::inside(x_->get_val(j), y_->get_val(j), xmin, ymin, xmax, ymax)) {
            if (j < cnt - 1) {
                ++j;
            }
            break;
        }
    }
    cnt = j + 1;
    if (cnt - i < 2) {
        return;
    }
#else
    i = 0;
#endif

    const Transformer& t = XYView::current_draw_view()->s2o();
    // Scene::view_transform((Canvas*)c, 1, t); //2 would keep fixed width
    // line even after scaling by Print Window Manager

    c->new_path();
    c->move_to(x_->get_val(i), y_->get_val(i));
#if 0
	for (++i; i < cnt; ++i) {
		c->line_to(x_->get_val(i), y_->get_val(i));
	}
// some printers can't take very long lines
// from alain@helmholtz.sdsc.edu
#else
    char counter = 0;
    for (++i; i < cnt; ++i) {
        c->line_to(x_->get_val(i), y_->get_val(i));
        if (!++counter) {
            c->push_transform();
            c->transform(t);
            c->stroke(color_, brush_);
            c->pop_transform();
            c->new_path();
            c->move_to(x_->get_val(i), y_->get_val(i));
        }
    }
#endif
    c->push_transform();
    c->transform(t);
    c->stroke(color_, brush_);
    c->pop_transform();
}

void GraphLine::plot() {
    if (pval_) {
        y_->add(*pval_);
    } else {
        Oc oc;
        nrn_hoc_lock();
        if (obj_) {
            ObjectContext obc(obj_);
            y_->add(oc.runExpr(expr_));
        } else if (valid()) {
            y_->add(oc.runExpr(expr_));
        }
        nrn_hoc_unlock();
    }
    // printf("GPolyLine::plot(%d) value = %g\n", loc, y_->value(loc));
}

bool GraphLine::valid(bool check) {
    if (check && !pval_) {
        Oc oc;
        valid_ = oc.valid_expr(expr_);
    }
    return valid_;
}

void GPolyLine::plot(Coord x, Coord y) {
    x_->add(x);
    y_->add(y);
}

void GPolyLine::color(const Color* col) {
    const Color* c = col;
    if (!c) {
        c = colors->color(1);
    }
    Resource::ref(c);
    Resource::unref(color_);
    color_ = c;
    if (glabel_ && glabel_->color() != color()) {
        glabel_->color(color());
    }
}

void GPolyLine::brush(const Brush* brush) {
    const Brush* b = brush;
    if (!b) {
        b = brushes->brush(1);
    }
    Resource::ref(b);
    Resource::unref(brush_);
    brush_ = b;
}

void GraphLine::save_color(const Color* color) {
    const Color* c = color;
    if (!c) {
        c = colors->color(1);
    }
    Resource::ref(c);
    Resource::unref(color_);
    save_color_ = c;
    GPolyLine::color(c);
}

void GraphLine::save_brush(const Brush* brush) {
    const Brush* b = brush;
    if (!b) {
        b = brushes->brush(1);
    }
    Resource::ref(b);
    Resource::unref(brush_);
    save_brush_ = b;
    GPolyLine::brush(b);
}

// GLabel
GLabel::GLabel(const char* s,
               const Color* color,
               int fixtype,
               float size,
               float x_align,
               float y_align) {
    gpl_ = NULL;
    WidgetKit& kit = *WidgetKit::instance();
    label_ = new Label(s, kit.font(), color);
    label_->ref();
    erase_flag_ = false;
    color_ = color;
    color_->ref();
    text_ = s;
    if (fixtype == 2) {
        vfixed(size);
    } else if (fixtype == 1) {
        fixed(size);
    } else {
        relative(size);
    }
    align(x_align, y_align);
}
GLabel::~GLabel() {
    //	printf("~GLabel %s\n", text());
    Resource::unref(label_);
    Resource::unref(color_);
    assert(!labeled_line());
}

Glyph* GLabel::clone() const {
    return new GLabel(text_.c_str(), color_, fixtype_, scale_, x_align_, y_align_);
}

void GLabel::save(std::ostream& o, Coord x, Coord y) {
    if (labeled_line()) {
        return;
    }
    char buf[256];
    Sprintf(buf,
            "save_window_.label(%g, %g, \"%s\", %d, %g, %g, %g, %d)",
            x,
            y,
            text_.c_str(),
            fixtype_,
            scale_,
            x_align_,
            y_align_,
            colors->color(color_));
    o << buf << std::endl;
}

void GLabel::fixed(float scale) {
    fixtype_ = 1;
    scale_ = scale;
}
void GLabel::vfixed(float scale) {
    fixtype_ = 2;
    scale_ = scale;
}
void GLabel::relative(float scale) {
    fixtype_ = 0;
    scale_ = scale;
}
void GLabel::align(float x, float y) {
    x_align_ = x;
    y_align_ = y;
}
void GLabel::color(const Color* c) {
    Resource::unref(label_);
    WidgetKit& kit = *WidgetKit::instance();
    label_ = new Label(text_.c_str(), kit.font(), c);
    label_->ref();
    Resource::ref(c);
    Resource::unref(color_);
    color_ = c;
    if (gpl_ && gpl_->color() != color()) {
        gpl_->color(color());
    }
}

void GLabel::text(const char* t) {
    Resource::unref(label_);
    WidgetKit& kit = *WidgetKit::instance();
    text_ = t;
    label_ = new Label(text_.c_str(), kit.font(), color_);
    label_->ref();
}

void GLabel::request(Requisition& req) const {
    label_->request(req);
    Requirement& rx = req.x_requirement();
    Requirement& ry = req.y_requirement();
    rx.natural(rx.natural() * scale_);
    ry.natural(ry.natural() * scale_);
    // printf("ry.alignment=%g\n", ry.alignment());
    rx.alignment(x_align_);
    ry.alignment(y_align_ + ry.alignment());
}
void GLabel::allocate(Canvas* c, const Allocation& a, Extension& e) {
    e.set(c, a);
}

void GLabel::draw(Canvas* c, const Allocation& a1) const {
    // printf("GLabel::draw\n");
    Transformer t;
    Coord width = a1.x_allotment().span();
    Coord height = a1.y_allotment().span();
    Coord x = a1.x() - width * x_align_;
    Coord y = a1.y() - height * y_align_;
    // printf("x=%g y=%g\n", x, y);
    Allotment ax(0, width, 0);
    Allotment ay(0, height, 0);
    Allocation a2;
    a2.allot_x(ax);
    a2.allot_y(ay);

    // printf("xend = %g, yend=%g\n", a2.right(), a2.top());
    c->push_transform();
    t.scale(scale_, scale_);
    t.translate(x, y);
    c->transform(t);
    // float a00, a01, a10,a11,a20,a21;
    // c->transformer().matrix(a00,a01,a10,a11,a20,a21);
    // printf("transformer %g %g %g %g %g %g\n", a00, a01, a10, a11, a20, a21);
    label_->draw(c, a2);
    c->pop_transform();
    IfIdraw(text(c, text_.c_str(), t, NULL, color()));
}

// DataVec------------------

DataVec::DataVec(int size) {
    y_ = new float[size];
    y_[0] = 0.;
    size_ = size;
    count_ = 0;
    iMinLoc_ = iMaxLoc_ = -1;
    running_min_loc_ = running_max_loc_ = -1;
}

DataVec::DataVec(const DataVec* v) {
    size_ = v->size_;
    y_ = new float[size_];
    count_ = v->count_;
    y_[0] = 0.;
    for (int i = 0; i < count_; ++i) {
        y_[i] = v->y_[i];
    }
    iMinLoc_ = v->iMinLoc_;
    iMaxLoc_ = v->iMaxLoc_;
    running_min_loc_ = v->running_min_loc_;
    running_max_loc_ = v->running_max_loc_;
}

DataVec::~DataVec() {
    delete[] y_;
}

void DataVec::running_start() {
    if (count_) {
        running_min_loc_ = running_max_loc_ = count_ - 1;
    } else {
        running_min_loc_ = running_max_loc_ = 0;
    }
}

void DataVec::add(float x) {
    if (count_ == size_) {
        size_ *= 2;
        float* y = new float[size_];
        for (int i = 0; i < count_; i++) {
            y[i] = y_[i];
        }
        delete[] y_;
        y_ = y;
    }
    if (x > 1e30) {
        x = 1e32;
    } else if (x < -1e32) {
        x = -1e32;
    }
    y_[count_] = x;
    if (running_min_loc_ >= 0) {
        if (x < get_val(running_min_loc_)) {
            running_min_loc_ = count_;
        }
        if (x > get_val(running_max_loc_)) {
            running_max_loc_ = count_;
        }
    }
    ++count_;
    iMinLoc_ = iMaxLoc_ = -1;
}

float DataVec::max() const {
    return get_val(loc_max());
}
float DataVec::min() const {
    return get_val(loc_min());
}

float DataVec::running_max() {
    if (running_max_loc_ < 0) {
        return max();
    } else {
        return get_val(running_max_loc_);
    }
}

float DataVec::running_min() {
    if (running_min_loc_ < 0) {
        return min();
    } else {
        return get_val(running_min_loc_);
    }
}

int DataVec::loc_max() const {
    DataVec* dv = (DataVec*) this;
    if (iMaxLoc_ < 0) {
        int i;
        float m;
        for (i = 0, dv->iMaxLoc_ = 0, m = y_[i++]; i < count_; i++) {
            if (m < y_[i]) {
                m = y_[i];
                dv->iMaxLoc_ = i;
            }
        }
    }
    return iMaxLoc_;
}

int DataVec::loc_min() const {
    DataVec* dv = (DataVec*) this;
    if (iMinLoc_ < 0) {
        int i;
        float m;
        for (i = 0, dv->iMinLoc_ = 0, m = y_[i++]; i < count_; i++) {
            if (m > y_[i]) {
                m = y_[i];
                dv->iMinLoc_ = i;
            }
        }
    }
    return iMinLoc_;
}

float DataVec::max(int low, int high) {
    int imax = loc_max();
    if (imax >= low && imax < high) {
        return get_val(imax);
    }
    float m;
    for (m = y_[low++]; low < high; low++) {
        if (m < y_[low]) {
            m = y_[low];
        }
    }
    return m;
}

float DataVec::min(int low, int high) {
    int imin = loc_min();
    if (imin >= low && imin < high) {
        return get_val(imin);
    }
    float m;
    for (m = y_[low++]; low < high; low++) {
        if (m > y_[low]) {
            m = y_[low];
        }
    }
    return m;
}

void DataVec::erase() {
    count_ = 0;
    iMinLoc_ = iMaxLoc_ = -1;
    running_min_loc_ = running_max_loc_ = -1;
}

void DataVec::write() {
#if 0
	cout << get_name() << std::endl;
	cout << count_ << std::endl;
	for (int i=0; i<count_; i++) {
		cout << y_[i] << std::endl;
	}
#endif
}

GraphVector::GraphVector(const char* name, const Color* color, const Brush* brush)
    : GPolyLine(new DataVec(50), color, brush) {
    dp_ = new DataPointers();
    dp_->ref();
    name_ = name;
    keepable_ = true;
    disconnect_defer_ = false;
    record_install();
}
GraphVector::~GraphVector() {
    Oc oc;
    oc.notify_pointer_disconnect(this);
    dp_->unref();
    record_uninstall();
}

const char* GraphVector::name() const {
    return name_.c_str();
}

void GraphVector::save(std::ostream&) {}

void GraphVector::begin() {
    dp_->erase();
    y_->erase();
    x_->erase();
}

static double zero;

void GraphVector::update(Observable*) {
    // cant notify_pointer_disconnect from here, it will screw up list
    disconnect_defer_ = true;
    begin();
}

void GraphVector::add(float x, neuron::container::data_handle<double> py) {
    if (disconnect_defer_) {
        Oc oc;
        oc.notify_pointer_disconnect(this);
        disconnect_defer_ = false;
    }
    // Dubious
    if (dp_->count() == 0 ||
        static_cast<double*>(py) != static_cast<double*>(dp_->p(dp_->count() - 1)) + 1) {
        neuron::container::notify_when_handle_dies(py, this);
    }
    x_->add(x);
    if (!py) {
        py = {neuron::container::do_not_search, &zero};
    }
    y_->add(*py);
    dp_->add(std::move(py));
}

bool GraphVector::trivial() const {
    for (int i = 0; i < dp_->count(); ++i) {
        if (static_cast<double const*>(dp_->p(i)) != &zero) {
            return false;
        }
    }
    return true;
}

void GraphVector::request(Requisition& req) const {
    y_->erase();
    for (int i = 0; i < dp_->count(); ++i) {
        y_->add(*dp_->p(i));
    }
    GPolyLine::request(req);
}

// LineExtension
LineExtension::LineExtension(GPolyLine* gp) {
    gp_ = gp;  // don't ref since this is referenced by the polyline
    start_ = previous_ = -1;
}
LineExtension::~LineExtension() {}

void LineExtension::begin() {
    previous_ = yd()->count() - 1;
    start_ = yd()->count() - 1;
    yd()->running_start();
}
void LineExtension::extend() {
    previous_ = start_;
    start_ = yd()->count() - 1;
    yd()->running_start();
}


void LineExtension::request(Requisition& req) const {
    Coord x, span;
    Coord x1, x2;
    const float eps = 1e-4;
    x1 = xd()->running_min();
    x2 = xd()->running_max();
    span = (x2 - x1);
    x = (x1);
    x = (span > 0) ? x / span : 0;
    Requirement rx(span, 0, 0, -x);
    x1 = yd()->running_min();
    x2 = yd()->running_max();
    span = (x2 - x1) / 2;
    x = (x1);
    x = (span > 0) ? x / span : 0;
    Requirement ry(span, 0, 0, -x);
    req.require_x(rx);
    req.require_y(ry);
}

void LineExtension::allocate(Canvas* c, const Allocation& a, Extension& e) {
    e.set(c, a);
    //	MyMath::extend(e, gp_->brush()->width()/2 + 1);
}

void LineExtension::draw(Canvas* c, const Allocation& a) const {
#if 0
	if (previous_ >= 0) {
		gp_->draw_specific(c, a, previous_, xd()->count());
	}else
#endif
    if (start_ >= 0) {
        gp_->draw_specific(c, a, start_, xd()->count());
    }
}

void LineExtension::damage(Graph* g) {
    g->damage(xd()->running_min(), yd()->running_min(), xd()->running_max(), yd()->running_max());
}

void Graph::change_prop() {
    picker()->bind_select((OcHandler*) NULL);
    picker()->set_scene_tool(CHANGECOLOR);
    ColorBrushWidget::start(this);
    if (Oc::helpmode()) {
        help();
    }
}

#endif /* HAVE_IV */
