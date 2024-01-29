#include <../../nrnconf.h>
#if HAVE_IV  // to end of file

#include <InterViews/canvas.h>
#include <InterViews/transformer.h>
#include <InterViews/label.h>
#include <InterViews/hit.h>
#include <InterViews/cursor.h>
#include <IV-look/kit.h>

#include <stdio.h>
#include <string.h>

#include "ivoc.h"
#include "mymath.h"
#include "rubband.h"
#include "graph.h"
#include "hocmark.h"
#include "utility.h"

#define LineRubberMarker_event_   "Crosshair Graph"
#define DeleteLabelHandler_event_ "Delete Graph"
#define ChangeLabelHandler_event_ "ChangeText"
#define DeleteLineHandler_event_  "Delete Graph"
#define LinePicker_event_         "Pick Graph"
#define MoveLabelBand_press_      "MoveText Graph"

extern double hoc_cross_x_, hoc_cross_y_;

class LineRubberMarker: public Rubberband {
  public:
    LineRubberMarker(GPolyLine*, RubberAction*, Canvas* c = NULL);
    LineRubberMarker(Coord, Coord, RubberAction*, Canvas* c = NULL);
    virtual ~LineRubberMarker();
    virtual bool event(Event&);
    virtual void undraw(Coord, Coord);
    virtual void draw(Coord, Coord);
    int index() {
        return index_;
    }

  private:
    GPolyLine* gl_;
    Label* label_;
    Coord x_, y_;
    int index_;
#if defined(WIN32)
    CopyString def_str_;
#endif
};

class MoveLabelBand: public Rubberband {
  public:
    MoveLabelBand(GLabel*, RubberAction*, Canvas* = NULL);
    virtual ~MoveLabelBand();
    virtual void draw(Coord, Coord);
    virtual void press(Event&);
    virtual void release(Event&);

  private:
    GLabel* gl_;
    GLabel* label_;
    GlyphIndex index_;
    Allocation a_;
    Coord x0_, y0_;
};

/*static*/ class DeleteLabelHandler: public Handler {
  public:
    DeleteLabelHandler(GLabel*);
    ~DeleteLabelHandler();
    virtual bool event(Event&);

  private:
    GLabel* gl_;
};

/*static*/ class ChangeLabelHandler: public Handler {
  public:
    ChangeLabelHandler(GLabel*);
    ~ChangeLabelHandler();
    virtual bool event(Event&);

  private:
    GLabel* gl_;
};

/*static*/ class DeleteLineHandler: public Handler {
  public:
    DeleteLineHandler(GPolyLine*);
    ~DeleteLineHandler();
    virtual bool event(Event&);

  private:
    GPolyLine* gpl_;
};

/*static*/ class LinePicker: public Rubberband {
  public:
    LinePicker(GPolyLine*);
    ~LinePicker();
    virtual void press(Event&);
    virtual void release(Event&);

  private:
    void common();

  private:
    GPolyLine* gpl_;
    const Color* c_;
};

void GraphLine::pick(Canvas* c, const Allocation& a, int depth, Hit& h) {
    GPolyLine::pick(c, a, depth, h);
}

void GPolyLine::pick(Canvas* c, const Allocation&, int depth, Hit& h) {
    if (h.count() && h.target(depth, 0)) {
        return;
    }
    if (h.event() && h.event()->type() == Event::down &&
        h.event()->pointer_button() == Event::left) {
        Coord x = h.left(), y = h.bottom();
        switch (XYView::current_pick_view()->scene()->tool()) {
        case Graph::CROSSHAIR:
            if (near(x, y, 10, c->transformer())) {
                h.target(depth, this, 0, new LineRubberMarker(this, NULL, c));
            }
            break;
        case Scene::DELETE:
            if (near(x, y, 10, c->transformer())) {
                h.target(depth, this, 0, new DeleteLineHandler(this));
            }
            break;
        case Graph::CHANGECOLOR:
            if (near(x, y, 10, c->transformer())) {
                XYView::current_pick_view()->scene()->change_line_color(this);
            }
            break;
        case Graph::PICK:
            if (near(x, y, 5, c->transformer())) {
                h.target(depth, this, 0, new LinePicker(this));
            }
            break;
        }
    }
}
void HocMark::pick(Canvas* c, const Allocation& a, int depth, Hit& h) {
    if (h.count() && h.target(depth, 0)) {
        return;
    }
    if (h.event() && h.event()->type() == Event::down &&
        h.event()->pointer_button() == Event::left) {
        Coord x = h.left(), y = h.bottom();
        switch (XYView::current_pick_view()->scene()->tool()) {
        case Graph::CROSSHAIR:
            h.target(depth, this, 0, new LineRubberMarker(a.x(), a.y(), NULL, c));
            break;
        }
    }
}

