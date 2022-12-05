#include <../../nrnconf.h>
#if HAVE_IV  // to end of file

#include <ivstream.h>
#include <cstdio>
#include <cassert>
#include <cmath>

#include <InterViews/event.h>
#include <InterViews/hit.h>
#include <InterViews/canvas.h>
#include <InterViews/printer.h>
#include <InterViews/session.h>

#include <InterViews/monoglyph.h>
#include <InterViews/tformsetter.h>
#include <InterViews/layout.h>
#include <InterViews/style.h>
#include <IV-look/kit.h>
#include <InterViews/background.h>

#include "mymath.h"
#include "apwindow.h"
#include "hocdec.h"
#include "ocglyph.h"
#include "scenevie.h"
#include "scenepic.h"
#include "rubband.h"
#include "idraw.h"

// XYView
/*static*/ class XYView_helper: public MonoGlyph {
  public:
    XYView_helper(Scene*, XYView*);
    virtual ~XYView_helper();
    virtual void request(Requisition&) const;
    virtual void allocate(Canvas*, const Allocation&, Extension&);
    virtual void draw(Canvas*, const Allocation&) const;
    virtual void print(Printer*, const Allocation&) const;
    virtual void pick(Canvas*, const Allocation&, int depth, Hit&);

  public:
    Transformer t_;

  private:
    XYView* v_;
    friend XYView* XYView::current_pick_view();
    friend XYView* XYView::current_draw_view();
    friend void XYView::current_pick_view(XYView*);
    static XYView* current_pick_view_;
    static XYView* current_draw_view_;
};

void XYView::current_pick_view(XYView* v) {
    XYView_helper::current_pick_view_ = v;
}

void print_t(const char* s, const Transformer& t) {
    float a00, a01, a10, a11, a20, a21;
    t.matrix(a00, a01, a10, a11, a20, a21);
    printf("%s transform %g %g %g %g %g %g\n", s, a00, a01, a10, a11, a20, a21);
}
XYView_helper::XYView_helper(Scene* s, XYView* v)
    : MonoGlyph(s) {
    v_ = v;
}
void XYView_helper::request(Requisition& req) const {
    Requirement rx(v_->width(), 0, 0, -v_->left() / v_->width());
    Requirement ry(v_->height(), 0, 0, -v_->bottom() / v_->height());
    req.require_x(rx);
    req.require_y(ry);
}

void XYView_helper::allocate(Canvas* c, const Allocation& a, Extension& ext) {
    t_ = c->transformer();
    // print_t("XYView_helper::allocate", t_);
    body()->allocate(c, a, ext);
}

void XYView_helper::draw(Canvas* c, const Allocation& a) const {
    current_draw_view_ = v_;
    ((XYView_helper*) this)->t_ = c->transformer();
    // print_t("XYView_helper::draw", c->transformer());
    v_->set_damage_area(c);
#if 0
	IfIdraw(pict(t_));
#else
    if (OcIdraw::idraw_stream) {
        Transformer tr(t_);
        tr.translate(3 * 72, 4 * 72);
        OcIdraw::pict(tr);
    }
#endif
    c->push_clipping();
    c->clip_rect(v_->left(), v_->bottom(), v_->right(), v_->top());
    body()->draw(c, a);
    c->pop_clipping();
    IfIdraw(end());
}

void XYView_helper::print(Printer* c, const Allocation&) const {
    current_draw_view_ = v_;
    c->push_clipping();
    c->clip_rect(v_->left(), v_->bottom(), v_->right(), v_->top());

    char buf[100];
    float x, b;
    v_->s2o().matrix(x, b, b, b, b, b);
    Sprintf(buf, "\n%g setlinewidth", x);
    c->comment(buf);

    // when printfile started printing at the level of the xyview
    // the allocation was incorrect and was used by the background
    // that was ok when the background was white...
    // set the allocation the same as the clipping
    Allocation a1;
    Allotment ax(v_->left(), v_->width(), 0);
    Allotment ay(v_->bottom(), v_->height(), 0);
    a1.allot_x(ax);
    a1.allot_y(ay);

    body()->print(c, a1);
    c->pop_clipping();
}

