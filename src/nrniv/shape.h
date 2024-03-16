#ifndef shape_h
#define shape_h

#include "scenevie.h"
#include "graph.h"

#undef Rubberband

struct Section;  // neuron section structure
class ShapeSection;
class SectionHandler;
class StandardPicker;
class MyRect;
class Color;
class Brush;
class SectionList;
class Rubberband;
class ColorValue;
class PolyGlyph;
struct Symbol;
class Rotation3d;
class Rotate3Band;
class ShapeChangeObserver;

class ShapeScene: public Graph {  // entire neuron
  public:
    enum { SECTION = Graph::EXTRAGRAPHTOOL, ROTATE, EXTRASHAPETOOL };
    ShapeScene(SectionList* = NULL);
    virtual ~ShapeScene();
    virtual void erase_all();
    virtual void observe(SectionList* = NULL);
    virtual void flush();
    virtual ShapeSection* selected();
    virtual float arc_selected();
    virtual void selected(ShapeSection*, Coord x = fil, Coord y = fil);
    virtual float nearest(Coord, Coord);  // and sets selected
    virtual void wholeplot(Coord& x1, Coord& y1, Coord& x2, Coord& y2) const;
    static ShapeScene* current_pick_scene();
    static ShapeScene* current_draw_scene();
    void color(Section* sec1, Section* sec2, const Color*);
    void color(Section* sec, const Color*);
    void colorseg(Section*, double, const Color*);
    void color(const Color*);
    void color(SectionList*, const Color*);
    ColorValue* color_value();
    virtual void view(Coord);
    virtual void view(Coord*);
    virtual void view(Rubberband*);
    enum { show_diam, show_centroid, show_schematic };
    void shape_type(int);
    int shape_type() {
        return shape_type_;
    }
    virtual void section_handler(SectionHandler*);
    virtual SectionHandler* section_handler();
    virtual SectionHandler* section_handler(ShapeSection*);
    PolyGlyph* shape_section_list();
    virtual void transform3d(Rubberband* rb = NULL);
    virtual ShapeSection* shape_section(Section*);
    virtual void name(const char*);
    virtual void save_phase2(std::ostream&);
    virtual void help();
    void force();
    bool view_all() {
        return view_all_;
    }
    void rotate();  // identity
    void rotate(Coord xorg,
                Coord yorg,
                Coord zorg,
                float xrad,
                float yrad,
                float zrad);  // relative
  private:
    bool view_all_;
    ShapeSection* selected_;
    Coord x_sel_, y_sel_;
    ColorValue* color_value_;
    int shape_type_;
    SectionHandler* section_handler_;
    PolyGlyph* sg_;
    Rotate3Band* r3b_;
    std::string var_name_;
    ShapeChangeObserver* shape_changed_;
};

class FastShape: public Glyph {
  public:
    FastShape();
    virtual ~FastShape();
    virtual void fast_draw(Canvas*, Coord x, Coord y, bool) const = 0;
};

class FastGraphItem: public GraphItem {
  public:
    FastGraphItem(FastShape* g, bool save = true, bool pick = true);
    virtual bool is_fast() {
        return true;
    }
};

class ShapeSection: public FastShape {  // single section
  public:
    ShapeSection(Section*);
    virtual ~ShapeSection();
    virtual void request(Requisition&) const;
    virtual void allocate(Canvas*, const Allocation&, Extension&);
    virtual void draw(Canvas*, const Allocation&) const;
    virtual void fast_draw(Canvas*, Coord x, Coord y, bool) const;
    virtual void pick(Canvas*, const Allocation&, int depth, Hit&);
    virtual void setColor(const Color*, ShapeScene*);
    virtual void setColorseg(const Color*, double, ShapeScene*);
    const Color* color() {
        return color_;
    }
    virtual void set_range_variable(Symbol*);
    virtual void clear_variable();
    virtual void selectMenu();
    virtual bool near_section(Coord, Coord, Coord mineps) const;
    float how_near(Coord, Coord) const;
    float arc_position(Coord, Coord) const;
    int get_coord(double arc, Coord&, Coord&) const;
    Section* section() const;
    bool good() const;
    virtual void damage(ShapeScene*);
    //	virtual void update(Observable*);
    virtual void draw_seg(Canvas*, const Color*, int iseg) const;
    virtual void draw_points(Canvas*, const Color*, int, int) const;
    virtual void transform3d(Rotation3d*);
    virtual void size(Coord& l, Coord& b, Coord& r, Coord& t) const;
    void scale(Coord x) {
        len_scale_ = x;
    }
    Coord scale() {
        return len_scale_;
    }

  private:
    void trapezoid(Canvas*, const Color*, int i) const;
    void trapezoid(Canvas*, const Color*, float, float, float, float, float, float) const;
    void loc(double, Coord&, Coord&);
    void bevel_join(Canvas*, const Color*, int, float) const;
#define FASTIDIOUS 1
#if FASTIDIOUS
    void fastidious_draw(Canvas*, const Color*, int, float, float) const;
#endif
  private:
    std::vector<neuron::container::data_handle<double>> pvar_;
    Section* sec_;
    Coord len_scale_;
    const Color* color_;
    std::vector<Color const*> old_;
    const Color** colorseg_;
    int colorseg_size_;  // so know when to unref colorseg_ items.
    Coord xmin_, xmax_, ymin_, ymax_;
    Coord *x_, *y_;
    int n_;
};

#if 1
class ShapeView: public View {
  public:
    ShapeView(ShapeScene*);
    ShapeView(ShapeScene*, Coord*);
    virtual ~ShapeView();
};
#endif


class SectionHandler: public Handler {
  public:
    SectionHandler();
    virtual ~SectionHandler();
    virtual bool event(Event&);
    void shape_section(ShapeSection*);
    ShapeSection* shape_section();

  private:
    ShapeSection* ss_;
};

#endif