bool GPolyLine::near(Coord xcm, Coord ycm, float epsilon, const Transformer& t) const {
    if (x_->count() <= 0) {
        return false;
    }
    int index = nearest(xcm, ycm, t);
    Coord xc, yc, x1, x2, y1, y2;
    x1 = x(index);
    y1 = y(index);
    if (index < x_->count() - 1) {
        x2 = x(index + 1);
        y2 = y(index + 1);
    } else {
        x2 = x1;
        y2 = y1;
    }
    // printf("nearest model %g %g to mouse %g %g\n", x1, y1, xcm, ycm);
    t.transform(xcm, ycm, xc, yc);
    t.transform(x1, y1);
    t.transform(x2, y2);
    // printf("nearest mouse %g %g box %g %g %g %g\n", xc, yc, x1, y1, x2, y2);
    return MyMath::near_line(xc, yc, x1, y1, x2, y2, epsilon);
}

int GPolyLine::nearest(Coord x1, Coord y1, const Transformer& t, int index_begin) const {
    int index, i;
    int count = x_->count();
    Coord x, y, xt, yt;
    t.transform(x1, y1, x, y);

#define Norm2(lval, arg)                                     \
    t.transform(x_->get_val(arg), y_->get_val(arg), xt, yt); \
    lval = MyMath::norm2(x - xt, y - yt);

    float dxmin, dx;
    if (index_begin < 0) {
        index = 0;
        Norm2(dxmin, 0);
        for (i = 1; i < count; ++i) {
            Norm2(dx, i);
            if (dx < dxmin) {
                dxmin = dx;
                index = i;
            }
        }
    } else {
        float dxleft, dxright;
        i = index = index_begin;
        Norm2(dxmin, i);
        dxleft = dxright = dxmin;
        if (i - 1 >= 0) {
            Norm2(dxleft, i - 1);
        }
        if (i + 1 > count) {
            Norm2(dxright, i + 1);
        }
        if (dxright < dxleft) {
            while (++i < count) {
                Norm2(dx, i);
                if (dx < dxmin) {
                    dxmin = dx;
                    index = i;
                } else {
                    break;
                }
            }
        } else {
            while (--i >= 0) {
                Norm2(dx, i);
                if (dx < dxmin) {
                    dxmin = dx;
                    index = i;
                } else {
                    break;
                }
            }
        }
    }
    return index;
}

LineRubberMarker::LineRubberMarker(GPolyLine* gl, RubberAction* ra, Canvas* c)
    : Rubberband(ra, c) {
    // printf("LineRubberMarker\n");
    gl_ = gl;
    Resource::ref(gl);
    label_ = NULL;
    index_ = -1;
}
LineRubberMarker::LineRubberMarker(Coord x, Coord y, RubberAction* ra, Canvas* c)
    : Rubberband(ra, c) {
    // printf("LineRubberMarker\n");
    gl_ = NULL;
    label_ = NULL;
    index_ = -1;
    x_ = x;
    y_ = y;
}
LineRubberMarker::~LineRubberMarker() {
    // printf("~LineRubberMarker\n");
    Resource::unref(gl_);
    Resource::unref(label_);
}
bool LineRubberMarker::event(Event& e) {
    if (Oc::helpmode()) {
        if (e.type() == Event::down) {
            Oc::help(LineRubberMarker_event_);
        }
        return true;
    }
    if (e.type() == Event::key) {
        char buf[2];
        if (e.mapkey(buf, 1) > 0) {
            if (gl_) {
                ((Graph*) XYView::current_pick_view()->scene())->cross_action(buf[0], gl_, index_);
            } else {
                ((Graph*) XYView::current_pick_view()->scene())->cross_action(buf[0], x_, y_);
            }
        }
        return true;
    } else {
#if defined(WIN32)
        if (e.type() == Event::down) {
            def_str_ = ((DismissableWindow*) canvas()->window())->name();
        } else if (e.type() == Event::up) {
            ((DismissableWindow*) canvas()->window())->name(def_str_.string());
        }
#endif
        return Rubberband::event(e);
    }
}
void LineRubberMarker::undraw(Coord, Coord) {
    Coord x, y;
    transformer().transform(x_, y_, x, y);
    Canvas* c = canvas();
    Transformer identity;
    c->push_transform();
    c->transformer(identity);
#if !defined(WIN32)
    Allocation a;
    a.allot_x(Allotment(x + 20, 0, 0));
    a.allot_y(Allotment(y, 0, 0));
    label_->draw(c, a);
#endif
    c->line(x - 10, y, x + 10, y, Rubberband::color(), Rubberband::brush());
    c->line(x, y - 10, x, y + 10, Rubberband::color(), Rubberband::brush());
    c->pop_transform();
}
void LineRubberMarker::draw(Coord x, Coord y) {
    // printf("draw %g %g", x, y);
    Coord x1, y1;
    transformer().inverse_transform(x, y, x1, y1);
    // printf(" model %g %g", x1, y1);
    if (gl_) {
        index_ = gl_->nearest(x1, y1, transformer(), index_);
        x_ = gl_->x(index_);
        y_ = gl_->y(index_);
        // printf("  on line %g %g\n", x_, y_);
    }
    char s[50];

#if defined(WIN32)
    Sprintf(s, "crosshair x=%g y=%g", x_, y_);
    ((DismissableWindow*) canvas()->window())->name(s);
#else
    Sprintf(s, "(%g,%g)", x_, y_);
    Resource::unref(label_);
    label_ = new Label(s, WidgetKit::instance()->font(), Rubberband::color());
#endif
    hoc_cross_x_ = x_;
    hoc_cross_y_ = y_;
    undraw(0, 0);
}

