#include <../../nrnconf.h>
// hoc level Glyph implementation for graphing
#include <stdio.h>
#include "classreg.h"
#include "oc2iv.h"
#if HAVE_IV
#include "ivoc.h"
#include <InterViews/printer.h>
#include <InterViews/image.h>
#include "grglyph.h"
#include "idraw.h"

extern Image* gif_image(const char*);
#else
#include <InterViews/resource.h>
class GrGlyph: public Resource {
  public:
    GrGlyph(Object*);
    virtual ~GrGlyph();
    Object** temp_objvar();

  private:
    Object* obj_;
};
#endif  // HAVE_IV

#include "gui-redirect.h"

double gr_addglyph(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Graph.addglyph", v);
#if HAVE_IV
    IFGUI
    Graph* g = (Graph*) v;
    Object* obj = *hoc_objgetarg(1);
    check_obj_type(obj, "Glyph");
    GrGlyph* gl = (GrGlyph*) (obj->u.this_pointer);
    Coord x = *getarg(2);
    Coord y = *getarg(3);
    Coord sx = ifarg(4) ? *getarg(4) : 1.;
    Coord sy = ifarg(5) ? *getarg(5) : 1.;
    Coord rot = ifarg(6) ? *getarg(6) : 0.;
    int fix = ifarg(7) ? int(chkarg(7, 0, 2)) : 0;
    GrGlyphItem* ggi = new GrGlyphItem(gl, sx, sy, rot);
    switch (fix) {
    case 0:
        g->append(ggi);
        break;
    case 1:
        g->append_fixed(ggi);
        break;
    case 2:
        g->append_viewfixed(ggi);
        break;
    }
    g->move(g->count() - 1, x, y);
    ENDGUI
#endif
    return 0.;
}

static Object** g_new_path(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_OBJ("Glyph.path", v);
    GrGlyph* g = (GrGlyph*) v;
#if HAVE_IV
    IFGUI
    g->new_path();
    ENDGUI
#endif
    return g->temp_objvar();
}

static Object** g_move_to(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_OBJ("Glyph.m", v);
    GrGlyph* g = (GrGlyph*) v;
#if HAVE_IV
    IFGUI
    g->move_to(*getarg(1), *getarg(2));
    ENDGUI
#endif
    return g->temp_objvar();
}

static Object** g_line_to(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_OBJ("Glyph.l", v);
    GrGlyph* g = (GrGlyph*) v;
#if HAVE_IV
    IFGUI
    g->line_to(*getarg(1), *getarg(2));
    ENDGUI
#endif
    return g->temp_objvar();
}

static Object** g_control_point(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_OBJ("Glyph.cpt", v);
    GrGlyph* g = (GrGlyph*) v;
#if HAVE_IV
    IFGUI
    g->control_point(*getarg(1), *getarg(2));
    ENDGUI
#endif
    return g->temp_objvar();
}

static Object** g_curve_to(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_OBJ("Glyph.curve", v);
    GrGlyph* g = (GrGlyph*) v;
#if HAVE_IV
    IFGUI
    g->curve_to(*getarg(1), *getarg(2), *getarg(3), *getarg(4), *getarg(5), *getarg(6));
    ENDGUI
#endif
    return g->temp_objvar();
}

static Object** g_stroke(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_OBJ("Glyph.s", v);
    GrGlyph* g = (GrGlyph*) v;
#if HAVE_IV
    IFGUI
    int ci = ifarg(1) ? int(chkarg(1, 0, 10000)) : 1;
    int bi = ifarg(2) ? int(chkarg(2, 0, 10000)) : 0;
    g->stroke(ci, bi);
    ENDGUI
#endif
    return g->temp_objvar();
}

static Object** g_close_path(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_OBJ("Glyph.close", v);
    GrGlyph* g = (GrGlyph*) v;
#if HAVE_IV
    IFGUI
    g->close_path();
    ENDGUI
#endif
    return g->temp_objvar();
}

static Object** g_fill(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_OBJ("Glyph.fill", v);
    GrGlyph* g = (GrGlyph*) v;
#if HAVE_IV
    IFGUI
    int ci = ifarg(1) ? int(chkarg(1, 0, 10000)) : 1;
    g->fill(ci);
    ENDGUI
#endif
    return g->temp_objvar();
}

static Object** g_erase(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_OBJ("Glyph.erase", v);
    GrGlyph* g = (GrGlyph*) v;
#if HAVE_IV
    IFGUI
    g->erase();
    ENDGUI
#endif
    return g->temp_objvar();
}

