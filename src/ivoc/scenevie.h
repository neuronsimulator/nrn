#ifndef sceneview_h
#define sceneview_h

/*
 A universe where data is placed is a scene. There can be 0 or many
  views into a scene. Each view has one canvas.
        canvas	canvas  canvas
            . . .
    view	view	view	view	view
            scene
  glyph  glyph  glyph  glyph  glyph  glyph  glyph

 A scene is very similar to a page in that we have
  arbitrary placement of glyphs. However, the protocol differs in that
  the user should call Scene::modified(GlyphIndex) whenever a glyph's allocation
  needs to be recomputed or its request will change. If you wish merely to
  redraw the glyph with Scene's stored allocation
  one can call Scene::damage(GlyphIndex). Notice that all communication
  with Scene is in terms of Scene coordinates. (If the scene coordinates
  are changed after construction, there are two coordinate systems.
  see below)

 Scene maintains a list of views whose canvas will be damaged whenever
  the scene changes. This list is maintained during construction and
  destruction of views.

 Scene maintains a list of glyphs along with their allocations and placement.
 Note that when Scene::modified is called, damage is called on both the
 old (stored by Scene) and new allocation of the glyph.

It is sometimes the case that scene coordinates have inappropriate scales
for some glyphs such as labels on a plot (even though
their placement is still in scene coords).
Here we want the labels to have a fixed appearance on the screen regardless
of the scene scaling. This is done with append_fixed(). Such glyphs are
relative to the scaling of the xyview's parent glyph.
(Therefore the PrintWindowManager
will scale these, relative to the resizing of the window on the
paper icon. See the implementation for how to go further using
view_transform())

It is also sometimes the case that we want items positioned relative to
the view window. This is done with append_viewfixed(). Such glyphs have a
location that is relative to view coordinates. (0,0) is left,bottom and (1,1)
is right,top

Items added to the list are not displayed until they are moved away
from (0,0) or modified() to avoid damageing large portions of the canvas
with the common append/move.
*/

/*
 The static function XYView* XYView::current_pick_view holds the view of
 latest pick. From that a glyph in the scene can obtain current scene.
*/

/*
 The creator of a scene may use a standard method of input handling by
 accessing the method picker() which provides a way of associating actions
 with mouse events.

 By default the right mouse button pops up a menu with zoom, new view,
 and whole scene items. The middle button translates the view. The
 left button defaults to new view. These actions have names which label
 the print window manager when one enters a view. See ocpicker.h for details.
*/

/*
 To put a view into a window with a banner that shows the size in scene
 coordinates, use ViewWindow(XYView*)
*/

#include <InterViews/tformsetter.h>
#include <InterViews/observe.h>
#include "apwindow.h"
#include "ocglyph.h"
#include <vector>

#undef Scene

class Scene;
class SceneInfo;
class XYView;
class XYView_PtrList;
class ScenePicker;
class GLabel;
class GPolyLine;
struct Object;

class OcViewGlyph: public OcGlyph {
  public:
    OcViewGlyph(XYView*);
    virtual ~OcViewGlyph();
    XYView* view() {
        return v_;
    }
    virtual void save(std::ostream&);
    void viewmenu(Glyph*);

  private:
    XYView* v_;
    Glyph* g_;
};

// view into a scene; independent scales in x and y direction.
// ie. as window resized, view remains same (directions magnified separately)
class XYView: public TransformSetter, public Observable {
  public:
    XYView(Scene*, Coord xsize = 200, Coord ysize = 200);
    XYView(Coord x1,
           Coord y1,
           Coord x_span,
           Coord y_span,
           Scene*,
           Coord xsize = 200,
           Coord ysize = 200);
    virtual ~XYView();

    virtual Scene* scene() const;
    virtual Coord left() const, right() const, top() const, bottom() const;
    virtual Coord width() const, height() const;

    virtual void damage(Glyph*, const Allocation&, bool fixed = false, bool viewfixed = false);
    virtual void damage(Coord x1, Coord y1, Coord x2, Coord y2);
    virtual void damage_all();

    /* damage area in model coords, call from draw */
    virtual void damage_area(Coord& x1, Coord& y1, Coord& x2, Coord& y2) const;
    virtual void set_damage_area(Canvas*);

