#ifndef shapeplot_h
#define shapeplot_h


#if HAVE_IV
#include "shape.h"
#endif

struct Symbol;
class ShapePlotImpl;
class SectionList;


class ShapePlotInterface {
public:
	virtual void scale(float min, float max) = 0;
	virtual const char* varname() const = 0;
	virtual void variable(Symbol*) = 0;
	virtual float low() = 0;
	virtual float high() = 0;
	virtual Object* neuron_section_list() = 0;
};

class ShapePlotData : public ShapePlotInterface {
public:
	ShapePlotData(Symbol* = NULL, Object* = NULL);
	virtual ~ShapePlotData();
	virtual void scale(float min, float max);
	virtual const char* varname() const;
	virtual void variable(Symbol*);
	virtual float low();
	virtual float high();
	virtual Object* neuron_section_list();

private:
	Symbol* sym_;
	float lo, hi;
	Object* sl_;
};

#if HAVE_IV
class ShapePlot : public ShapeScene, public ShapePlotInterface {
public:
	enum {TIME=ShapeScene::EXTRASHAPETOOL, SPACE, SHAPE};
	ShapePlot(Symbol* = NULL, SectionList* = NULL);
	virtual ~ShapePlot();
	virtual void observe(SectionList* = NULL);
	virtual void erase_all();
	virtual void draw(Canvas*, const Allocation&) const;
	virtual void variable(Symbol*);
	virtual const char* varname()const;
	virtual void scale(float min, float max);
	virtual void save_phase1(ostream&);

	virtual void shape_plot();

	virtual void make_time_plot(Section*, float x);
	virtual void make_space_plot(Section* s1, float x1, Section* s2, float x2);
	virtual void flush();
	virtual void fast_flush();
	virtual float low();
	virtual float high();
	virtual Object* neuron_section_list();
	void update_ptrs();
private:
	ShapePlotImpl* spi_;
	Object* sl_;

};

class ColorValue : public Resource, public Observable{
public:
	ColorValue();
	virtual ~ColorValue();
	void set_scale(float low, float high);
	const Color* get_color(float) const;
	const Color* no_value() const;
	float low() const { return low_;}
	float high() const { return high_;}
	Glyph* make_glyph();
	void colormap(int size, bool global = false);
	void colormap(int index, int red, int green, int blue);
private:
	float low_, high_;
	int csize_;
	const Color** crange_;
};

class Hinton : public Observer, public FastShape {
public:
	Hinton(double*, Coord xsize, Coord ysize, ShapeScene*);
	virtual ~Hinton();
	virtual void request(Requisition&) const;
	virtual void allocate(Canvas*, const Allocation&, Extension&);
	virtual void draw(Canvas*, const Allocation&) const;
	virtual void fast_draw(Canvas*, Coord x, Coord y, bool) const;
	virtual void update(Observable*);
private:
	double* pd_;
	const Color* old_;
	Coord xsize_, ysize_;
	ShapeScene* ss_;
};
#endif
#endif
