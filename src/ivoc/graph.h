#ifndef graph_h
#define graph_h

#include "neuron/container/data_handle.hpp"
#include <OS/list.h>
#include <OS/string.h>
#include <InterViews/observe.h>
#include "scenevie.h"

class DataVec;
class Color;
class Brush;
struct Symbol;
struct Symlist;
class GraphLine;
class GLabel;
class GPolyLine;
class SymChooser;
class Event;
class GraphVector;
class HocCommand;
class LineExtension;
class TelltaleState;
struct Object;

// all Glyphs added to Graph must be enclosed in a GraphItem
class GraphItem: public MonoGlyph {
  public:
    enum { ERASE_LINE = 1, ERASE_AXIS };
    GraphItem(Glyph* g, bool = true, bool pick = true);
    virtual ~GraphItem();
    virtual void pick(Canvas*, const Allocation&, int depth, Hit&);
    virtual void save(std::ostream&, Coord, Coord);
    virtual void erase(Scene*, GlyphIndex, int erase_type);
    bool save() {
        return save_;
    }
    void save(bool s) {
        save_ = s;
    }
    virtual bool is_polyline();
    virtual bool is_mark();
    virtual bool is_fast() {
        return false;
    }
    virtual bool is_graphVector() {
        return false;
    }

  private:
    bool save_;
    bool pick_;
};

class Graph: public Scene {  // Scene of GraphLines labels and polylines
  public:
    enum { CROSSHAIR = Scene::EXTRATOOL, CHANGELABEL, PICK, EXTRAGRAPHTOOL };
    Graph(bool = true);  // true means map a new default view
    virtual ~Graph();
    void axis(DimensionName,
              float min,
              float max,
              float pos = 0.,
              int ntics = -1,
              int nminor = 0,
              int invert = 0,
              bool number = true);
    GraphLine* add_var(const char*,
                       const Color*,
                       const Brush*,
                       bool usepointer,
                       int fixtype = 1,
                       neuron::container::data_handle<double> p = {},
                       const char* lab = NULL,
                       Object* obj = NULL);
    void x_expr(const char*, bool usepointer);
    void add_polyline(GPolyLine*);
    void add_graphVector(GraphVector*);
    void begin();
    void plot(float);
    void flush();
    void fast_flush();
    void begin_line(const char* s = NULL);
    void begin_line(const Color*, const Brush*, const char* s = NULL);
    void line(Coord x, Coord y);
    void mark(Coord x,
              Coord y,
              char style = '+',
              float size = 12,
              const Color* = NULL,
              const Brush* = NULL);
    void erase();
    virtual void erase_all();
    void erase_lines();  // all GPolylines
    virtual void delete_label(GLabel*);
    virtual bool change_label(GLabel*, const char*, GLabel* gl = NULL);
    virtual void help();
    void keep_lines();
    void keep_lines_toggle();
    void family(bool);
    void family(const char*);
    void family_label_chooser();
    void new_axis();
    void erase_axis();
    void view_axis();
    void view_box();
    void change_prop();
    void color(int);
    void brush(int);
    const Color* color() const {
        return color_;
    }
    const Brush* brush() const {
        return brush_;
    }
    void set_cross_action(const char*, Object*, bool vectorcopy = false);
    void cross_action(char, GPolyLine*, int);
    void cross_action(char, Coord, Coord);
    void simgraph();  // faintly analogous to Vector.record for localstep plotting

    virtual void draw(Canvas*, const Allocation&) const;
    virtual void pick(Canvas*, const Allocation&, int depth, Hit&);
    virtual GlyphIndex glyph_index(const Glyph*);
    virtual void new_size(Coord x1, Coord y1, Coord x2, Coord y2);
    virtual void wholeplot(Coord& x1, Coord& y1, Coord& x2, Coord& y2) const;

    // label info
    GLabel* label(float x,
                  float y,
                  const char* s,
                  int fixtype,
                  float scale,
                  float x_align,
                  float y_align,
                  const Color*);
    GLabel* label(float x, float y, const char* s, float n = 0, int fixtype = -1);
    GLabel* label(const char* s, int fixtype = -1);
    GLabel* new_proto_label() const;
    void fixed(float scale);
    void vfixed(float scale);
    void relative(float scale);
    void align(float x, float y);
    void choose_sym();
    void name(char*);
    void change_label_color(GLabel*);
    void change_line_color(GPolyLine*);

    virtual void save_phase1(std::ostream&);
    virtual void save_phase2(std::ostream&);
    int labeltype() const {
        return label_fixtype_;
    }
    static bool label_chooser(const char*, char*, GLabel*, Coord x = 400., Coord y = 400.);

    virtual void see_range_plot(GraphVector*);
    static void ascii(std::ostream*);
    static std::ostream* ascii();