void XYView_helper::pick(Canvas* c, const Allocation& a, int depth, Hit& h) {
    if (MyMath::inside(h.left(), h.bottom(), v_->left(), v_->bottom(), v_->right(), v_->top())) {
        if (h.event()->grabber()) {  // fixes a bug but I dont know why
#if 1
            // The above fix broke the handling of keystrokes for crosshairs and Rotate3D
            // It was needed so that buttons would appear normal when moving quickly from
            // a button through a box to a scene. Now we put in the right handler in
            // case event was a keystroke.
            if (h.event()->type() == Event::key) {
                h.target(depth, this, 0, h.event()->grabber());
            }
#endif
            return;
        }
        current_pick_view_ = v_;
        MonoGlyph::pick(c, a, depth, h);
        if (h.event()->type() == Event::down) {
#if 0
printf("XYView_helper hit (%g, %g)  event (%g, %g)\n", h.left(), h.bottom(),
h.event()->pointer_x(), h.event()->pointer_y());
printf(" allocation lb=(%g, %g), rt=(%g,%g)\n", a.left(), a.bottom(), a.right(), a.top());
#endif
        }
    }
}

static Coord pick_epsilon;
static void set_pick_epsilon() {
    pick_epsilon = 2;
}


XYView::XYView(Scene* s, Coord xsize, Coord ysize)
    : TransformSetter(new XYView_helper(s, this)) {
    init(s->x1(), s->y1(), s->x2() - s->x1(), s->y2() - s->y1(), s, xsize, ysize);
}

XYView::XYView(Coord x1, Coord y1, Coord xs, Coord ys, Scene* s, Coord xsize, Coord ysize)
    : TransformSetter(new XYView_helper(s, this)) {
    init(x1, y1, xs, ys, s, xsize, ysize);
}

void XYView::init(Coord x1, Coord y1, Coord xs, Coord ys, Scene* s, Coord xsize, Coord ysize) {
    set_pick_epsilon();
    xsize_orig_ = xsize;
    ysize_orig_ = ysize;
    csize(0., xsize, 0., ysize);
    origin(x1, y1);
    x_span(xs);
    y_span(ys);
    canvas(NULL);
    parent_ = NULL;  // not reffed
    append_view(s);
#if 0
	if (view_margin_ == fil) {
		Style* style = Session::instance()->style();
		if (!style->find_attribute("view_margin", view_margin_)) {
			view_margin_ = 0;
		}
		view_margin_ *= 72;
	}
#endif
}

Coord XYView::view_margin_ = fil;

XYView_helper::~XYView_helper() {
    if (v_ == current_pick_view_) {
        current_pick_view_ = NULL;
    }
    if (v_ == current_draw_view_) {
        current_draw_view_ = NULL;
    }
}

XYView::~XYView() {
    //	printf("~XYView\n");
    scene()->remove_view(this);
}

// should only be accessed by a method that traces its call from the pick
XYView* XYView::current_pick_view() {
    //	printf("current pick view %p\n", XYView_helper::current_pick_view_);
    return XYView_helper::current_pick_view_;
}

XYView* XYView_helper::current_pick_view_;

// should only be accessed by a method that traces its call from the draw
// or print
XYView* XYView::current_draw_view() {
    //	printf("current draw view %p\n", XYView_helper::current_draw_view_);
    return XYView_helper::current_draw_view_;
}

XYView* XYView_helper::current_draw_view_;

void XYView::append_view(Scene* s) {
    s->append_view(this);
}

void XYView::canvas(Canvas* c) {
    canvas_ = c;
}