void GLabel::pick(Canvas* c, const Allocation&, int depth, Hit& h) {
    if (h.count() && h.target(depth, 0)) {
        return;
    }
    if (h.event() && h.event()->type() == Event::down &&
        h.event()->pointer_button() == Event::left) {
        // printf("GLabel picked %s\n", text_.string());
        switch (XYView::current_pick_view()->scene()->tool()) {
        case Scene::MOVE:
            h.target(depth, this, 0, new MoveLabelBand(this, NULL, c));
            break;
        case Scene::DELETE:
            h.target(depth, this, 0, new DeleteLabelHandler(this));
            break;
        case Scene::CHANGECOLOR:
            XYView::current_pick_view()->scene()->change_label_color(this);
            break;
        case Graph::CHANGELABEL:
            h.target(depth, this, 0, new ChangeLabelHandler(this));
        }
    }
}

DeleteLabelHandler::DeleteLabelHandler(GLabel* gl) {
    //	printf("DeleteLabelHandler\n");
    gl_ = gl;
}

DeleteLabelHandler::~DeleteLabelHandler() {
    //	printf("~DeleteLabelHandler\n");
}
bool DeleteLabelHandler::event(Event& e) {
    if (Oc::helpmode()) {
        if (e.type() == Event::down) {
            Oc::help(DeleteLabelHandler_event_);
        }
        return true;
    }
    XYView::current_pick_view()->scene()->delete_label(gl_);
    return true;
}

ChangeLabelHandler::ChangeLabelHandler(GLabel* gl) {
    //	printf("ChangeLabelHandler\n");
    gl_ = gl;
}

ChangeLabelHandler::~ChangeLabelHandler() {
    //	printf("~ChangeLabelHandler\n");
}
bool ChangeLabelHandler::event(Event& e) {
    if (Oc::helpmode()) {
        if (e.type() == Event::down) {
            Oc::help(ChangeLabelHandler_event_);
        }
        return true;
    }
    char buf[200];
    strcpy(buf, gl_->text());
    GLabel* gl = (GLabel*) gl_->clone();
    gl->ref();
    if (Graph::label_chooser("Modify Label", buf, gl, e.pointer_root_x(), e.pointer_root_y())) {
        ((Graph*) (XYView::current_pick_view()->scene()))->change_label(gl_, buf, gl);
    }
    gl->unref();
    return true;
}

void Scene::change_label_color(GLabel* gl) {
    printf("No method for changeing label color %s\n", gl->text());
}

void Scene::change_line_color(GPolyLine*) {
    printf("No method for changeing line color \n");
}

void Scene::delete_label(GLabel* gl) {
    printf("No method for deleting label %s\n", gl->text());
}

DeleteLineHandler::DeleteLineHandler(GPolyLine* gpl) {
    //	printf("DeleteLineHandler\n");
    gpl_ = gpl;
}

