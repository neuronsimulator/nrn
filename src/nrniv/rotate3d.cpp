#include <../../nrnconf.h>
#if HAVE_IV // to end of file

#include <math.h>
#include <InterViews/canvas.h>
#include <IV-look/kit.h>
#include <InterViews/font.h>
#include <InterViews/polyglyph.h>
#include "rot3band.h"
#include <stdio.h>
#include "nrnoc2iv.h"
#include "shape.h"
#include "ivoc.h"

#define Rotate_		"Rotate3D PlotShape"

Rotation3d::Rotation3d() {
	identity();
	int i, j;
	for (i=0; i < 2; ++i) for (j=0; j < 3; ++j) o_[i][j] = 0.;
}

Rotation3d::~Rotation3d() {}

void Rotation3d::identity(){
	int i, j;
	for (i=0; i < 3; ++i) {
		for (j=0; j < 3; ++j) {
			a_[i][j] = 0.;
		}
		a_[i][i] = 1.;
	}
}

void Rotation3d::rotate_x(float radian){
	Rotation3d m;
	m.a_[1][1] = m.a_[2][2] = cos(radian);	
	m.a_[2][1] = - (m.a_[1][2] = sin(radian));
	post_multiply(m);
}
void Rotation3d::rotate_y(float radian){
	Rotation3d m;
	m.a_[2][2] = m.a_[0][0] = cos(radian);	
	m.a_[2][0] = - (m.a_[0][2] = sin(radian));
	post_multiply(m);
}
void Rotation3d::rotate_z(float radian){
	Rotation3d m;
	m.a_[0][0] = m.a_[1][1] = cos(radian);	
	m.a_[1][0] = - (m.a_[0][1] = sin(radian));
	post_multiply(m);
}

void Rotation3d::post_multiply(Rotation3d& m) { // r = m*r
	float x[3][3];
	int i, j, k;
	for (i=0; i < 3; ++i) {
		for (j=0; j < 3; ++j) {
			x[i][j] = 0;
			for (k=0; k < 3; ++k) {
				x[i][j] += m.a_[i][k]*a_[k][j];
			}
		}
	}
	for (i=0; i < 3; ++i) {
		for (j=0; j < 3; ++j) {
			a_[i][j] = x[i][j];
		}
	}
}

void Rotation3d::rotate(float x, float y, float z, float* tr) const {
	float r[3];
	r[0] = x; r[1] = y; r[2] = z;
	rotate(r, tr);
}

void Rotation3d::rotate(float* r, float* tr) const
{
	int i;
	float x[3];
	for (i=0; i < 3; ++i) {
		x[i] = r[i] - o_[0][i];
	}
	for (i=0; i < 3; ++i) {
		tr[i] = a_[i][0]*x[0] + a_[i][1]*x[1] + a_[i][2]*x[2] + o_[1][i];
	}
}

void Rotation3d::origin(float x, float y, float z) {
	o_[0][0] = x;
	o_[0][1] = y;
	o_[0][2] = z;
}
void Rotation3d::offset(float x, float y) {
	o_[1][0] = x;
	o_[1][1] = y;
	o_[1][2] = 0.;
}
	
void Rotation3d::x_axis(float& x, float& y) const {
	x = a_[0][0];
	y = a_[1][0];
}
void Rotation3d::y_axis(float& x, float& y) const {
	x = a_[0][1];
	y = a_[1][1];
}
void Rotation3d::z_axis(float& x, float& y) const {
	x = a_[0][2];
	y = a_[1][2];
}
void Rotation3d::inverse_rotate(float* tr, float* r) const
{
	int i;
	float x[3];
	for (i=0; i < 3; ++i) {
		x[i] = tr[i];
	}
	for (i=0; i < 3; ++i) {
		r[i] = a_[0][i]*x[0] + a_[1][i]*x[1] + a_[2][i]*x[2];
	}
}

Rotate3Band::Rotate3Band(Rotation3d* r3, RubberAction* ra, Canvas* c)
	: Rubberband(ra, c)
{
	if (r3) {
		rot_ = r3;
	}else{
		rot_ = new Rotation3d();
	}
	Resource::ref(rot_);
}

Rotate3Band::~Rotate3Band(){
	Resource::unref(rot_);
}

Rotation3d* Rotate3Band::rotation() { return rot_; }

