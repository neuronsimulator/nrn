#ifndef rect_h
#define rect_h

#undef Rect
#define Rect nrn_Rect

class Requisition;
class Canvas;
class Allocation;
class Extension;
class Hit;
class Brush;
class Color;

class Appear: public Glyph {
  protected:
    Appear(const Color* color = NULL, const Brush* brush = NULL);

  public:
    virtual ~Appear();
    const Color* color() const {
        return color_;
    }
    void color(const Color*);
    const Brush* brush() const {
        return brush_;
    }
    void brush(const Brush*);
    static const Color* default_color();
    static const Brush* default_brush();

  private:
    const Color* color_;
    const Brush* brush_;
    static const Color* dc_;
    static const Brush* db_;
};

class Rect: public Appear {
  public:
    Rect(Coord left,
         Coord bottom,
         Coord width,
         Coord height,
         const Color* c = NULL,
         const Brush* b = NULL);
    virtual void request(Requisition&) const;
    virtual void allocate(Canvas*, const Allocation&, Extension&);
    virtual void draw(Canvas*, const Allocation&) const;
    virtual void pick(Canvas*, const Allocation&, int depth, Hit&);

    Coord left() const, right() const, top() const, bottom() const;
    Coord width() const, height() const;
    void left(Coord), bottom(Coord);
    void width(Coord), height(Coord);

  private:
    Coord l_, b_, w_, h_;
};

class Line: public Appear {
  public:
    Line(Coord dx, Coord dy, const Color* color = NULL, const Brush* brush = NULL);
    Line(Coord dx,
         Coord dy,
         float x_align,
         float y_align,
         const Color* color = NULL,
         const Brush* brush = NULL);
    virtual ~Line();

    virtual void request(Requisition&) const;
    virtual void allocate(Canvas*, const Allocation&, Extension&);
    virtual void draw(Canvas*, const Allocation&) const;
    virtual void pick(Canvas*, const Allocation&, int depth, Hit&);

  private:
    Coord dx_, dy_;
    float x_, y_;
};

class Circle: public Appear {
  public:
    Circle(float radius, bool filled = false, const Color* color = NULL, const Brush* brush = NULL);
    virtual ~Circle();

    virtual void request(Requisition&) const;
    virtual void allocate(Canvas*, const Allocation&, Extension&);
    virtual void draw(Canvas*, const Allocation&) const;

  private:
    float radius_;
    bool filled_;
};

class Triangle: public Appear {
  public:
    Triangle(float side, bool filled = false, const Color* color = NULL, const Brush* brush = NULL);
    virtual ~Triangle();

    virtual void request(Requisition&) const;
    virtual void allocate(Canvas*, const Allocation&, Extension&);
    virtual void draw(Canvas*, const Allocation&) const;

  private:
    float side_;
    bool filled_;
};

class Rectangle: public Appear {
  public:
    Rectangle(float height,
              float width,
              bool filled = false,
              const Color* color = NULL,
              const Brush* brush = NULL);
    virtual ~Rectangle();

    virtual void request(Requisition&) const;
    virtual void allocate(Canvas*, const Allocation&, Extension&);
    virtual void draw(Canvas*, const Allocation&) const;

  private:
    float height_;
    float width_;
    bool filled_;
};

inline Coord Rect::left() const {
    return l_;
}
inline Coord Rect::right() const {
    return l_ + w_;
}
inline Coord Rect::bottom() const {
    return b_;
}
inline Coord Rect::top() const {
    return b_ + h_;
}
inline Coord Rect::width() const {
    return w_;
}
inline Coord Rect::height() const {
    return h_;
}

inline void Rect::left(Coord x) {
    l_ = x;
}
inline void Rect::bottom(Coord x) {
    b_ = x;
}
inline void Rect::width(Coord x) {
    w_ = (x > 0) ? x : 1.;
}
inline void Rect::height(Coord x) {
    h_ = (x > 0) ? x : 1.;
}

#endif
