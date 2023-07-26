#ifndef rubberband_h
#define rubberband_h

#include <InterViews/handler.h>
#include <InterViews/event.h>
#include <InterViews/transformer.h>

#undef Rubberband
#undef RubberLine
#undef RubberRect

class RubberAction;
class Rubberband;
class Color;
class Brush;
class Canvas;
class Printer;

// called on rubberband release event
class RubberAction: public Resource {
  protected:
    RubberAction();
    virtual ~RubberAction();

  public:
    virtual void execute(Rubberband*);
    virtual void help();
};

class OcHandler: public Handler {
  public:
    OcHandler();
    virtual ~OcHandler();
    virtual void help();
};

class Rubberband: public OcHandler {
  public:
    Rubberband(RubberAction* = NULL, Canvas* = NULL);
    virtual ~Rubberband();
    virtual bool event(Event&);
    Coord x_begin() const, y_begin() const, x() const, y() const;  // canvas coords
    static const Color* color();
    static const Brush* brush();
    void canvas(Canvas*);
    Canvas* canvas() const {
        return canvas_;
    }
    const Transformer& transformer() const {
        return t_;
    }
    const Event& event() const {
        return *e_;
    }
    virtual void help();
    virtual void snapshot(Printer*);
    static Rubberband* current() {
        return current_;
    }

  protected:
    // subclasses manipulate the rubberband glyph
    virtual void draw(Coord x, Coord y);
    virtual void undraw(Coord x, Coord y);

    virtual void press(Event&);
    virtual void drag(Event&);
    virtual void release(Event&);

    void rubber_on(Canvas*);
    void rubber_off(Canvas*);

  private:
    Canvas* canvas_;
    Transformer t_;
    Event* e_;
    RubberAction* ra_;
    Coord x_begin_, y_begin_, x_, y_;
    static const Color* xor_color_;
    static const Brush* brush_;
    static Rubberband* current_;
};

class RubberRect: public Rubberband {
  public:
    RubberRect(RubberAction* = NULL, Canvas* = NULL);
    virtual ~RubberRect();

    virtual void draw(Coord, Coord);

    virtual void get_rect(Coord& x1, Coord& y1, Coord& x2, Coord& y2) const;
    virtual void get_rect_canvas(Coord& x1, Coord& y1, Coord& x2, Coord& y2) const;
    virtual void help();
};

class RubberLine: public Rubberband {
  public:
    RubberLine(RubberAction* = NULL, Canvas* = NULL);
    virtual ~RubberLine();

    virtual void draw(Coord, Coord);

    virtual void get_line(Coord& x1, Coord& y1, Coord& x2, Coord& y2) const;
    virtual void get_line_canvas(Coord& x1, Coord& y1, Coord& x2, Coord& y2) const;
    virtual void help();
};

inline Coord Rubberband::x() const {
    return x_;
}
inline Coord Rubberband::y() const {
    return y_;
}
inline Coord Rubberband::x_begin() const {
    return x_begin_;
}
inline Coord Rubberband::y_begin() const {
    return y_begin_;
}

/*
 * RubberAction  denoted by an object and member function to call on the object.
 * Used the FieldEditorAction as a template
 */

#if defined(__STDC__) || defined(__ANSI_CPP__) || defined(WIN32)
#define __RubberCallback(T)       T##_RubberCallback
#define RubberCallback(T)         __RubberCallback(T)
#define __RubberMemberFunction(T) T##_RubberMemberFunction
#define RubberMemberFunction(T)   __RubberMemberFunction(T)
#else
#define __RubberCallback(T)       T /**/ _RubberCallback
#define RubberCallback(T)         __RubberCallback(T)
#define __RubberMemberFunction(T) T /**/ _RubberMemberFunction
#define RubberMemberFunction(T)   __RubberMemberFunction(T)
#endif

#define declareRubberCallback(T)                             \
    typedef void (T::*RubberMemberFunction(T))(Rubberband*); \
    class RubberCallback(T)                                  \
        : public RubberAction {                              \
      public:                                                \
        RubberCallback(T)(T*, RubberMemberFunction(T));      \
        virtual ~RubberCallback(T)();                        \
                                                             \
        virtual void execute(Rubberband*);                   \
                                                             \
      private:                                               \
        T* obj_;                                             \
        RubberMemberFunction(T) func_;                       \
    };

#define implementRubberCallback(T)                                                \
    RubberCallback(T)::RubberCallback(T)(T * obj, RubberMemberFunction(T) func) { \
        obj_ = obj;                                                               \
        func_ = func;                                                             \
    }                                                                             \
                                                                                  \
    RubberCallback(T)::~RubberCallback(T)() {}                                    \
                                                                                  \
    void RubberCallback(T)::execute(Rubberband* rb) {                             \
        (obj_->*func_)(rb);                                                       \
    }

#endif