void XYView::stroke(Canvas* c, const Color* color, const Brush* brush) {
    if (scene()->drawing_fixed_item()) {
        c->stroke(color, brush);
    } else {
        c->push_transform();
        c->transform(s2o());
        c->stroke(color, brush);
        c->pop_transform();
    }
}

Canvas* XYView::canvas() {
    return canvas_;
}

void XYView::undraw() {
    canvas_ = NULL;
    TransformSetter::undraw();
}

void XYView::damage(Glyph* g, const Allocation& a, bool fixed, bool vf) {
    if (canvas_) {
        Extension e;
        canvas_->push_transform();
        canvas_->transformer(((XYView_helper*) body())->t_);
        if (fixed) {
            Coord x, y;
            canvas_->transform(s2o());
            if (vf) {
                view_ratio(a.x(), a.y(), x, y);
            } else {
                s2o().inverse_transform(a.x(), a.y(), x, y);
            }
            Allocation a_fix = a;
            a_fix.x_allotment().origin(x);
            a_fix.y_allotment().origin(y);
            g->allocate(canvas_, a_fix, e);
        } else {
            g->allocate(canvas_, a, e);
        }
        // printf("damage extension %g %g %g %g\n", e.left(), e.bottom(), e.right(), e.top());
        // print_t("XYView::damage", canvas_->transformer());
        canvas_->pop_transform();
        canvas_->damage(e);
    }
}

void XYView::damage_all() {
    if (canvas_) {
        canvas_->damage(xc0_, yc0_, xc0_ + xsize_, yc0_ + ysize_);
    }
}

void XYView::damage(Coord x1, Coord y1, Coord x2, Coord y2) {
    if (canvas_) {
        Transformer& t = ((XYView_helper*) body())->t_;
        Coord tx1, ty1, tx2, ty2;
        t.transform(x1, y1, tx1, ty1);
        t.transform(x2, y2, tx2, ty2);
        const float off = canvas_->to_coord(1);
        tx1 = std::max(tx1 - off, Coord(0));
        ty1 = std::max(ty1 - off, Coord(0));
        tx2 = std::min(tx2 + off, canvas_->width());
        ty2 = std::min(ty2 + off, canvas_->height());
        canvas_->damage(tx1, ty1, tx2, ty2);
    }
}

void XYView::set_damage_area(Canvas* c) {
    Extension e;
    c->restrict_damage(0., 0., c->width(), c->height());
    c->damage_area(e);
    const float off = c->to_coord(1);
    c->transformer().inverse_transform(e.left() - off, e.bottom() - off, xd1_, yd1_);
    c->transformer().inverse_transform(e.right() + off, e.top() + off, xd2_, yd2_);
}

void XYView::damage_area(Coord& x1, Coord& y1, Coord& x2, Coord& y2) const {
    x1 = xd1_;
    y1 = yd1_;
    x2 = xd2_;
    y2 = yd2_;
}

void XYView::request(Requisition& req) const {
    TransformSetter::request(req);
    Requirement rx(xsize_orig_);
    Requirement ry(ysize_orig_);
    req.require_x(rx);
    req.require_y(ry);
}

void XYView::allocate(Canvas* c, const Allocation& a, Extension& ext) {
#ifdef MINGW
    if (a.y_allotment().span() <= 0. || a.x_allotment().span() <= 0.) {
        // a bug in mswindows iconify
        return;
    }
#endif
    if (canvas_ == NULL) {
        canvas_ = c;
    }
    c->push_transform();
    TransformSetter::allocate(c, a, ext);
    c->pop_transform();
}

void XYView::pick(Canvas* c, const Allocation& a, int depth, Hit& h) {
    canvas_ = c;
    c->push_transform();
    if (h.event()->type() == Event::down) {
#if 0
printf("XYView hit (%g, %g)  event (%g, %g)\n", h.left(), h.bottom(),
h.event()->pointer_x(), h.event()->pointer_y());
#endif
    }
    TransformSetter::pick(c, a, depth, h);
    c->pop_transform();
}

