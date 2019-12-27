#include <../../nrnconf.h>
#if HAVE_IV // to end of file

#include <InterViews/glyph.h>
#include <InterViews/session.h>
#include <InterViews/style.h>
#include <InterViews/canvas.h>
#include <InterViews/color.h>
#include <InterViews/hit.h>
#include <InterViews/brush.h>
#include <InterViews/transformer.h>
#include <IV-look/kit.h>
#include <OS/string.h>
#include "scenevie.h"
#include "rect.h"
#include "mymath.h"
#include "idraw.h"
#include <stdio.h>

Appear::Appear(const Color* c, const Brush* b) {
	color_ = NULL;
	brush_ = NULL;
	color(c);
	brush(b);
}
Appear::~Appear() {
	Resource::unref(color_);
	Resource::unref(brush_);
}

void Appear::color(const Color* c) {
	const Color* c1;
	if (c) {
		c1 = c;
	}else{
		c1 = default_color();
	}
	Resource::ref(c1);
	Resource::unref(color_);
	color_ = c1;
}
void Appear::brush(const Brush* b) {
	const Brush* b1;
	if (b) {
		b1 = b;
	}else{
		b1 = default_brush();
	}
	Resource::ref(b1);
	Resource::unref(brush_);
	brush_ = b1;
}

const Color* Appear::dc_;
const Brush* Appear::db_;

const Color* Appear::default_color() {
	return Scene::default_foreground();
}

const Brush* Appear::default_brush() {
	if (!db_) {
		Style* s = Session::instance()->style();
		Coord b = 0;
		s->find_attribute("default_brush", b);
		db_ = new Brush(b);
		Resource::ref(db_);
	}
	return db_;
}
	
//Rect
Rect::Rect(Coord l, Coord b, Coord w, Coord h, const Color* c, const Brush* br)
  : Appear(c, br) {
	left(l); bottom(b); width(w); height(h);
}

void Rect::request(Requisition& req) const {
	Requirement rx(width(), 0, 0, left()/width());
	Requirement ry(height(), 0, 0, bottom()/height());
	req.require_x(rx);
	req.require_y(ry);
}

void Rect::allocate(Canvas* c, const Allocation& a, Extension& ext) {
	ext.set(c, a);
	MyMath::extend(ext, 1);
}

void Rect::draw(Canvas* c, const Allocation& a) const {
	Coord x = a.x();
	Coord y = a.y();
	c->rect(x + left(), y+ bottom(), x + right(), y + top(), color(), brush());
}

void Rect::pick(Canvas*, const Allocation&, int depth, Hit& h) {
	Coord x = h.left();
	Coord y = h.bottom();
	if (MyMath::inside(x, y, left(), right(), bottom(), top())) {
		h.target(depth, this, 0);
	}
}

//Line
Line::Line(Coord dx, Coord dy, const Color* c, const Brush* b) : Appear(c, b) {
	dx_ = dx;
	dy_ = dy;
	x_ = 0;
	y_ = 0;
}
Line::Line(Coord dx, Coord dy, float xa, float ya,
   const Color* c, const Brush* b) : Appear(c, b) {
	dx_ = dx;
	dy_ = dy;
	x_ = -dx_*xa;
	y_ = -dy_*ya;
}
Line::~Line(){}

void Line::request(Requisition& req) const {
	Coord dx, dy;
	dx = (dx_)?dx_:1e-5;
	dy = (dy_)?dy_:1e-5;
	Requirement rx(dx, 0, 0, x_/dx);
	Requirement ry(dy, 0, 0, y_/dy);
	req.require_x(rx);
	req.require_y(ry);
}

void Line::allocate(Canvas* c, const Allocation& a, Extension& ext) {
	ext.set(c, a);
	MyMath::extend(ext, brush()->width()/2 + 1);
}

#if 0
void Line::draw(Canvas* c, const Allocation& a) const {
	Coord x = a.x() + x_;
	Coord y = a.y() + y_;
//printf("line %g %g %g %g\n", x, y, dx_, dy_);
	c->line(x, y, x + dx_, y + dy_, color(), brush());
	IfIdraw(line(c, x, y, x + dx_, y + dy_, color(), brush()));
}
#else
void Line::draw(Canvas* c, const Allocation& a) const {
        Coord x = a.x() + x_;
        Coord y = a.y() + y_; 
//printf("line %g %g %g %g\n", x, y, dx_, dy_);

        c->new_path();
        c->move_to(x, y);
        c->line_to(x+dx_,y+dy_);
             
// transform the line to get thickness right on printer
	XYView::current_draw_view()->stroke(c, color(), brush());
	IfIdraw(line(c, x, y, x + dx_, y + dy_, color(), brush()));
}
#endif

void Line::pick(Canvas* c, const Allocation& a, int depth, Hit& h) {
	Coord l=a.x() + x_, b=a.y() + y_, r=a.x()+ x_ + dx_, t=a.y()+ y_ + dy_;
	Coord x = h.left();
	Coord y = h.bottom();
	if (MyMath::inside(x,y,l,b,r,t)) {
		float epsilon = 5;
		const Transformer& tr = c->transformer();
		tr.transform(x,y);
		tr.transform(l,b);
		tr.transform(r,t);
		if (MyMath::near_line(x, y, l, b, r, t, epsilon)) {
			h.target(depth, this, 0);
		}
	}
}