    virtual void request(Requisition&) const;
    virtual void allocate(Canvas*, const Allocation&, Extension&);
    virtual void pick(Canvas*, const Allocation&, int depth, Hit&);
    virtual void undraw();

    Canvas* canvas();
    // transforms canvas from scene to parent glyph coordinates.
    const Transformer& s2o() const {
        return scene2viewparent_;
    }
    void canvas(Canvas*);

    void size(Coord x1, Coord y1, Coord x2, Coord y2);
    void origin(Coord x1, Coord y1);
    void x_span(Coord);
    void y_span(Coord);
    virtual void box_size(Coord x1, Coord y1, Coord x2, Coord y2);

    static XYView* current_pick_view();
    static void current_pick_view(XYView*);
    static XYView* current_draw_view();
    Coord x_pick_epsilon() {
        return x_pick_epsilon_;
    }
    Coord y_pick_epsilon() {
        return y_pick_epsilon_;
    }
    virtual void move_view(Coord dx, Coord dy);  // in screen coords.
    virtual void scale_view(Coord xorg, Coord yorg, float dxscale, float dyscale);  // in screen
                                                                                    // coords.
    virtual XYView* new_view(Coord x1, Coord y1, Coord x2, Coord y2);
    void rebind();  // stop the flicker on scale_view and move_view
    virtual void save(std::ostream&);
    OcViewGlyph* parent() {
        return parent_;
    }
    virtual void printfile(const char*);
    virtual void zout(Coord& x1, Coord& y1, Coord& x2, Coord& y2) const;
    virtual void zin(Coord& x1, Coord& y1, Coord& x2, Coord& y2) const;
    Coord view_margin() const {
        return view_margin_;
    }
    virtual void view_ratio(float xratio, float yratio, Coord& x, Coord& y) const;
    virtual void ratio_view(Coord x, Coord y, float& xratio, float& yratio) const;
    virtual void stroke(Canvas*, const Color*, const Brush*);

  protected:
    virtual void transform(Transformer&, const Allocation&, const Allocation& natural) const;
    void scene2view(const Allocation& parent) const;
    void csize(Coord x0, Coord xsize, Coord y0, Coord ysize) const;  // not really const
  private:
    void init(Coord x1, Coord y1, Coord x_span, Coord y_span, Scene*, Coord xsize, Coord ysize);
    void append_view(Scene*);

  protected:
    Coord x_pick_epsilon_, y_pick_epsilon_;

  private:
    Coord x1_, y1_, x_span_, y_span_;
    Canvas* canvas_;
    Transformer scene2viewparent_;
    Coord xsize_, ysize_, xsize_orig_, ysize_orig_, xc0_, yc0_;
    friend class OcViewGlyph;
    OcViewGlyph* parent_;
    Coord xd1_, xd2_, yd1_, yd2_;
    static Coord view_margin_;
};

// view into a scene; scale in x & y direction is the same and determined
//  by span in smallest window dimension. Coords are scene coordinates.

class View: public XYView {
  public:
    View(Scene*);  // view of entire scene
    View(Coord x, Coord y, Coord span, Scene*, Coord xsize = 200, Coord ysize = 200);  // x,y is
                                                                                       // center of
                                                                                       // view
    View(Coord left,
         Coord bottom,
         Coord x_span,
         Coord y_span,
         Scene*,
         Coord xsize = 200,
         Coord ysize = 200);
    virtual ~View();

    virtual Coord x() const, y() const;
    virtual Coord view_width() const, view_height() const;

    void origin(Coord x, Coord y);  // center
    virtual void box_size(Coord x1, Coord y1, Coord x2, Coord y2);
    virtual void move_view(Coord dx, Coord dy);  // in screen coords.
    virtual void scale_view(Coord xorg, Coord yorg, float dxscale, float dyscale);  // in screen
                                                                                    // coords.
    virtual XYView* new_view(Coord x1, Coord y1, Coord x2, Coord y2);

  protected:
    virtual void transform(Transformer&, const Allocation&, const Allocation& natural) const;

  private:
    Coord x_span_, y_span_;
};