Scene* XYView::scene() const {
    return (Scene*) (((XYView_helper*) body())->body());
}

Coord XYView::left() const {
    return x1_;
}
Coord XYView::right() const {
    return x1_ + x_span_;
}
Coord XYView::bottom() const {
    return y1_;
}
Coord XYView::top() const {
    return y1_ + y_span_;
}
Coord XYView::width() const {
    return x_span_;
}
Coord XYView::height() const {
    return y_span_;
}

void XYView::view_ratio(float xrat, float yrat, Coord& x, Coord& y) const {
    x = xrat * xsize_ + xc0_;
    y = yrat * ysize_ + yc0_;
}

void XYView::ratio_view(Coord x, Coord y, float& xrat, float& yrat) const {
    xrat = (x - xc0_) / xsize_;
    yrat = (y - yc0_) / ysize_;
}

void XYView::size(Coord x1, Coord y1, Coord x2, Coord y2) {
    x1_ = std::min(x1, x2);
    y1_ = std::min(y1, y2);
    x_span_ = std::abs(x2 - x1);
    y_span_ = std::abs(y2 - y1);
    notify();
}

void XYView::origin(Coord x1, Coord y1) {
    x1_ = x1;
    y1_ = y1;
    notify();
}

void XYView::csize(Coord x0, Coord x, Coord y0, Coord y) const {
    XYView* v = (XYView*) this;
    v->xsize_ = x;
    v->ysize_ = y;
    v->xc0_ = x0;
    v->yc0_ = y0;
}

void XYView::box_size(Coord x1, Coord y1, Coord x2, Coord y2) {
    size(x1, y1, x2, y2);
}

void XYView::x_span(Coord x) {
    x_span_ = (x > 0) ? x : 1.;
    notify();
}
void XYView::y_span(Coord x) {
    y_span_ = (x > 0) ? x : 1.;
    notify();
}


void XYView::zout(Coord& x1, Coord& y1, Coord& x2, Coord& y2) const {
    Coord dx, dy;
    x1 = left();
    x2 = right();
    y1 = bottom();
    y2 = top();
    dx = .1 * (x2 - x1);
    dy = .1 * (y2 - y1);
    x1 -= dx;
    x2 += dx;
    y1 -= dy;
    y2 += dy;
}
void XYView::zin(Coord& x1, Coord& y1, Coord& x2, Coord& y2) const {
    Coord dx, dy;
    x1 = left();
    x2 = right();
    y1 = bottom();
    y2 = top();
    dx = .1 / 1.2 * (x2 - x1);
    dy = .1 / 1.2 * (y2 - y1);
    x1 += dx;
    x2 -= dx;
    y1 += dy;
    y2 -= dy;
}

void XYView::save(std::ostream& o) {
    PrintableWindow* w;
    if (!canvas_) {
        if (!parent() || !parent()->has_window()) {
            return;
        }
        w = parent()->window();
    } else {
        w = (PrintableWindow*) canvas()->window();
    }
    char buf[256];
    Coord x1, y1, x2, y2;
    zin(x1, y1, x2, y2);
    Sprintf(buf,
            "{save_window_.view(%g, %g, %g, %g, %g, %g, %g, %g)}",
            x1,
            y1,
            x2 - x1,
            y2 - y1,
            w->save_left(),
            w->save_bottom(),
            xsize_,
            ysize_);
    o << buf << std::endl;
}

void XYView::scene2view(const Allocation& a) const {
    float m00 = width() / a.x_allotment().span();
    float m11 = height() / a.y_allotment().span();

    // takes a canvas transformation from scene to parent glyph coordinates
    // transforms vectors from original to xyview
    XYView* xyv = (XYView*) this;
    xyv->scene2viewparent_ =
        Transformer(m00, 0, 0, m11, left() - a.left() * m00, bottom() - a.bottom() * m11);
    // print_t("scene2view", scene2viewparent_);
}

