#include <../../nrnconf.h>
#if HAVE_IV // to end of file

#include <InterViews/session.h>
#include <InterViews/display.h>
#include <InterViews/color.h>
#include <InterViews/brush.h>
#include <InterViews/canvas.h>
#include <InterViews/printer.h>
#include <InterViews/window.h>
#include <InterViews/transformer.h>
#include "rubband.h"

#include <OS/math.h>
#include <stdio.h>

RubberAction::RubberAction() {
}
RubberAction::~RubberAction() {}
void RubberAction::execute(Rubberband* rb) {
	Resource::unref(rb);
}
void RubberAction::help() {
	printf("no help for this Rubberband action\n");
}

OcHandler::OcHandler() {}
OcHandler::~OcHandler() {}
void OcHandler::help() {
	printf("no help for this handler\n");
}

Rubberband* Rubberband::current_;

Rubberband::Rubberband(RubberAction* ra, Canvas* c) {
//printf("Rubberband\n");
	canvas(c);
	ra_ = ra;
	Resource::ref(ra_);
	if (!xor_color_) {
		xor_color_ = new Color(0,0,0,1,Color::Xor);
		Resource::ref(xor_color_);
		brush_ = new Brush(0);
		Resource::ref(brush_);
	}
}

Rubberband::~Rubberband() {
//printf("~Rubberband\n");
	Resource::unref(ra_);
}

void Rubberband::help() {
	if (ra_) {
		ra_->help();
	}
}

void Rubberband::canvas(Canvas* c) {
	canvas_ = c;
	if (c) {
		t_ = c->transformer();
	}
}

const Color* Rubberband::xor_color_;
const Color* Rubberband::color() { return xor_color_;}
const Brush* Rubberband::brush_;
const Brush* Rubberband::brush() { return brush_;}

bool Rubberband::event(Event& e) {
	e_ = &e;
	EventType type = e.type();
	switch (type) {
	case Event::down:
		current_ = this;
//printf("Rubberband::event down\n");
		Resource::ref(this);
		if (canvas_) {
			rubber_on(canvas_);
		}
		e.grab(this);
#ifdef WIN32
		e.window()->grab_pointer();
#endif
		x_ = x_begin_ = e.pointer_x();
		y_ = y_begin_ = e.pointer_y();
		press(e);
		draw(x_, y_);
		break;
	case Event::motion:
		undraw(x_, y_);
		x_ = e.pointer_x();
		y_ = e.pointer_y();
		drag(e);
		draw(x_, y_);
		break;
	case Event::up:
//printf("Rubberband::event up\n");
		current_ = NULL;
		e.ungrab(this);
#ifdef WIN32
		e.window()->ungrab_pointer();
#endif
		undraw(x_, y_);
		if (canvas_) {
			rubber_off(canvas_);
		}
		x_ = e.pointer_x();
		y_ = e.pointer_y();
		release(e);
		if (ra_) {
			ra_->execute(this);
		}
		Resource::unref(this); //destroyed here if user never ref'ed it
		break;
	}		
	return true;
}

void Rubberband::draw(Coord, Coord){}
void Rubberband::undraw(Coord x, Coord y){ draw(x,y); }
void Rubberband::press(Event&){}
void Rubberband::drag(Event&){}
void Rubberband::release(Event&){}
void Rubberband::snapshot(Printer* pr) {
	Canvas* can = canvas();
	canvas(pr);
	draw(x(), y());
	canvas(can);
}


//RubberRect
RubberRect::RubberRect(RubberAction* ra, Canvas* c) : Rubberband(ra, c){}
RubberRect::~RubberRect(){}
void RubberRect::draw(Coord x, Coord y) {
//printf("RubberRect::draw(%g %g)\n", x, y);
	Coord x1, y1, x2, y2;
	x1 = Math::min(x, x_begin());
	y1 = Math::min(y, y_begin());
	x2 = Math::max(x, x_begin());
	y2 = Math::max(y, y_begin());
	if (x1 < x2 && y1 < y2) {
		Canvas* c = canvas();
		c->push_transform();
		Transformer t;
		c->transformer(t);
		c->new_path();
		c->rect(x1, y1, x2, y2, Rubberband::color(), Rubberband::brush());
//printf("rect %g %g %g %g\n", x1, y1, x2, y2);
		c->pop_transform();
	}
}
void RubberRect::help() {
	Rubberband::help();
	printf("RubberRect::help\n");
}

void RubberRect::get_rect_canvas(Coord& x1, Coord& y1, Coord& x2, Coord& y2)const {
	x1 = Math::min(x(), x_begin());
	y1 = Math::min(y(), y_begin());
	x2 = Math::max(x(), x_begin());
	y2 = Math::max(y(), y_begin());
}
void RubberRect::get_rect(Coord& x1, Coord& y1, Coord& x2, Coord& y2)const {
	get_rect_canvas(x1,y1,x2,y2);
	const Transformer& t = transformer();
	t.inverse_transform(x1, y1);
	t.inverse_transform(x2, y2);
}

//RubberLine
RubberLine::RubberLine(RubberAction* ra, Canvas* c) : Rubberband(ra, c){}
RubberLine::~RubberLine(){}
void RubberLine::help() {
	Rubberband::help();
	printf("RubberRect::help\n");
}

void RubberLine::draw(Coord x, Coord y) {
//printf("RubberLine::draw(%g %g)\n", x, y);
		Canvas* c = canvas();
		c->push_transform();
		Transformer t;
		c->transformer(t);
		c->new_path();
		c->line(x_begin(), y_begin(), x, y, Rubberband::color(), Rubberband::brush());
		c->pop_transform();
}
void RubberLine::get_line_canvas(Coord& x1, Coord& y1, Coord& x2, Coord& y2)const {
	x1 = x_begin();
	y1 = y_begin();
	x2 = x();
	y2 = y();
}
void RubberLine::get_line(Coord& x1, Coord& y1, Coord& x2, Coord& y2)const {
	get_line_canvas(x1,y1,x2,y2);
	const Transformer& t = transformer();
	t.inverse_transform(x1, y1);
	t.inverse_transform(x2, y2);
}
#endif