bool Rotate3Band::event(Event& e) {
	const float deg = 3.14159265358979323846/18.;
	if (e.type() == Event::key) {
		undraw(x(), y());
		char buf[2];
		if (e.mapkey(buf, 1) > 0) switch (buf[0]) {
		case 'x':
			rot_->identity();
			rot_->rotate_y(3.14159265358979323846/2.);
			break;
		case 'y':
		case 'a':
			rot_->identity();
			rot_->rotate_x(3.14159265358979323846/2.);
			break;
		case 'z':
		case ' ':
			rot_->identity();
			break;
		case 'X':
			rot_->rotate_x(deg);
			break;
		case 'Y':
		case 'A':
			rot_->rotate_y(deg);
			break;
		case 'Z':
			rot_->rotate_z(deg);
			break;
		case 037&'x':
			rot_->rotate_x(-deg);
			break;
		case 037&'y':
		case 037&'a':
			rot_->rotate_y(-deg);
			break;
		case 037&'z':
			rot_->rotate_z(-deg);
			break;
		default:
			break;
		}
		draw(x(), y());
		return true;
	}else{
		return Rubberband::event(e);
	}
}

void Rotate3Band::help() {
	Oc::help(Rotate_);
}

void Rotate3Band::press(Event& e) {
	Canvas* c = canvas();
	c->push_transform();
	Transformer t;
	c->transformer(transformer());
	XYView* v = XYView::current_pick_view();
	c->fill_rect(v->left(), v->bottom(), v->right(), v->top(),
		Scene::default_background());
	c->pop_transform();

	x_old_ = x();
	y_old_ = y();
	ShapeScene* ss = (ShapeScene*)v->scene();
	Coord x1, y1;
	transformer().inverse_transform(x(), y(), x1, y1);
	ss->nearest(x1, y1);
	ShapeSection* ssec = ss->selected();
	Section* s = ssec->section();
	// this is the point we want to stay fixed.
	int i = ssec->get_coord(ss->arc_selected(), x1, y1);
	// what is it now
	float r[3];
	rot_->rotate(s->pt3d[i].x, s->pt3d[i].y, s->pt3d[i].z, r);
	rot_->origin(s->pt3d[i].x, s->pt3d[i].y, s->pt3d[i].z);
	rot_->offset(r[0], r[1]);
}

void Rotate3Band::drag(Event&) {
	float dx = x() - x_old_;
	float dy = y() - y_old_;	
//printf ("dx=%g dy=%g\n", dx,dy);
	rot_->rotate_x(dy/50);
	rot_->rotate_y(dx/50);
	x_old_ = x();
	y_old_ = y();
}

void Rotate3Band::draw(Coord, Coord) {
	Canvas* c = canvas();
	const Font* f = WidgetKit::instance()->font();
	float x, y;
	float x0, y0;

	c->push_transform();
	c->transformer(transformer());
#if 0
	hoc_Item* qsec;
	ForAllSections(sec) //{
#else
	PolyGlyph* sg = ((ShapeScene*)XYView::current_pick_view()->scene())
		->shape_section_list();
	GlyphIndex cnt = sg->count();
	for (GlyphIndex i=0; i < cnt; ++i) {
		Section* sec = ((ShapeSection*)sg->component(i))->section();
#endif
	   if (sec->npt3d) {
		float r[3];
		int i = 0;
		r[0] = sec->pt3d[i].x;
		r[1] = sec->pt3d[i].y;
		r[2] = sec->pt3d[i].z;
		rot_->rotate(r, r);
		c->move_to(r[0], r[1]);
		i = sec->npt3d - 1;
		r[0] = sec->pt3d[i].x;
		r[1] = sec->pt3d[i].y;
		r[2] = sec->pt3d[i].z;
		rot_->rotate(r, r);
		c->line_to(r[0], r[1]);
		c->stroke(color(), brush());
	   }
	}
	c->pop_transform();
	
	x0 = x_begin();
	y0 = y_begin();
	c->push_transform();
	Transformer t;
	c->transformer(t);
	c->new_path();
	Coord w = canvas()->width()/4;
	rot_->x_axis(x, y);
//printf("x_axis %g %g\n", x, y);
	c->line(x0, y0, x0+x*w, y0+y*w, color(), brush());
#ifndef WIN32
	c->character(f, 'x', f->width('x'), color(), x0+x*w*1.1, y0+y*w*1.1);
#endif
	rot_->y_axis(x, y);
//printf("y_axis %g %g\n", x, y);
	c->line(x0, y0, x0+x*w, y0+y*w, color(), brush());
#ifndef WIN32
	c->character(f, 'y', f->width('y'), color(), x0+x*w*1.1, y0+y*w*1.1);
#endif
	rot_->z_axis(x, y);
//printf("z_axis %g %g\n", x, y);
	c->line(x0, y0, x0+x*w, y0+y*w, color(), brush());
#ifndef WIN32
	c->character(f, 'z', f->width('z'), color(), x0+x*w*1.1, y0+y*w*1.1);
#endif
	c->pop_transform();
}
#endif