void XYView::transform(Transformer& t, const Allocation& a, const Allocation& n) const {
#if 0
	Allotment ax, ay;
	if (view_margin()) {
		const Allotment& alx = a.x_allotment();
		ax.span(alx.span() - 2*view_margin());
		ax.origin(alx.begin() + view_margin());
		ax.alignment(0);
		const Allotment& aly = a.y_allotment();
		ay.span(aly.span() - 2*view_margin());
		ay.origin(aly.begin() + view_margin());
		ay.alignment(0);
	}else{
		ax = a.x_allotment();
		ay = a.y_allotment();
	}
	Allocation al;
	al.allot_x(ax);
	al.allot_y(ay);
	scene2view(al);
#else
    scene2view(a);
    const Allotment& ax = a.x_allotment();
    const Allotment& ay = a.y_allotment();
#endif
    const Allotment& nx = n.x_allotment();
    const Allotment& ny = n.y_allotment();
    XYView* v = (XYView*) this;
    csize(ax.begin(), ax.span(), ay.begin(), ay.span());
    float sx = xsize_ / width();
    float sy = ysize_ / height();
    XYView* xv = (XYView*) this;
    xv->x_pick_epsilon_ = pick_epsilon / sx;
    xv->y_pick_epsilon_ = pick_epsilon / sy;
    t.translate(-left(), -bottom());
    t.scale(sx, sy);
    t.translate(ax.begin(), ay.begin());
#if 0
printf("XYView::transform ax origin=%g span=%g alignment=%g begin=%g\n",
ax.origin(), ax.span(), ax.alignment(), ax.begin());
printf("XYView::transform ay origin=%g span=%g alignment=%g begin=%g %g\n",
ay.origin(), ay.span(), ay.alignment(), ay.begin(), ay.end());
printf("XYView::transform natx origin=%g span=%g alignment=%g begin=%g\n",
nx.origin(), nx.span(), nx.alignment(), nx.begin());
printf("XYView::transform naty origin=%g span=%g alignment=%g begin=%g %g\n",
ny.origin(), ny.span(), ny.alignment(), ny.begin(), ny.end());
#endif
}

// View
View::View(Scene* s)
    : XYView(s, s->x2() - s->x1(), s->y2() - s->y1()) {
    x_span_ = XYView::width();
    y_span_ = XYView::height();
}
View::View(Coord x, Coord y, Coord span, Scene* s, Coord xsize, Coord ysize)
    : XYView(x - span / 2., y - (ysize / xsize) * span / 2., span, span, s, xsize, ysize) {
    x_span_ = XYView::width();
    y_span_ = XYView::height();
}
View::View(Coord x1, Coord y1, Coord xs, Coord ys, Scene* s, Coord xsize, Coord ysize)
    : XYView(x1, y1, xs, ys, s, xsize, ysize) {
    x_span_ = XYView::width();
    y_span_ = XYView::height();
}
View::~View() {}

void View::origin(Coord x, Coord y) {
    XYView::origin(x - XYView::width() / 2., y - XYView::height() / 2.);
}

void View::box_size(Coord x1, Coord y1, Coord x2, Coord y2) {
    Coord w = x2 - x1;
    Coord h = y2 - y1;
    Coord magx = w / x_span_;
    Coord magy = h / y_span_;
    if (magx > magy) {
        x_span_ *= magx;
        y_span_ *= magx;
    } else {
        x_span_ *= magy;
        y_span_ *= magy;
    }
    x_span(x_span_);
    y_span(y_span_);
    origin((x1 + x2) / 2, (y1 + y2) / 2);
}

Coord View::x() const {
    return left() + XYView::width() / 2.;
}
Coord View::y() const {
    return bottom() + XYView::height() / 2.;
}
Coord View::view_width() const {
    return x_span_;
}
Coord View::view_height() const {
    return y_span_;
}

