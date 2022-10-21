#include <../../nrnconf.h>
#if HAVE_IV  // to end of file

#include <cstdio>
#include <ivstream.h>
#include <InterViews/background.h>
#include <InterViews/canvas.h>
#include <InterViews/printer.h>
#include <InterViews/label.h>
#include <IV-look/kit.h>
#include <cmath>
#include "scenevie.h"
#include "mymath.h"
#include "axis.h"
#include "rect.h"
#include "graph.h"
#include "idraw.h"

/*static*/ class GAxisItem: public GraphItem {
  public:
    GAxisItem(Glyph* g)
        : GraphItem(g) {}
    virtual ~GAxisItem(){};
    virtual void save(std::ostream&, Coord, Coord) {}
    virtual void erase(Scene* s, GlyphIndex i, int type) {
        if (type & GraphItem::ERASE_AXIS) {
            s->remove(i);
        }
    }
};

Axis::Axis(Scene* s, DimensionName d) {
    s_ = s;
    d_ = d;
    set_range();
    location();
    init(min_, max_, pos_, ntic_);
}

Axis::Axis(Scene* s, DimensionName d, Coord x1, Coord x2) {
    s_ = s;
    d_ = d;
    min_ = x1;
    max_ = x2;
    location();
    MyMath::round_range_down(x1, x2, amin_, amax_, ntic_);
    init(float(amin_), float(amax_), pos_, ntic_);
}

Axis::Axis(Scene* s,
           DimensionName d,
           Coord x1,
           Coord x2,
           Coord pos,
           int ntic,
           int nminor,
           int invert,
           bool number) {
    s_ = s;
    d_ = d;
    init(x1, x2, pos, ntic, nminor, invert, number);
}

void Axis::init(Coord x1, Coord x2, Coord pos, int ntic, int nminor, int invert, bool number) {
    min_ = x1;
    max_ = x2;
    pos_ = pos;
    ntic_ = ntic;
    nminor_ = nminor;
    invert_ = invert;
    number_ = number;
    amin_ = x1;
    amax_ = x2;
    s_->attach(this);
    install();
}

Axis::~Axis() {
    s_->detach(this);
    //	printf("~Axis\n");
}

void Axis::size(float& min, float& max) {
    min = float(amin_);
    max = float(amax_);
}

void Axis::save(std::ostream& o) {
    char buf[256];
    int c;
    if (d_ == Dimension_X) {
        c = 'x';
    } else {
        c = 'y';
    }
    sprintf(buf,
            "save_window_.%caxis(%g,%g,%g,%d,%d,%d,%d)",
            c,
            amin_,
            amax_,
            pos_,
            ntic_,
            nminor_,
            invert_,
            number_);
    o << buf << std::endl;
}

void Axis::update(Observable*) {}

bool Axis::set_range() {
    Coord x1, x2;
    if (d_ == Dimension_X) {
        x1 = s_->x1();
        x2 = s_->x2();
    } else {
        x1 = s_->y1();
        x2 = s_->y2();
    }

    min_ = x1;
    max_ = x2;
    MyMath::round_range(x1, x2, amin_, amax_, ntic_);
    return true;
}

void Axis::location() {
    Coord x1, y1, x2, y2;
    XYView* v = XYView::current_pick_view();
    if (v && v->scene() == s_) {
        v->zin(x1, y1, x2, y2);
    } else {
        x1 = s_->x1();
        x2 = s_->x2();
        y1 = s_->y1();
        y2 = s_->y2();
    }
    if (d_ == Dimension_X) {
        if (y1 > 0) {
            pos_ = y1;
        } else if (y2 < 0) {
            pos_ = y2;
        } else {
            pos_ = 0;
        }
    } else {
        if (x1 > 0) {
            pos_ = x1;
        } else if (x2 < 0) {
            pos_ = x2;
        } else {
            pos_ = 0;
        }
    }
}
#if 1
// Zach Mainen improvements