  private:
    void extension_start();
    void extension_continue();
    void ascii_save(std::ostream& o) const;
    void family_value();

  private:
    Symlist* symlist_;
    std::vector<GraphLine*> line_list_;
    int loc_;
    DataVec* x_;
    bool extension_flushed_;
    SymChooser* sc_;
    static SymChooser* fsc_;
    std::string var_name_;
    GPolyLine* current_polyline_;

    const Color* color_;
    const Brush* brush_;
    int label_fixtype_;
    float label_scale_;
    float label_x_align_, label_y_align_;
    float label_x_, label_y_, label_n_;
    TelltaleState* keep_lines_toggle_;
    bool family_on_;
    GLabel* family_label_;
    double family_val_;
    int family_cnt_;
    HocCommand* cross_action_;
    bool vector_copy_;

    Symbol* x_expr_;
    neuron::container::data_handle<double> x_pval_;

    GraphVector* rvp_;
    static std::ostream* ascii_;
};

class DataVec: public Resource {  // info for single dimension
  public:
    DataVec(int size);
    DataVec(const DataVec*);
    virtual ~DataVec();
    void add(float);
    float max() const, min() const;
    float max(int low, int high), min(int, int);
    int loc_max() const, loc_min() const;
    void erase();
    int count() const {
        return count_;
    }
    void write();
    float get_val(int i) const {
        return y_[i];
    }  // y[(i<count_)?i:count_-1)];
    int size() const {
        return size_;
    }
    const Coord* vec() {
        return y_;
    }
    void running_start();
    float running_max();
    float running_min();
    Object** new_vect(GLabel* g = NULL) const;

  private:
    int count_, size_, iMinLoc_, iMaxLoc_;
    int running_min_loc_, running_max_loc_;
    float* y_;
};

class DataPointers: public Resource {  // vector of pointers
  public:
    virtual ~DataPointers() {}
    void add(neuron::container::data_handle<double> dh) {
        px_.push_back(std::move(dh));
    }
    void erase() {
        px_.clear();
    }
    [[nodiscard]] std::size_t size() {
        return px_.capacity();
    }
    [[nodiscard]] std::size_t count() {
        return px_.size();
    }
    [[nodiscard]] neuron::container::data_handle<double> p(std::size_t i) {
        assert(i < px_.size());
        return px_[i];
    }

  private:
    std::vector<neuron::container::data_handle<double>> px_;
};

class GPolyLine: public Glyph {
  public:
    GPolyLine(DataVec* x, const Color* = NULL, const Brush* = NULL);
    GPolyLine(DataVec* x, DataVec* y, const Color* = NULL, const Brush* = NULL);
    GPolyLine(GPolyLine*);
    virtual ~GPolyLine();

    virtual void request(Requisition&) const;
    virtual void allocate(Canvas*, const Allocation&, Extension&);
    virtual void draw(Canvas*, const Allocation&) const;
    virtual void draw_specific(Canvas*, const Allocation&, int, int) const;
    virtual void print(Printer*, const Allocation&) const;
    virtual void pick(Canvas*, const Allocation&, int depth, Hit&);
    virtual void save(std::ostream&);
    virtual void pick_vector();

    void plot(Coord x, Coord y);
    void erase() {
        y_->erase();
    }
    virtual void erase_line(Scene*, GlyphIndex);  // Erase by menu command

    void color(const Color*);
    void brush(const Brush*);
    const Color* color() const {
        return color_;
    }
    const Brush* brush() const {
        return brush_;
    }

    Coord x(int index) const {
        return x_->get_val(index);
    }
    Coord y(int index) const {
        return y_->get_val(index);
    }
    const DataVec* x_data() const {
        return x_;
    }
    const DataVec* y_data() const {
        return y_;
    }

    GLabel* label() const {
        return glabel_;
    }
    void label(GLabel*);
    void label_loc(Coord& x, Coord& y) const;

    // screen coords
    bool near(Coord, Coord, float, const Transformer&) const;
    // model coords input but checking relative to screen coords
    int nearest(Coord, Coord, const Transformer&, int index = -1) const;
    bool keepable() {
        return keepable_;
    }

  private:
    void init(DataVec*, DataVec*, const Color*, const Brush*);

  protected:
    DataVec* y_;
    DataVec* x_;
    const Color* color_;
    const Brush* brush_;
    GLabel* glabel_;
    bool keepable_;
};

class GraphLine: public GPolyLine, public Observer {  // An oc variable to plot
  public:
    GraphLine(const char*,
              DataVec* x,
              Symlist**,
              const Color* = NULL,
              const Brush* = NULL,
              bool usepointer = 0,
              neuron::container::data_handle<double> pd = {},
              Object* obj = NULL);
    virtual ~GraphLine();