void View::transform(Transformer& t, const Allocation& a, const Allocation&) const {
    scene2view(a);
    const Allotment& ax = a.x_allotment();
    const Allotment& ay = a.y_allotment();
    csize(ax.begin(), ax.span(), ay.begin(), ay.span());
    float sx = ax.span() / XYView::width();
    float sy = ay.span() / XYView::height();
    //	if (sx > sy) sx = sy;
    t.translate(-x(), -y());
    t.scale(sx, sx);
    View* v = (View*) this;
    v->x_pick_epsilon_ = pick_epsilon / sx;
    v->y_pick_epsilon_ = pick_epsilon / sx;
    t.translate((ax.begin() + ax.end()) / 2, (ay.begin() + ay.end()) / 2);
    // printf("\nx origin=%g span=%g alignment=%g begin=%g end=%g\n", ax.origin(), ax.span(),
    // ax.alignment(), ax.begin(), ax.end()); printf("\ny origin=%g span=%g alignment=%g begin=%g
    // end=%g\n", ay.origin(), ay.span(), ay.alignment(), ay.begin(), ay.end());
    Coord x1, y1;
    t.transform(x() - x_span_ / 2, y() - y_span_ / 2, x1, y1);
    if (!MyMath::eq(ax.begin(), x1, 1.f) || !MyMath::eq(ay.begin(), y1, 1.f)) {
        t.inverse_transform(ax.begin(), ay.begin(), x1, y1);
        v->x_span_ = 2 * (x() - x1);
        v->y_span_ = 2 * (y() - y1);
        v->size(x1, y1, x1 + v->x_span_, y1 + v->y_span_);
    }
}

void XYView::move_view(Coord dx1, Coord dy1) {
    //	printf("move by %g %g \n", dx1, dy1);
    Coord x0, x1, y0, y1;
    Coord dx = std::abs(dx1);
    Coord dy = std::abs(dy1);
    if (dx < .9 * dy) {
        dx = 0.;
        dy = dy1;
    } else if (dy < .9 * dx) {
        dx = dx1;
        dy = 0.;
    } else {
        dx = dx1;
        dy = dy1;
    }
    s2o().transform(0, 0, x0, y0);
    s2o().transform(dx, dy, x1, y1);
    x0 = x0 - x1 + left();
    y0 = y0 - y1 + bottom();
    x1 = x0 + width();
    y1 = y0 + height();

#if 1
    if (dx > 0) {
        MyMath::round(x0, x1, MyMath::Higher, 4);
    } else {
        MyMath::round(x0, x1, MyMath::Lower, 4);
    }
    if (dy > 0) {
        MyMath::round(y0, y1, MyMath::Higher, 4);
    } else {
        MyMath::round(y0, y1, MyMath::Lower, 4);
    }
#endif

    XYView::origin(x0, y0);
    damage_all();
}

void View::move_view(Coord dx, Coord dy) {
    XYView::move_view(dx, dy);
}


void XYView::scale_view(Coord xorg, Coord yorg, float dxscl, float dyscl) {
    Coord x0, y0, l, b, r, t;
    Coord dx = std::abs(dxscl);
    Coord dy = std::abs(dyscl);
    if (dx < .9 * dy) {
        dx = 0.;
        dy = dyscl;
    } else if (dy < .9 * dx) {
        dx = dxscl;
        dy = 0.;
    } else {
        dx = dxscl;
        dy = dyscl;
    }
    s2o().transform(xorg, yorg, x0, y0);
    // printf("org %g %g %g %g\n", xorg, yorg, x0, y0);
    l = -(left() - x0) * dx + left();
    b = -(bottom() - y0) * dy + bottom();
    r = -(right() - x0) * dx + right();
    t = -(top() - y0) * dy + top();
#if 1
    if (dxscl > 1) {
        MyMath::round(l, r, MyMath::Expand, 4);
    } else {
        MyMath::round(l, r, MyMath::Contract, 4);
    }
    if (dyscl > 1) {
        MyMath::round(b, t, MyMath::Expand, 4);
    } else {
        MyMath::round(b, t, MyMath::Contract, 4);
    }
#endif
    size(l, b, r, t);
    damage_all();
}