void Axis::install() {
    int i, j;
    GlyphIndex gi;
    Line* tic = NULL;
    Coord x, y;
    char str[20];
    float tic_space;

    float y_align, x_align;
    Line* minor_tic = NULL;

    Coord l_minor = 5.;
    Coord length = 10.;

    if (invert_ == 1) {
        l_minor *= -1.;
        length *= -1.;
    }

    char pform[10];

    int addprec;
    double s = (amax_ - amin_) / float(ntic_);
    while (s < 1)
        s *= 10.;
    if (s == 1 || s == 2)
        addprec = 0;
    else
        addprec = 1;

    double logstep = -log10((amax_ - amin_) / float(ntic_));

    if (d_ == Dimension_X) {
        if (logstep >= 0 && logstep <= 5) {
            sprintf(pform, "%%0.%.0ff", logstep + addprec);
        } else {
            sprintf(pform, "%%g");
        }
        y = pos_;
        s_->append(new GAxisItem(new Line(amax_ - amin_, 0)));
        gi = s_->count() - 1;
        s_->move(gi, amin_, y);

        tic = new Line(0, length);
        tic->ref();
        minor_tic = new Line(0, l_minor);
        minor_tic->ref();
        tic_space = (amax_ - amin_) / float(ntic_);

        for (i = 0; i <= ntic_; ++i) {
            x = amin_ + i * tic_space;
            if (std::abs(x) < 1e-10) {
                x = 0.;
            }
            if (invert_ >= 0) {
                s_->append_fixed(new GAxisItem(tic));
                gi = s_->count() - 1;
                s_->move(gi, x, y);
            }

            if (number_) {
                /*  if (i==0) x_align = 0.;
                  else if (i==ntic_) x_align = .9;
                  else x_align = 0.5;
                */
                x_align = 0.5;
                if (invert_ == 1)
                    y_align = -0.3;
                else
                    y_align = 1.5;
                sprintf(str, pform, x);
                s_->append_fixed(new GAxisItem(
                    new GLabel(str, Appear::default_color(), true, 1, x_align, y_align)));
                gi = s_->count() - 1;
                s_->move(gi, x, y);
            }

            if (i < ntic_ && invert_ >= 0) {
                for (j = 0; j < nminor_; j++) {
                    x = amin_ + i * tic_space + j * tic_space / nminor_;
                    s_->append_fixed(new GAxisItem(minor_tic));
                    gi = s_->count() - 1;
                    s_->move(gi, x, y);
                }
            }
        }
    } else {
        if (logstep >= 0 && logstep <= 5) {
            sprintf(pform, " %%0.%.0ff ", logstep + 1);
        } else {
            sprintf(pform, " %%g ");
        }
        x = pos_;
        s_->append(new GAxisItem(new Line(0, amax_ - amin_)));
        gi = s_->count() - 1;
        s_->move(gi, x, amin_);

        tic = new Line(length, 0);
        tic->ref();
        minor_tic = new Line(l_minor, 0);
        minor_tic->ref();
        tic_space = (amax_ - amin_) / float(ntic_);

        for (i = 0; i <= ntic_; ++i) {
            y = amin_ + i * tic_space;
            if (invert_ >= 0) {
                s_->append_fixed(new GAxisItem(tic));
                gi = s_->count() - 1;
                s_->move(gi, x, y);
            }

            if (number_) {
                sprintf(str, pform, y);
                y_align = 0.5;
                // if (i==0) y_align = 0.;
                // else if (i==ntic_) y_align = .66;
                // else y_align = 0.33;
                if (invert_ == 1)
                    x_align = 0;
                else
                    x_align = 1.3;
                s_->append_fixed(new GAxisItem(
                    new GLabel(str, Appear::default_color(), true, 1, x_align, y_align)));
                gi = s_->count() - 1;
                s_->move(gi, x, y);
            }

            if (i < ntic_ && invert_ >= 0) {
                for (j = 0; j < nminor_; j++) {
                    y = amin_ + i * tic_space + j * tic_space / nminor_;
                    s_->append_fixed(new GAxisItem(minor_tic));
                    gi = s_->count() - 1;
                    s_->move(gi, x, y);
                }
            }
        }
    }
    Resource::unref(tic);
    Resource::unref(minor_tic);
}
#else
void Axis::install() {
    int i;
    GlyphIndex gi;
    Coord length = 10;
    Line* tic = NULL;
    Coord x, y;
    char str[20];
    if (d_ == Dimension_X) {
        y = pos_;
        s_->append(new GAxisItem(new Line(amax_ - amin_, 0)));
        gi = s_->count() - 1;
        s_->move(gi, amin_, y);

        tic = new Line(0, length);
        tic->ref();
        for (i = 0; i <= ntic_; ++i) {
            x = amin_ + i * (amax_ - amin_) / float(ntic_);
            if (std::abs(x) < 1e-10) {
                x = 0.;
            }
            s_->append_fixed(new GAxisItem(tic));
            gi = s_->count() - 1;
            s_->move(gi, x, y);
            sprintf(str, "%g", x);
            s_->append_fixed(
                new GAxisItem(new GLabel(str, Appear::default_color(), true, 1, .5, 1.1)));
            gi = s_->count() - 1;
            s_->move(gi, x, y);
        }
    } else {
        x = pos_;
        s_->append(new GAxisItem(new Line(0, amax_ - amin_)));
        gi = s_->count() - 1;
        s_->move(gi, x, amin_);

        tic = new Line(length, 0);
        tic->ref();
        for (i = 0; i <= ntic_; ++i) {
            y = amin_ + i * (amax_ - amin_) / float(ntic_);
            s_->append_fixed(new GAxisItem(tic));
            gi = s_->count() - 1;
            s_->move(gi, x, y);
            sprintf(str, "%g", y);
            s_->append_fixed(
                new GAxisItem(new GLabel(str, Appear::default_color(), true, 1, 1, .5)));
            gi = s_->count() - 1;
            s_->move(gi, x, y);
        }
    }
    Resource::unref(tic);
}
#endif