//Circle
Circle::Circle(float radius, bool filled, const Color* c, const Brush* b)
	:Appear(c, b)
{
	radius_ = radius;
	filled_ = filled;
}

Circle::~Circle() {}

void Circle::request(Requisition& req) const {
    Coord w = brush()->width();
    Coord diameter = radius_ + radius_ + w + w;
    Requirement rx(diameter, 0, 0, 0.5);
    Requirement ry(diameter, 0, 0, 0.5);
    req.require(Dimension_X, rx);
    req.require(Dimension_Y, ry);
}

void Circle::allocate(Canvas* c, const Allocation& a, Extension& ext) {
	ext.merge(c, a);
	MyMath::extend(ext, brush()->width()/2 + 1);
}

void Circle::draw(Canvas* c, const Allocation& a) const {
    const Coord r = radius_, x = a.x(), y = a.y();
    const Coord p0 = 1.00000000 * r;
    const Coord p1 = 0.89657547 * r;   // cos 30 * sqrt(1 + tan 15 * tan 15)
    const Coord p2 = 0.70710678 * r;   // cos 45
    const Coord p3 = 0.51763809 * r;   // cos 60 * sqrt(1 + tan 15 * tan 15)
    const Coord p4 = 0.26794919 * r;   // tan 15
    c->new_path();
    c->move_to(x+p0, y);
    c->curve_to(x+p2, y+p2, x+p0, y+p4, x+p1, y+p3);
    c->curve_to(x, y+p0, x+p3, y+p1, x+p4, y+p0);
    c->curve_to(x-p2, y+p2, x-p4, y+p0, x-p3, y+p1);
    c->curve_to(x-p0, y, x-p1, y + p3, x-p0, y+p4);
    c->curve_to(x-p2, y-p2, x-p0, y-p4, x-p1, y-p3);
    c->curve_to(x, y-p0, x-p3, y-p1, x-p4, y-p0);
    c->curve_to(x+p2, y-p2, x+p4, y-p0, x+p3, y-p1);
    c->curve_to(x+p0, y, x+p1, y-p3, x+p0, y-p4);
    c->close_path();
    if (filled_) {
    	c->fill(color());
    }else{
	c->stroke(color(), brush());
    }
    IfIdraw(ellipse(c, x, y, r, r, color(), brush(), filled_));
}


//Rectangle
Rectangle::Rectangle(float height, float width, bool filled, const Color* c, const Brush* b)
	:Appear(c, b)
{
	height_ = height;
	width_ = width;	
	filled_ = filled;
}

Rectangle::~Rectangle() {}

void Rectangle::request(Requisition& req) const {
    Coord w = brush()->width();
    Requirement rx(width_ + w + w, 0, 0, 0.5);
    Requirement ry(height_ + w + w, 0, 0, 0.5);
    req.require(Dimension_X, rx);
    req.require(Dimension_Y, ry);
}

void Rectangle::allocate(Canvas* c, const Allocation& a, Extension& ext) {
	ext.merge(c, a);
	MyMath::extend(ext, brush()->width()/2 + 1);
}

void Rectangle::draw(Canvas* c, const Allocation& a) const {
    const Coord dx = width_/2, dy = height_/2, x = a.x(), y = a.y();

    if (filled_) {
      c->fill_rect(x-dx,y-dy,x+dx,y+dy,color());
    } else {
      c->rect(x-dx,y-dy,x+dx,y+dy,color(),brush());
    }
    IfIdraw(rect(c, x-dx, y-dy, x+dx, y+dy, color(), brush(), filled_));
}


//Triangle
Triangle::Triangle(float side, bool filled, const Color* c, const Brush* b)
	:Appear(c, b)
{
	side_ = side/2;
	filled_ = filled;
}

Triangle::~Triangle() {}

void Triangle::request(Requisition& req) const {
    Coord w = brush()->width();
    Requirement rx(side_ + side_+ w + w, 0, 0, 0.5);
    Requirement ry((side_ + side_)*1.1547 + w + w, 0, 0, 0.5);
    req.require(Dimension_X, rx);
    req.require(Dimension_Y, ry);
}

void Triangle::allocate(Canvas* c, const Allocation& a, Extension& ext) {
	ext.merge(c, a);
	MyMath::extend(ext, brush()->width()/2 + 1);
}

void Triangle::draw(Canvas* c, const Allocation& a) const {
    const Coord x = a.x(), y = a.y();
    const Coord radius = 1.1547*side_;

    c->new_path();
    c->move_to(x, y+radius);
    c->line_to(x+side_, y-radius);
    c->line_to(x-side_, y-radius);
    c->close_path();
    if (filled_) {
      c->fill(color());
    } else {
      c->stroke(color(),brush());
    }

    Coord *xList = new Coord[4];
    Coord *yList = new Coord[4];
    
    xList[0] = x;
    xList[1] = x+side_;
    xList[2] = x-side_;
    xList[3] = x;

    yList[0] = y+radius;
    yList[1] = y-radius;
    yList[2] = y-radius;
    yList[3] = y+radius;

    IfIdraw(polygon(c, 3, xList, yList, color(), brush(), filled_));

    delete [] xList;
    delete [] yList;
}



#endif
