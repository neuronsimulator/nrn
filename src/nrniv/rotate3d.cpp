#include <../../nrnconf.h>
#if HAVE_IV  // to end of file

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

#define Rotate_ "Rotate3D PlotShape"

Rotation3d::Rotation3d() {
    identity();
}

void Rotation3d::identity() {
    mat.setIdentity();
    tr.setIdentity();
    orig.setIdentity();
}

void Rotation3d::rotate_x(float radian) {
    mat *= Eigen::AngleAxisf(radian, Eigen::Vector3f::UnitX()).toRotationMatrix();
}
void Rotation3d::rotate_y(float radian) {
    mat *= Eigen::AngleAxisf(-radian, Eigen::Vector3f::UnitY()).toRotationMatrix();
}
void Rotation3d::rotate_z(float radian) {
    mat *= Eigen::AngleAxisf(radian, Eigen::Vector3f::UnitZ()).toRotationMatrix();
}

Eigen::Vector3f Rotation3d::rotate(const Eigen::Vector3f& v) const {
    auto r = orig * v;
    auto t = mat.transpose() * r;
    auto e = tr * t;
    return e;
}

void Rotation3d::origin(float x, float y, float z) {
    orig = Eigen::Translation<float, 3>(-x, -y, -z);
}
void Rotation3d::offset(float x, float y) {
    tr = Eigen::Translation<float, 3>(x, y, 0.);
}

std::array<float, 2> Rotation3d::x_axis() const {
    return {mat(0, 0), mat(0, 1)};
}
std::array<float, 2> Rotation3d::y_axis() const {
    return {mat(0, 1), mat(1, 1)};
}
std::array<float, 2> Rotation3d::z_axis() const {
    return {mat(0, 2), mat(2, 1)};
}

Rotate3Band::Rotate3Band(Rotation3d* r3, RubberAction* ra, Canvas* c)
    : Rubberband(ra, c) {
    if (r3) {
        rot_ = r3;
    } else {
        rot_ = new Rotation3d();
    }
    Resource::ref(rot_);
}

Rotate3Band::~Rotate3Band() {
    Resource::unref(rot_);
}

Rotation3d* Rotate3Band::rotation() {
    return rot_;
}

bool Rotate3Band::event(Event& e) {
    const float deg = 3.14159265358979323846 / 18.;
    if (e.type() == Event::key) {
        undraw(x(), y());
        char buf[2];
        if (e.mapkey(buf, 1) > 0)
            switch (buf[0]) {
            case 'x':
                rot_->identity();
                rot_->rotate_y(3.14159265358979323846 / 2.);
                break;
            case 'y':
            case 'a':
                rot_->identity();
                rot_->rotate_x(3.14159265358979323846 / 2.);
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
            case 037 & 'x':
                rot_->rotate_x(-deg);
                break;
            case 037 & 'y':
            case 037 & 'a':
                rot_->rotate_y(-deg);
                break;
            case 037 & 'z':
                rot_->rotate_z(-deg);
                break;
            default:
                break;
            }
        draw(x(), y());
        return true;
    } else {
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
    c->fill_rect(v->left(), v->bottom(), v->right(), v->top(), Scene::default_background());
    c->pop_transform();

    x_old_ = x();
    y_old_ = y();
    ShapeScene* ss = (ShapeScene*) v->scene();
    Coord x1, y1;
    transformer().inverse_transform(x(), y(), x1, y1);
    ss->nearest(x1, y1);
    ShapeSection* ssec = ss->selected();
    Section* s = ssec->section();
    // this is the point we want to stay fixed.
    int i = ssec->get_coord(ss->arc_selected(), x1, y1);
    // what is it now
    auto r = rot_->rotate({s->pt3d[i].x, s->pt3d[i].y, s->pt3d[i].z});
    rot_->origin(s->pt3d[i].x, s->pt3d[i].y, s->pt3d[i].z);
    rot_->offset(r[0], r[1]);
}

void Rotate3Band::drag(Event&) {
    float dx = x() - x_old_;
    float dy = y() - y_old_;
    // printf ("dx=%g dy=%g\n", dx,dy);
    rot_->rotate_x(dy / 50);
    rot_->rotate_y(dx / 50);
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
    PolyGlyph* sg = ((ShapeScene*) XYView::current_pick_view()->scene())->shape_section_list();
    GlyphIndex cnt = sg->count();
    for (GlyphIndex i = 0; i < cnt; ++i) {
        Section* sec = ((ShapeSection*) sg->component(i))->section();
        if (sec->npt3d) {
            int i = 0;
            auto r = rot_->rotate({sec->pt3d[i].x, sec->pt3d[i].y, sec->pt3d[i].z});
            c->move_to(r[0], r[1]);
            i = sec->npt3d - 1;
            r = rot_->rotate({sec->pt3d[i].x, sec->pt3d[i].y, sec->pt3d[i].z});
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
    Coord w = canvas()->width() / 4;
    {
        auto [x, y] = rot_->x_axis();
        c->line(x0, y0, x0 + x * w, y0 + y * w, color(), brush());
#ifndef WIN32
        c->character(f, 'x', f->width('x'), color(), x0 + x * w * 1.1, y0 + y * w * 1.1);
#endif
    }
    {
        auto [x, y] = rot_->y_axis();
        c->line(x0, y0, x0 + x * w, y0 + y * w, color(), brush());
#ifndef WIN32
        c->character(f, 'y', f->width('y'), color(), x0 + x * w * 1.1, y0 + y * w * 1.1);
#endif
    }
    {
        auto [x, y] = rot_->z_axis();
        c->line(x0, y0, x0 + x * w, y0 + y * w, color(), brush());
#ifndef WIN32
        c->character(f, 'z', f->width('z'), color(), x0 + x * w * 1.1, y0 + y * w * 1.1);
#endif
    }
    c->pop_transform();
}
#endif