BoxBackground::BoxBackground()
    : Background(NULL, Scene::default_background()) {}

BoxBackground::~BoxBackground() {}

void BoxBackground::draw(Canvas* c, const Allocation& a) const {
    Background::draw(c, a);
    draw_help(c, a);
}
void BoxBackground::print(Printer* c, const Allocation& a) const {
    Background::print(c, a);
    draw_help(c, a);
}

#define IDLINE(x1, y1, x2, y2, color, br) \
    c->line(x1, y1, x2, y2, color, br);   \
    IfIdraw(line(c, x1, y1, x2, y2, color, br));

void BoxBackground::draw_help(Canvas* c, const Allocation&) const {
    // printf("BoxBackground::draw\n");
    const Color* color = Scene::default_foreground();
    Coord x1, y1, x2, y2;
    double d1, d2;
    int xtic, ytic;
    XYView& v = *XYView::current_draw_view();
    v.zin(x1, y1, x2, y2);
    MyMath::round_range_down(x1, x2, d1, d2, xtic);
    x1 = d1;
    x2 = d2;
    MyMath::round_range_down(y1, y2, d1, d2, ytic);
    y1 = d1;
    y2 = d2;
    const Transformer& tr = v.s2o();
    c->push_transform();
    c->transform(tr);
    IfIdraw(pict(tr));
    Coord l, r, b, t;
    tr.inverse_transform(x1, y1, l, b);
    tr.inverse_transform(x2, y2, r, t);
    const Brush* br = Appear::default_brush();
    c->rect(l, b, r, t, color, br);
    IfIdraw(rect(c, l, b, r, t, color, br, false));
    const Coord tic = 10;
    Coord x, y;

    Coord dtic = (r - l) / xtic;
    Coord dx = (x2 - x1) / xtic;
    int i;
    for (i = 0; i <= xtic; ++i) {
        x = l + i * dtic;
        if (i > 0 && i < xtic) {
            IDLINE(x, b, x, b + tic, color, br);
            IDLINE(x, t, x, t - tic, color, br);
        }
        tic_label(x, b - 5, x1 + i * dx, .5, 1, c);
    }
    dtic = (t - b) / ytic;
    dx = (y2 - y1) / ytic;
    for (i = 0; i <= ytic; ++i) {
        y = b + i * dtic;
        if (i > 0 && i < ytic) {
            IDLINE(l, y, l + tic, y, color, br);
            IDLINE(r, y, r - tic, y, color, br);
        }
        tic_label(l - 5, y, y1 + i * dx, 1, .5, c);
    }

    c->clip_rect(l, b, r, t);
    c->pop_transform();
    IfIdraw(end());
}