DeleteLineHandler::~DeleteLineHandler() {
    //	printf("~DeleteLineHandler\n");
}
bool DeleteLineHandler::event(Event& e) {
    if (Oc::helpmode()) {
        if (e.type() == Event::down) {
            Oc::help(DeleteLineHandler_event_);
        }
        return true;
    }
    Scene* s = XYView::current_pick_view()->scene();
    GlyphIndex i = s->glyph_index(gpl_);
    s->modified(i);
    s->damage(i);
    gpl_->erase_line(s, i);
    return true;
}

LinePicker::LinePicker(GPolyLine* gpl)
    : Rubberband() {
    //	printf("LinePicker\n");
    gpl_ = gpl;
}

LinePicker::~LinePicker() {
    //	printf("~LinePicker\n");
}
void LinePicker::press(Event&) {
    const Color* c;
    if (Oc::helpmode()) {
        Oc::help(LinePicker_event_);
        return;
    }
    c_ = gpl_->color();
    c = colors->color(2);
    if (c == c_) {
        c = colors->color(3);
    }
    gpl_->color(c);
    gpl_->pick_vector();
    common();
}
void LinePicker::release(Event&) {
    gpl_->color(c_);
    common();
}

void LinePicker::common() {
    Scene* s = XYView::current_pick_view()->scene();
    GlyphIndex i = s->glyph_index(gpl_);
    s->modified(i);
    s->damage(i);
    if (gpl_->label() && (i = s->glyph_index(gpl_->label())) >= 0) {
        s->modified(i);
        s->damage(i);
    }
}

MoveLabelBand::MoveLabelBand(GLabel* gl, RubberAction* ra, Canvas* c)
    : Rubberband(ra, c) {
    //	printf("MoveLabelBand\n");
    gl_ = gl;
    gl_->ref();
    label_ = (GLabel*) gl_->clone();
    label_->ref();
    label_->color(Rubberband::color());
    Graph* gr = (Graph*) XYView::current_pick_view()->scene();
    index_ = gr->glyph_index(gl);
    gr->location(index_, x0_, y0_);
    if (gl_->fixed()) {
        transformer().transform(x0_, y0_);
    } else {
        XYView::current_pick_view()->view_ratio(x0_, y0_, x0_, y0_);
    }
    //	printf("MoveLabelBand label index %d (%g, %g)\n", index_, x0_, y0_);
    Allotment ax, ay;
    gr->allotment(index_, Dimension_X, ax);
    gr->allotment(index_, Dimension_Y, ay);
    a_.allot_x(ax);
    a_.allot_y(ay);
}

MoveLabelBand::~MoveLabelBand() {
    //	printf("~MoveLabelBand\n");
    Resource::unref(label_);
    Resource::unref(gl_);
}

void MoveLabelBand::press(Event&) {
    if (Oc::helpmode()) {
        Oc::help(MoveLabelBand_press_);
        return;
    }
    x0_ -= x_begin();
    y0_ -= y_begin();
#if !defined(WIN32)
    undraw(x(), y());  // so initial draw does not make it disappear
#endif
}

void MoveLabelBand::release(Event&) {
    if (Oc::helpmode()) {
        return;
    }
    Graph* gr = (Graph*) XYView::current_pick_view()->scene();
    Coord x1, y1, x2, y2;
    if (gl_->fixed()) {
        transformer().inverse_transform(x(), y(), x2, y2);
        transformer().inverse_transform(x_begin(), y_begin(), x1, y1);
    } else {
        x2 = x();
        y2 = y();
        x1 = x_begin();
        y1 = y_begin();
    }
    gr->location(index_, x0_, y0_);
    if (gl_->fixed()) {
        x1 = x0_ + x2 - x1;
        y1 = y0_ + y2 - y1;
    } else {
        XYView::current_pick_view()->view_ratio(x0_, y0_, x0_, y0_);
        XYView::current_pick_view()->ratio_view(x0_ + x2 - x1, y0_ + y2 - y1, x1, y1);
    }
    // printf("move to %g %g\n", x1, y1);
    gr->move(index_, x1, y1);
}

void MoveLabelBand::draw(Coord x, Coord y) {
    if (Oc::helpmode()) {
        return;
    }
    Canvas* c = canvas();
    // printf("MoveLabelBand::draw(%g, %g)\n", x, y);
    a_.x_allotment().origin(x + x0_);
    a_.y_allotment().origin(y + y0_);
#if defined(WIN32)
    c->rect(a_.x_allotment().begin(),
            a_.y_allotment().begin(),
            a_.x_allotment().end(),
            a_.y_allotment().end(),
            Rubberband::color(),
            Rubberband::brush());
#else
    label_->draw(c, a_);
#endif
}
#endif
