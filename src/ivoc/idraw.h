#ifndef ocidraw_h
#define ocidraw_h
/*
Hooks for special processing make use of the request() method in Glyphs.
*/
#include <ivstream.h>

class Canvas;
class Transformer;
class Font;
class Color;
class Brush;

class OcIdraw {
  public:
    static void prologue();
    static void epilog();
    static void pict();
    static void pict(const Transformer&);
    static void end();
    static void text(Canvas*,
                     const char*,
                     const Transformer&,
                     const Font* f = NULL,
                     const Color* c = NULL);
    static void mline(Canvas*,
                      int count,
                      const Coord* x,
                      const Coord* y,
                      const Color* c = NULL,
                      const Brush* b = NULL);
    static void polygon(Canvas*,
                        int count,
                        const Coord* x,
                        const Coord* y,
                        const Color* c = NULL,
                        const Brush* b = NULL,
                        bool fill = false);
    static void rect(Canvas*,
                     Coord x1,
                     Coord y1,
                     Coord x2,
                     Coord y2,
                     const Color* c = NULL,
                     const Brush* b = NULL,
                     bool fill = false);
    static void line(Canvas*,
                     Coord x1,
                     Coord y1,
                     Coord x2,
                     Coord y2,
                     const Color* c = NULL,
                     const Brush* b = NULL);
    static void ellipse(Canvas*,
                        Coord x1,
                        Coord y1,
                        Coord width,
                        Coord height,
                        const Color* c = NULL,
                        const Brush* b = NULL,
                        bool fill = false);

    static void new_path();
    static void move_to(Coord x, Coord y);
    static void line_to(Coord x, Coord y);
    static void curve_to(Coord x, Coord y, Coord x1, Coord y1, Coord x2, Coord y2);
    static void close_path();
    static void stroke(Canvas*, const Color*, const Brush*);
    static void fill(Canvas*, const Color*);
    static void bspl(Canvas*,
                     int count,
                     const Coord* x,
                     const Coord* y,
                     const Color* c = NULL,
                     const Brush* b = NULL);
    static void cbspl(Canvas*,
                      int count,
                      const Coord* x,
                      const Coord* y,
                      const Color* c = NULL,
                      const Brush* b = NULL,
                      bool fill = false);

  public:
    static std::ostream* idraw_stream;

  private:
    static void rcurve(int level, Coord x, Coord y, Coord x1, Coord y1, Coord x2, Coord y2);
    static void poly(int count,
                     const Coord* x,
                     const Coord* y,
                     const Color* c = NULL,
                     const Brush* b = NULL,
                     bool fill = false);
    static void add(Coord, Coord);
    static void brush(const Brush*);
    static void ifill(const Color*, bool);
    static bool closed_;
    static bool curved_;
    static Coord *xpath_, *ypath_;
    static int ipath_, capacity_;
};

#define IfIdraw(arg)             \
    if (OcIdraw::idraw_stream) { \
        OcIdraw::arg;            \
    }


#endif