void BoxBackground::tic_label(Coord x1, Coord y1, Coord val, float xa, float ya, Canvas* c) const {
    char buf[20];
    sprintf(buf, "%g", val);
    Glyph* g = new Label(buf, WidgetKit::instance()->font(), Appear::default_color());
    g->ref();
    Requisition req;
    g->request(req);
    Allocation a;
    a.x_allotment().origin(x1 - xa * req.x_requirement().natural());
    a.y_allotment().origin(y1 - ya * req.y_requirement().natural());
    g->draw(c, a);
    g->unref();
    if (OcIdraw::idraw_stream) {
        Transformer t;
        t.translate(a.x(), a.y());
        OcIdraw::text(c, buf, t, NULL, Appear::default_color());
    }
}

AxisBackground::AxisBackground()
    : Background(NULL, Scene::default_background()) {}

AxisBackground::~AxisBackground() {}

void AxisBackground::draw(Canvas* c, const Allocation& a) const {
    Background::draw(c, a);
    draw_help(c, a);
}
void AxisBackground::print(Printer* c, const Allocation& a) const {
    Background::print(c, a);
    draw_help(c, a);
}

void AxisBackground::draw_help(Canvas* c, const Allocation&) const {
    // printf("AxisBackground::draw\n");
    const Color* color = Scene::default_foreground();
    Coord x1, y1, x2, y2;
    double d1, d2;
    int xtic, ytic;
    XYView& v = *XYView::current_draw_view();
    v.zin(x1, y1, x2, y2);
    MyMath::round_range_down(x1, x2, d1, d2, xtic);
    x1 = d1;
    x2 = d2;
    MyMath::round_range_down(y1, y2, d1, d2, ytic);
    y1 = d1;
    y2 = d2;
    const Transformer& tr = v.s2o();
    c->push_transform();
    c->transform(tr);
    IfIdraw(pict(tr));
    Coord l, r, b, t;
    tr.inverse_transform(x1, y1, l, b);
    tr.inverse_transform(x2, y2, r, t);
    Coord xorg, yorg, xo, yo;
    if (MyMath::inside(0, x1, x2)) {
        xorg = 0;
    } else {
        xorg = x1;
    }
    if (MyMath::inside(0, y1, y2)) {
        yorg = 0;
    } else {
        yorg = y1;
    }
    tr.inverse_transform(xorg, yorg, xo, yo);
    const Brush* br = Appear::default_brush();
    IDLINE(l, yo, r, yo, color, br);
    IDLINE(xo, b, xo, t, color, br);
    const Coord tic = 10;
    Coord x, y;

    Coord dtic = (r - l) / xtic;
    Coord dx = (x2 - x1) / xtic;
    int i;
    for (i = 0; i <= xtic; ++i) {
        x = l + i * dtic;
        IDLINE(x, yo, x, yo + tic, color, br);
        tic_label(x, yo - 5, x1 + i * dx, .5, 1, c);
    }
    dtic = (t - b) / ytic;
    dx = (y2 - y1) / ytic;
    for (i = 0; i <= ytic; ++i) {
        y = b + i * dtic;
        IDLINE(xo, y, xo + tic, y, color, br);
        tic_label(xo - 5, y, y1 + i * dx, 1, .5, c);
    }

    c->pop_transform();
    IfIdraw(end());
}

void AxisBackground::tic_label(Coord x1, Coord y1, Coord val, float xa, float ya, Canvas* c) const {
    char buf[20];
    sprintf(buf, "%g", val);
    Glyph* g = new Label(buf, WidgetKit::instance()->font(), Appear::default_color());
    g->ref();
    Requisition req;
    g->request(req);
    Allocation a;
    a.x_allotment().origin(x1 - xa * req.x_requirement().natural());
    a.y_allotment().origin(y1 - ya * req.y_requirement().natural());
    g->draw(c, a);
    g->unref();
    if (OcIdraw::idraw_stream) {
        Transformer t;
        t.translate(a.x(), a.y());
        OcIdraw::text(c, buf, t, NULL, Appear::default_color());
    }
}

#endif