class Scene: public Glyph, public Observable {
  public:
    Scene(Coord x1, Coord y1, Coord x2, Coord y2, Glyph* background = NULL);
    virtual ~Scene();
    virtual void background(Glyph* bg = NULL);
    virtual Coord x1() const, y1() const, x2() const, y2() const;
    virtual void new_size(Coord x1, Coord y1, Coord x2, Coord y2);
    virtual void wholeplot(Coord& x1, Coord& y1, Coord& x2, Coord& y2) const;
    virtual int view_count() const;
    virtual XYView* sceneview(int) const;
    virtual void dismiss();  // dismiss windows that contain only this scene
    virtual void printfile(const char*);

    virtual void modified(GlyphIndex);
    void move(GlyphIndex, Coord x, Coord y);
    void location(GlyphIndex, Coord& x, Coord& y) const;
    void show(GlyphIndex, bool);
    bool showing(GlyphIndex) const;

    virtual void damage(GlyphIndex);
    virtual void damage(Coord x1, Coord y1, Coord x2, Coord y2);
    virtual void damage_all();

    enum { NOTOOL = 0, MOVE, DELETE, CHANGECOLOR, EXTRATOOL };
    virtual void tool(int);
    virtual int tool();
    virtual void help();
    virtual void delete_label(GLabel*);
    virtual void change_label_color(GLabel*);
    virtual void change_line_color(GPolyLine*);
    virtual void request(Requisition&) const;
    virtual void allocate(Canvas*, const Allocation&, Extension&);
    virtual void draw(Canvas*, const Allocation&) const;
    virtual void print(Printer*, const Allocation&) const;
    virtual void pick(Canvas*, const Allocation&, int depth, Hit&);

    virtual void append(Glyph*);
    virtual void append_fixed(Glyph*);
    virtual void append_viewfixed(Glyph*);
    virtual void prepend(Glyph*);
    virtual void insert(GlyphIndex, Glyph*);
    virtual void remove(GlyphIndex);
    virtual void replace(GlyphIndex, Glyph*);
    virtual void change(GlyphIndex);
    virtual void change_to_fixed(GlyphIndex, XYView*);
    virtual void change_to_vfixed(GlyphIndex, XYView*);

    virtual GlyphIndex count() const;
    virtual Glyph* component(GlyphIndex) const;
    virtual void allotment(GlyphIndex, DimensionName, Allotment&) const;
    virtual GlyphIndex glyph_index(const Glyph*);
    bool drawing_fixed_item() const {
        return drawing_fixed_item_;
    }

    static void save_all(std::ostream&);
    static long scene_list_index(Scene*);
    bool mark() {
        return mark_;
    }
    void mark(bool m) {
        mark_ = m;
    }
    virtual void save_phase1(std::ostream&);
    virtual void save_phase2(std::ostream&);
    virtual Coord mbs() const;

    static const Color* default_background();
    static const Color* default_foreground();

    ScenePicker* picker();
    Object* hoc_obj_ptr() {
        return hoc_obj_ptr_;
    }
    void hoc_obj_ptr(Object* o) {
        hoc_obj_ptr_ = o;
    }
    bool menu_picked() {
        return menu_picked_;
    }

  protected:
    virtual void save_class(std::ostream&, const char*);

  private:
#if 1
    friend class XYView;
#else
    // I prefer this but the SGI compiler doesn't like it
    friend void XYView::append_view(Scene*);
    friend XYView::~XYView();
#endif
    virtual void damage(GlyphIndex, const Allocation&);
    void append_view(XYView*);
    void remove_view(XYView*);
    void check_allocation(GlyphIndex);

  private:
    Coord x1_, y1_, x2_, y2_;
    std::vector<SceneInfo>* info_;
    std::vector<XYView*>* views_;
    Glyph* background_;
    ScenePicker* picker_;
    int tool_;
    bool mark_;
    static Scene* current_scene_;
    static Coord mbs_;  // menu_box_size (pixels) in left top
    bool drawing_fixed_item_;
    Object* hoc_obj_ptr_;
    bool menu_picked_;

    Coord x1_orig_, x2_orig_, y1_orig_, y2_orig_;
};

class ViewWindow: public PrintableWindow, public Observer {
  public:
    ViewWindow(XYView*, const char* name);
    virtual ~ViewWindow();
    virtual void update(Observable*);
    virtual void reconfigured();
};

inline Coord Scene::x1() const {
    return x1_;
}
inline Coord Scene::x2() const {
    return x2_;
}
inline Coord Scene::y1() const {
    return y1_;
}
inline Coord Scene::y2() const {
    return y2_;
}


#endif
