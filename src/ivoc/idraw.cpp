#include <../../nrnconf.h>
#if HAVE_IV  // to end of file

#include <stdio.h>
#include <fstream>

#include <InterViews/color.h>
#include <InterViews/brush.h>
#include <InterViews/font.h>
#include <IV-look/kit.h>
#include <InterViews/transformer.h>
#include <InterViews/session.h>
#include <InterViews/style.h>
#include <OS/string.h>

#include "hocdec.h"
#include "oc_ansi.h"
#include "scenevie.h"
#include "mymath.h"
#include "idraw.h"

#define out *OcIdraw::idraw_stream

bool OcIdraw::closed_ = false;
bool OcIdraw::curved_ = false;
int OcIdraw::ipath_ = 0;
int OcIdraw::capacity_ = 0;
Coord* OcIdraw::xpath_ = 0;
Coord* OcIdraw::ypath_ = 0;

void OcIdraw::prologue() {
    std::filebuf ibuf;
    Style* s = Session::instance()->style();
    CopyString name;
    if (!s->find_attribute("pwm_idraw_prologue", name)) {
        printf("can't find the \"pwm_idraw_prologue\" attribute\n");
        printf("will have to prepend the prologue by hand before reading with idraw.\n");
        return;
    }
    name = expand_env_var(name.string());
#if defined(WIN32)
    if (!ibuf.open(name.string(), std::ios::in)) {
#else
    if (!ibuf.open(name.string(), std::ios::in)) {
#endif
        printf("can't open the idraw prologue in %s\n", name.string());
        return;
    }
    out << &ibuf << std::endl;
    ibuf.close();
    if (!xpath_) {
        capacity_ = 10;
        xpath_ = new Coord[capacity_];
        ypath_ = new Coord[capacity_];
    }
}

void OcIdraw::epilog() {
    out << "\
End %I eop\n\
showpage\n\n\
%%Trailer\n\n\
end\
" << std::endl;
}

static void transformer(const Transformer& t) {
    float a00, a01, a10, a11, a20, a21;
    t.matrix(a00, a01, a10, a11, a20, a21);
    char buf[200];
    Sprintf(buf, "[ %g %g %g %g %g %g ] concat", a00, a01, a10, a11, a20, a21);
    out << buf << std::endl;
}

static char* hidepar(const char* s) {
    static char buf[256];
    const char* ps;
    char* pbuf;
    for (ps = s, pbuf = buf; *ps;) {
        if (*ps == '(' || *ps == ')') {
            *pbuf++ = '\\';
        }
        *pbuf++ = *ps++;
    }
    *pbuf = '\0';
    return buf;
}

static void rgbcolor(const Color* c, ColorIntensity& r, ColorIntensity& g, ColorIntensity& b) {
    // always use black for default_foreground.
    if (c == Scene::default_foreground()) {
        r = 0;
        g = 0;
        b = 0;
    } else {
        c->intensities(r, g, b);
    }
}

static void common_pict() {
    out << "\n\
Begin %I Pict\n\
%I b u\n\
%I cfg u\n\
%I cbg u\n\
%I f u\n\
%I p u\
" << std::endl;
}

void OcIdraw::pict() {
    common_pict();
    out << "%I t u" << std::endl;
}

void OcIdraw::pict(const Transformer& t) {
    common_pict();
    out << "%I t" << std::endl;
    transformer(t);
}

void OcIdraw::end() {
    out << "End %I eop" << std::endl;
}

void OcIdraw::text(Canvas*,
                   const char* s,
                   const Transformer& t,
                   const Font* font,
                   const Color* color) {
    // ZFM tried to allow colors and fonts, but doesn't seem to work
    // 3/12/95
    char buf[100];
    ColorIntensity r = 0, g = 0, b = 0;
    if (color) {
        rgbcolor(color, r, g, b);
    }

    // idraw needs hex
    Sprintf(buf,
            "%%I cfg %x%x%x\n%f %f %f SetCFg\n",
            int(r * 256),
            int(g * 256),
            int(b * 256),
            r,
            g,
            b);
    out << "Begin %I Text\n";
    out << buf;
    if (font) {
        out << "%I f " << font->encoding() << "\n";
        out << font->name() << font->size() << "SetF\n";
    } else {
        out << "\
%I f -*-helvetica-medium-r-normal-*-12-*-*-*-*-*-*-*\n\
Helvetica 12 SetF\n\
";
    };
    out << "%I t" << std::endl;
    Glyph* l = WidgetKit::instance()->label(s);
    Requisition req;
    l->request(req);
    l->unref();
    Requirement& ry = req.y_requirement();
    float y = (1. - ry.alignment()) * ry.natural();
    float x = 0;
    Transformer tr(t);
    tr.translate(x, y);
    transformer(tr);
    out << "%I\n[" << std::endl;
    out << "(" << hidepar(s) << ")" << std::endl;
    out << "] Text\nEnd" << std::endl;
}

void OcIdraw::mline(Canvas*,
                    int count,
                    const Coord* x,
                    const Coord* y,
                    const Color* color,
                    const Brush* b) {
#define cnt_group 200
    int ixd[cnt_group], iyd[cnt_group];
    int i, size;
    float xmax = x[0], xmin = x[0], ymax = y[0], ymin = y[0];
    XYView* v = XYView::current_draw_view();
    xmax = v->right();
    xmin = v->left();
    ymax = v->top();
    ymin = v->bottom();

    float scalex, scaley;
    if (xmax != xmin) {
        scalex = 10000 / (xmax - xmin);
    } else {
        scalex = 1;
    }
    if (ymax != ymin) {
        scaley = 10000 / (ymax - ymin);
    } else {
        scaley = 1;
    }
    Transformer t;
    t.translate(-xmin, -ymin);
    t.scale(scalex, scaley);
    t.invert();
    if (count > cnt_group) {
        pict();
    }
    for (i = 0; i < count;) {
        int ixold, iyold;
        ixold = iyold = -20000;
        for (; i < count; ++i) {
            if (MyMath::inside(x[i], y[i], xmin, ymin, xmax, ymax)) {
                break;
            }
        }
        for (size = 0; i < count;) {
            float x1, y1;
            int ix, iy;
            t.inverse_transform(x[i], y[i], x1, y1);
            if (x1 > 20000.)
                x1 = 20000.;
            if (x1 < -20000.)
                x1 = -20000.;
            if (y1 > 20000.)
                y1 = 20000.;
            if (y1 < -20000.)
                y1 = -20000.;
            ix = int(x1);
            iy = int(y1);
            if (ix != ixold || iy != iyold) {
                ixd[size] = ix;
                iyd[size] = iy;
                ++size;
            }
            if (size >= cnt_group) {
                break;
            }
            ixold = ix;
            iyold = iy;
            ++i;
        }
        if (size < 2) {
            break;
        }
        out << "\nBegin %I MLine\n";
        brush(b);
        ifill(color, false);
        out << "%I t" << std::endl;
        transformer(t);
        out << "%I " << size << std::endl;

        for (int j = 0; j < size; ++j) {
            out << ixd[j] << " " << iyd[j] << std::endl;
        }

        out << size << " MLine\n%I 1\nEnd" << std::endl;
    }

    if (count > cnt_group) {
        end();
    }
}

void OcIdraw::rect(Canvas* c,
                   Coord x1,
                   Coord y1,
                   Coord x2,
                   Coord y2,
                   const Color* color,
                   const Brush* b,
                   bool f) {
    Coord x[4], y[4];
    x[0] = x1;
    y[0] = y1;
    x[1] = x2;
    y[1] = y1;
    x[2] = x2;
    y[2] = y2;
    x[3] = x1;
    y[3] = y2;
    polygon(c, 4, x, y, color, b, f);
}

void OcIdraw::polygon(Canvas*,
                      int count,
                      const Coord* x,
                      const Coord* y,
                      const Color* color,
                      const Brush* b,
                      bool f) {
    char buf[100];
    out << "\nBegin %I Poly\n";
    poly(count, x, y, color, b, f);
    Sprintf(buf, "%d Poly\nEnd", count);
    out << buf << std::endl;
}

void OcIdraw::bspl(Canvas*,
                   int count,
                   const Coord* x,
                   const Coord* y,
                   const Color* color,
                   const Brush* b) {
    char buf[100];
    out << "\nBegin %I BSpl\n";
    poly(count, x, y, color, b, false);
    Sprintf(buf, "%d BSpl\n%%I 1\nEnd", count);
    out << buf << std::endl;
}

void OcIdraw::cbspl(Canvas*,
                    int count,
                    const Coord* x,
                    const Coord* y,
                    const Color* color,
                    const Brush* b,
                    bool f) {
    char buf[100];
    out << "\nBegin %I CBSpl\n";
    poly(count, x, y, color, b, f);
    Sprintf(buf, "%d CBSpl\nEnd", count);
    out << buf << std::endl;
}

void OcIdraw::poly(int count,
                   const Coord* x,
                   const Coord* y,
                   const Color* color,
                   const Brush* b,
                   bool f) {
    brush(b);
    ifill(color, f);
    out << "%I t" << std::endl;


    float x1, x2, y1, y2;
    x1 = MyMath::min(count, x);
    x2 = MyMath::max(count, x);
    y1 = MyMath::min(count, y);
    y2 = MyMath::max(count, y);
    float scalex, scaley;
    if (MyMath::eq(x1, x2, .0001f)) {
        scalex = 1;
    } else {
        scalex = (x2 - x1) / 10000.f;
    }
    if (MyMath::eq(y1, y2, .0001f)) {
        scaley = 1;
    } else {
        scaley = (y2 - y1) / 10000.f;
    }
    Transformer t;
    t.scale(scalex, scaley);
    t.translate(x1, y1);
    transformer(t);
    out << "%I " << count << std::endl;
    char buf[100];
    for (int i = 0; i < count; ++i) {
        float a, b;
        t.inverse_transform(x[i], y[i], a, b);
        Sprintf(buf, "%d %d\n", int(a), int(b));
        out << buf;
    }
}

void OcIdraw::line(Canvas*,
                   Coord x1,
                   Coord y1,
                   Coord x2,
                   Coord y2,
                   const Color* color,
                   const Brush* b) {
    out << "\nBegin %I Line\n";
    brush(b);
    ifill(color, false);
    out << "%I t" << std::endl;
    float scalex, scaley;
    if (MyMath::eq(x1, x2, .0001f)) {
        scalex = 1;
    } else {
        scalex = (x2 - x1) / 10000;
    }
    if (MyMath::eq(y1, y2, .0001f)) {
        scaley = 1;
    } else {
        scaley = (y2 - y1) / 10000;
    }
    Transformer t;
    t.scale(scalex, scaley);
    t.translate(x1, y1);
    transformer(t);
    out << "%I" << std::endl;
    float a, bb, x, y;
    t.inverse_transform(x1, y1, a, bb);
    t.inverse_transform(x2, y2, x, y);
    out << int(a) << " " << int(bb) << " " << int(x) << " " << int(y);

    out << " Line\n%I 1\nEnd" << std::endl;
}

void OcIdraw::ellipse(Canvas*,
                      Coord x1,
                      Coord y1,
                      Coord width,
                      Coord height,
                      const Color* color,
                      const Brush* b,
                      bool f) {
    out << "\nBegin %I Elli\n";
    brush(b);
    ifill(color, f);
    out << "%I t" << std::endl;

    float y = y1;
    float x = x1;
    Transformer tr;
    tr.scale(.01, .01);
    tr.translate(x, y);
    transformer(tr);
    char buf[100];
    Sprintf(buf, "%%I\n0 0 %d %d Elli\nEnd", int(width * 100), int(height * 100));
    out << buf << std::endl;
}

void OcIdraw::brush(const Brush* b) {
    char buf[100];
    Coord w = b ? b->width() : 0;
    int i, p;

    p = 0;
    if (b)
        for (i = 0; i < b->dash_count(); ++i) {
            int nbit = b->dash_list(i);
            for (int j = 0; j < nbit; ++j) {
                p = ((p << 1) | ((i + 1) % 2));
            }
        }
    Sprintf(buf, "%%I b %d\n%d 0 0 [", p, int(w));
    out << buf;
    if (b)
        for (i = 0; i < b->dash_count(); ++i) {
            out << b->dash_list(i) << " ";
        }
    Sprintf(buf, "] 0 SetB");
    out << buf << std::endl;
}

void OcIdraw::ifill(const Color* color, bool f) {
    char buf[100];
    ColorIntensity r = 0, g = 0, b = 0;
    if (color) {
        rgbcolor(color, r, g, b);
    }
    //	Sprintf(buf, "%%I cfg %s\n%d %d %d SetCFg", "Black", 0,0,0);
    // idraw needs hex
    Sprintf(
        buf, "%%I cfg %x%x%x\n%f %f %f SetCFg", int(r * 256), int(g * 256), int(b * 256), r, g, b);
    out << buf << std::endl;
    if (f) {
        //		Sprintf(buf, "%%I cbg %s\n%d %d %d SetCBg\n%%I p\n1 SetP",
        //		 "Black", 0,0,0);
        Sprintf(buf,
                "%%I cbg %x%x%x\n%f %f %f SetCBg\n%%I p\n1 SetP",
                int(r * 256),
                int(g * 256),
                int(b * 256),
                r,
                g,
                b);
    } else {
        Sprintf(buf, "%%I cbg %s\n%d %d %d SetCBg\nnone SetP %%I p n", "White", 1, 1, 1);
    }
    out << buf << std::endl;
}

// only implemented for continuous lines, polygons, and curves.
// i.e. only one move_to at beginning and cannot mix line_to and curve_to

void OcIdraw::new_path() {
    curved_ = false;
    closed_ = false;
    ipath_ = 0;
}

void OcIdraw::move_to(Coord x, Coord y) {
    add(x, y);
}

void OcIdraw::line_to(Coord x, Coord y) {
    add(x, y);
}

void OcIdraw::curve_to(Coord x, Coord y, Coord x1, Coord y1, Coord x2, Coord y2) {
    curved_ = true;
    rcurve(0, x, y, x1, y1, x2, y2);  // first arg is recursion level
}

void OcIdraw::rcurve(int r, Coord x, Coord y, Coord x1, Coord y1, Coord x2, Coord y2) {
    if (r < 2) {
        // split into two bezier
        Coord m01x = (xpath_[ipath_ - 1] + x1) / 2;
        Coord m01y = (ypath_[ipath_ - 1] + y1) / 2;
        Coord m12x = (x1 + x2) / 2;
        Coord m12y = (y1 + y2) / 2;
        Coord m23x = (x2 + x) / 2;
        Coord m23y = (y2 + y) / 2;
        Coord ax = (m01x + m12x) / 2;
        Coord ay = (m01y + m12y) / 2;
        Coord bx = (m12x + m23x) / 2;
        Coord by = (m12y + m23y) / 2;
        Coord cx = (ax + bx) / 2;
        Coord cy = (ay + by) / 2;
        rcurve(r + 1, cx, cy, m01x, m01y, ax, ay);
        rcurve(r + 1, x, y, bx, by, m23x, m23y);
    } else {
        add((x1 + x2) / 2, (y1 + y2) / 2);
        add(x, y);
    }
}

void OcIdraw::close_path() {
    closed_ = true;
    if (curved_) {
        curve_to(
            xpath_[0], ypath_[0], xpath_[ipath_ - 1], ypath_[ipath_ - 1], xpath_[0], ypath_[0]);
    }
}

void OcIdraw::stroke(Canvas* can, const Color* c, const Brush* b) {
    if (closed_) {
        if (curved_) {
            cbspl(can, ipath_, xpath_, ypath_, c, b, false);
        } else {
            polygon(can, ipath_, xpath_, ypath_, c, b, false);
        }
    } else {
        if (curved_) {
            bspl(can, ipath_, xpath_, ypath_, c, b);
        } else {
            mline(can, ipath_, xpath_, ypath_, c, b);
        }
    }
}

void OcIdraw::fill(Canvas* can, const Color* c) {
    if (curved_) {
        cbspl(can, ipath_, xpath_, ypath_, c, NULL, true);
    } else {
        polygon(can, ipath_, xpath_, ypath_, c, NULL, true);
    }
}

void OcIdraw::add(Coord a, Coord b) {
    if (ipath_ >= capacity_) {
        capacity_ *= 2;
        Coord* x = new Coord[capacity_];
        Coord* y = new Coord[capacity_];
        for (int i = 0; i < ipath_; ++i) {
            x[i] = xpath_[i];
            y[i] = ypath_[i];
        }
        delete[] xpath_;
        delete[] ypath_;
        xpath_ = x;
        ypath_ = y;
    }
    xpath_[ipath_] = a;
    ypath_[ipath_] = b;
    ++ipath_;
}

#endif