    virtual void pick(Canvas*, const Allocation&, int depth, Hit&);
    virtual void save(std::ostream&);

    void plot();

    const char* name() const;
    LineExtension* extension() {
        return extension_;
    }
    void extension_start();
    void extension_continue();
    const Color* save_color() const {
        return save_color_;
    }
    const Brush* save_brush() const {
        return save_brush_;
    }
    void save_color(const Color*);
    void save_brush(const Brush*);
    bool change_expr(const char*, Symlist**);
    virtual void update(Observable*);
    bool valid(bool check = false);
    virtual void erase_line(Scene*, GlyphIndex) {
        erase();
    }  // Erase by menu command
    void simgraph_activate(bool);
    void simgraph_init();
    void simgraph_continuous(double);

    Symbol* expr_;
    neuron::container::data_handle<double> pval_;
    Object* obj_;

  private:
    LineExtension* extension_;
    const Color* save_color_;
    const Brush* save_brush_;
    bool valid_;
    DataVec* simgraph_x_sav_;
};

class GraphVector: public GPolyLine, public Observer {  // fixed x and vector of pointers
  public:
    GraphVector(const char*, const Color* = NULL, const Brush* = NULL);
    virtual ~GraphVector();
    virtual void request(Requisition&) const;
    void begin();
    void add(float, neuron::container::data_handle<double>);
    virtual void save(std::ostream&);
    const char* name() const;
    bool trivial() const;

    virtual bool choose_sym(Graph*);
    virtual void update(Observable*);
    DataPointers* py_data() {
        return dp_;
    }
    void record_install();
    void record_uninstall();

  private:
    DataPointers* dp_;
    std::string name_;
    bool disconnect_defer_;
};

class GPolyLineItem: public GraphItem {
  public:
    GPolyLineItem(Glyph* g)
        : GraphItem(g) {}
    virtual ~GPolyLineItem(){};
    virtual bool is_polyline();
    virtual void save(std::ostream& o, Coord, Coord) {
        ((GPolyLine*) body())->save(o);
    }
    virtual void erase(Scene* s, GlyphIndex i, int type) {
        if (type & GraphItem::ERASE_LINE) {
            s->remove(i);
        }
    }
};

class GLabel: public Glyph {
  public:
    GLabel(const char* s,
           const Color*,
           int fixtype = 1,
           float size = 12,
           float x_align = 0.,
           float y_align = 0.);
    virtual ~GLabel();
    virtual Glyph* clone() const;

    virtual void request(Requisition&) const;
    virtual void allocate(Canvas*, const Allocation&, Extension&);
    virtual void draw(Canvas*, const Allocation&) const;
    virtual void save(std::ostream&, Coord, Coord);
    virtual void pick(Canvas*, const Allocation&, int depth, Hit&);

    void text(const char*);
    void fixed(float scale);
    void vfixed(float scale);
    void relative(float scale);
    void align(float x, float y);
    void color(const Color*);

    bool fixed() const {
        return fixtype_ == 1;
    }
    float scale() const {
        return scale_;
    }
    const char* text() const {
        return text_.c_str();
    }
    int fixtype() const {
        return fixtype_;
    }
    float x_align() const {
        return x_align_;
    }
    float y_align() const {
        return y_align_;
    }
    const Color* color() const {
        return color_;
    }
    bool erase_flag() {
        return erase_flag_;
    }
    void erase_flag(bool b) {
        erase_flag_ = b;
    }

    GPolyLine* labeled_line() const {
        return gpl_;
    }

  private:
    void need(Canvas*, const Allocation&, Extension&) const;
    friend void GPolyLine::label(GLabel*);

  private:
    int fixtype_;
    float scale_;
    float x_align_, y_align_;
    std::string text_;
    Glyph* label_;
    const Color* color_;
    GPolyLine* gpl_;
    bool erase_flag_;
};

class ColorPalette {
  public:
    ColorPalette();
    virtual ~ColorPalette();
    const Color* color(int) const;
    const Color* color(int, const char*);
    const Color* color(int, const Color*);
    int color(const Color*) const;
    //	enum {COLOR_SIZE = 20};
    // ZFM: changed to allow more colors
    enum { COLOR_SIZE = 100 };

  private:
    const Color* color_palette[COLOR_SIZE];
};
class BrushPalette {
  public:
    BrushPalette();
    virtual ~BrushPalette();
    const Brush* brush(int) const;
    const Brush* brush(int index, int pattern, Coord width);
    int brush(const Brush*) const;
    enum { BRUSH_SIZE = 25 };

  private:
    const Brush* brush_palette[BRUSH_SIZE];
};
extern ColorPalette* colors;
extern BrushPalette* brushes;

#endif