void View::scale_view(Coord xorg, Coord yorg, float dxscl, float) {
    XYView::scale_view(xorg, yorg, dxscl, dxscl);
}

XYView* XYView::new_view(Coord x1, Coord y1, Coord x2, Coord y2) {
    Coord l, b, r, t;
    s2o().inverse_transform(x1, y1, l, b);
    s2o().inverse_transform(x2, y2, r, t);
    return new XYView(x1, y1, x2 - x1, y2 - y1, scene(), r - l, t - b);
}

XYView* View::new_view(Coord x1, Coord y1, Coord x2, Coord y2) {
    Coord l, b, r, t;
    s2o().inverse_transform(x1, y1, l, b);
    s2o().inverse_transform(x2, y2, r, t);
    return new View((x1 + x2) / 2, (y1 + y2) / 2, x2 - x1, scene(), r - l, t - b);
}

/*static*/ class NPInsetFrame: public MonoGlyph {
  public:
    NPInsetFrame(Glyph*);
    virtual ~NPInsetFrame();
    virtual void print(Printer*, const Allocation&) const;
};

NPInsetFrame::NPInsetFrame(Glyph* g)
    : MonoGlyph(WidgetKit::instance()->inset_frame(g)) {}
NPInsetFrame::~NPInsetFrame() {}
void NPInsetFrame::print(Printer* p, const Allocation& a) const {
    Style* s = WidgetKit::instance()->style();
    long i = 1;
    s->find_attribute("scene_print_border", i);
    // printf("NPInsetFrame %ld\n", i);
    if (i) {
        body()->print(p, a);
    } else {
        ((MonoGlyph*) body())->body()->print(p, a);
    }
}

OcViewGlyph::OcViewGlyph(XYView* v)
    : OcGlyph(new Background(
          //   WidgetKit::instance()->inset_frame(
          new NPInsetFrame(LayoutKit::instance()->variable_span(v)),
          WidgetKit::instance()->background())) {
    v_ = v;
    g_ = NULL;
    v_->ref();
    assert(v_->parent() == NULL);
    v_->parent_ = this;
};

OcViewGlyph::~OcViewGlyph() {
    v_->parent_ = NULL;
    v_->unref();
    Resource::unref(g_);
}

void OcViewGlyph::save(std::ostream& o) {
    Scene* s = v_->scene();
    char buf[256];
    long i = Scene::scene_list_index(s);
    if (!s->mark()) {
        s->save_phase1(o);
        Sprintf(buf, "scene_vector_[%ld] = save_window_", i);
    } else {
        Sprintf(buf, "save_window_ = scene_vector_[%ld]", i);
    }
    o << buf << std::endl;
    v_->save(o);
    if (!s->mark()) {
        s->save_phase2(o);
        s->mark(true);
    }
}

ViewWindow::ViewWindow(XYView* v, const char* name)
    : PrintableWindow(new OcViewGlyph(v)) {
    if (name) {
        type(name);
    }
    v->attach(this);
    update(v);
}

ViewWindow::~ViewWindow() {
    OcViewGlyph* g = (OcViewGlyph*) glyph();
    g->view()->detach(this);
}

void ViewWindow::update(Observable* o) {
    char s[200];
    XYView* v = (XYView*) o;
    Sprintf(s,
            "%s %s x %g : %g  y %g : %g",
            type(),
            v->scene()->picker()->select_name(),
            v->left(),
            v->right(),
            v->bottom(),
            v->top());
    name(s);
}
#endif