static Object** g_circle(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_OBJ("Glyph.circle", v);
    GrGlyph* g = (GrGlyph*) v;
#if HAVE_IV
    IFGUI
    g->circle(*getarg(1), *getarg(2), *getarg(3));
    ENDGUI
#endif
    return g->temp_objvar();
}

static Object** g_gif(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_OBJ("Glyph.gif", v);
    GrGlyph* g = (GrGlyph*) v;
#if HAVE_IV
    IFGUI
    g->gif(gargstr(1));
    ENDGUI
#endif
    return g->temp_objvar();
}

static Symbol* sggl_;

Member_func members[] = {{0, 0}};

Member_ret_obj_func objmembers[] = {{"path", g_new_path},
                                    {"m", g_move_to},
                                    {"l", g_line_to},
                                    {"s", g_stroke},
                                    {"close", g_close_path},
                                    {"fill", g_fill},
                                    {"curve", g_curve_to},
                                    {"cpt", g_control_point},
                                    {"erase", g_erase},
                                    {"gif", g_gif},
                                    {"circle", g_circle},
                                    {0, 0}};

static void* cons(Object* o) {
    TRY_GUI_REDIRECT_OBJ("Glyph", NULL);
    GrGlyph* g = new GrGlyph(o);
    g->ref();
    return g;
}

static void destruct(void* v) {
    TRY_GUI_REDIRECT_NO_RETURN("~Glyph", v);
    GrGlyph* g = (GrGlyph*) v;
    g->unref();
}

void GrGlyph_reg() {
    class2oc("Glyph", cons, destruct, members, NULL, objmembers, NULL);
    sggl_ = hoc_lookup("Glyph");
}

GrGlyph::GrGlyph(Object* o) {
    obj_ = o;
#if HAVE_IV
    IFGUI
    type_ = new DataVec(10);
    x_ = new DataVec(10);
    y_ = new DataVec(10);
    type_->ref();
    x_->ref();
    y_->ref();
    gif_ = NULL;
    ENDGUI
#endif
}

GrGlyph::~GrGlyph() {
#if HAVE_IV
    IFGUI
    type_->unref();
    x_->unref();
    y_->unref();
    Resource::unref(gif_);
    ENDGUI
#endif
}

Object** GrGlyph::temp_objvar() {
    GrGlyph* gg = (GrGlyph*) this;
    Object** po;
    po = hoc_temp_objptr(gg->obj_);
    return po;
}

#if HAVE_IV

GrGlyphItem::GrGlyphItem(Glyph* g, float sx, float sy, float angle)
    : GraphItem(g) {
    t_.scale(sx, sy);
    t_.rotate(angle);
}
GrGlyphItem::~GrGlyphItem() {}

void GrGlyphItem::allocate(Canvas* c, const Allocation& a, Extension& e) {
    e.set(c, a);
}

void GrGlyphItem::draw(Canvas* c, const Allocation& a) const {
    c->push_transform();
    Transformer t = t_;
    t.translate(a.x(), a.y());
    c->transform(t);
    IfIdraw(pict(t));
    // float m1, m2, m3,m4,m5,m6;
    // t.matrix(m1,m2,m3,m4,m5,m6);
    // printf("transformer %g %g %g %g %g %g\n", m1,m2,m3,m4,m5,m6);
    body()->draw(c, a);
    c->pop_transform();
    IfIdraw(end());
}

void GrGlyphItem::print(Printer* c, const Allocation& a) const {
    draw(c, a);
}

void GrGlyph::gif(const char* file) {
    gif_ = gif_image(file);
}
void GrGlyph::new_path() {
    type_->add(1);
}
void GrGlyph::move_to(Coord x, Coord y) {
    type_->add(2);
    x_->add(x);
    y_->add(y);
}
void GrGlyph::line_to(Coord x, Coord y) {
    type_->add(3);
    x_->add(x);
    y_->add(y);
}
void GrGlyph::curve_to(Coord x, Coord y, Coord x1, Coord y1, Coord x2, Coord y2) {
    type_->add(4);
    x_->add(x);
    y_->add(y);
    x_->add(x1);
    y_->add(y1);
    x_->add(x2);
    y_->add(y2);
}
void GrGlyph::close_path() {
    type_->add(5);
}
void GrGlyph::circle(Coord x, Coord y, Coord r) {
    const Coord p0 = 1.00000000 * r;
    const Coord p1 = 0.89657547 * r;  // cos 30 * sqrt(1 + tan 15 * tan 15)
    const Coord p2 = 0.70710678 * r;  // cos 45
    const Coord p3 = 0.51763809 * r;  // cos 60 * sqrt(1 + tan 15 * tan 15)
    const Coord p4 = 0.26794919 * r;  // tan 15
    new_path();
    move_to(x + p0, y);
    curve_to(x + p2, y + p2, x + p0, y + p4, x + p1, y + p3);
    curve_to(x, y + p0, x + p3, y + p1, x + p4, y + p0);
    curve_to(x - p2, y + p2, x - p4, y + p0, x - p3, y + p1);
    curve_to(x - p0, y, x - p1, y + p3, x - p0, y + p4);
    curve_to(x - p2, y - p2, x - p0, y - p4, x - p1, y - p3);
    curve_to(x, y - p0, x - p3, y - p1, x - p4, y - p0);
    curve_to(x + p2, y - p2, x + p4, y - p0, x + p3, y - p1);
    curve_to(x + p0, y, x + p1, y - p3, x + p0, y - p4);
    close_path();
}

void GrGlyph::stroke(int ci, int bi) {
    type_->add(6);
    type_->add(ci);
    type_->add(bi);
}
void GrGlyph::fill(int ci) {
    type_->add(7);
    type_->add(ci);
}
void GrGlyph::control_point(Coord x, Coord y) {
    type_->add(8);
    x_->add(x);
    y_->add(y);
}
void GrGlyph::erase() {
    type_->erase();
    x_->erase();
    y_->erase();
    if (gif_) {
        gif_->unref();
        gif_ = NULL;
    }
}

void GrGlyph::draw(Canvas* c, const Allocation& a) const {
    int i, j;
    Coord x, y;
    if (gif_) {
        gif_->draw(c, a);
    }
    for (i = 0, j = 0; i < type_->count(); ++i) {
        switch (int(type_->get_val(i))) {
        case 1:
            c->new_path();
            IfIdraw(new_path());
            break;
        case 2:
            x = x_->get_val(j);
            y = y_->get_val(j);
            ++j;
            c->move_to(x, y);
            IfIdraw(move_to(x, y));
            break;
        case 3:
            x = x_->get_val(j);
            y = y_->get_val(j);
            ++j;
            c->line_to(x, y);
            IfIdraw(line_to(x, y));
            break;
        case 4:
            x = x_->get_val(j);
            y = y_->get_val(j);
            c->curve_to(x,
                        y,
                        x_->get_val(j + 1),
                        y_->get_val(j + 1),
                        x_->get_val(j + 2),
                        y_->get_val(j + 2));
            IfIdraw(curve_to(x,
                             y,
                             x_->get_val(j + 1),
                             y_->get_val(j + 1),
                             x_->get_val(j + 2),
                             y_->get_val(j + 2)));
            j += 3;
            break;
        case 5:
            c->close_path();
            IfIdraw(close_path());
            break;
        case 6:
            x = type_->get_val(++i);
            y = type_->get_val(++i);
            c->stroke(colors->color(int(x)), brushes->brush(int(y)));
            IfIdraw(stroke(c, colors->color(int(x)), brushes->brush(int(y))));
            break;
        case 7:
            x = type_->get_val(++i);
            c->fill(colors->color(int(x)));
            IfIdraw(fill(c, colors->color(int(x))));
            break;
        case 8:
            x = x_->get_val(j);
            y = y_->get_val(j);
            ++j;
            c->transformer().transform(x, y);
            // printf("x=%g y=%g\n", x, y);
            c->push_transform();
            Transformer t;
            c->transformer(t);
            c->rect(x - 2, y - 2, x + 2, y + 2, colors->color(1), brushes->brush(0));
            c->pop_transform();
            break;
        }
    }
}

void GrGlyph::request(Requisition& r) const {
    Coord min, max, natural;
    min = x_->min();
    max = x_->max();
    natural = max - min;
    r.x_requirement().natural(natural);
    if (natural > 0) {
        r.x_requirement().alignment(-min / natural);
    }

    min = y_->min();
    max = y_->max();
    natural = max - min;
    r.y_requirement().natural(natural);
    if (natural > 0) {
        r.y_requirement().alignment(-min / natural);
    }

    if (gif_) {
        gif_->request(r);
    }
}

#endif
