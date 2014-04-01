#ifndef shapeplot_h
#define shapeplot_h

#include "shape.h"

struct Symbol;
class ShapePlotImpl;
class SectionList;

class ShapePlot : public ShapeScene {
public:
	enum {TIME=ShapeScene::EXTRASHAPETOOL, SPACE, SHAPE};
	ShapePlot(Symbol* = NULL, SectionList* = NULL);
	virtual ~ShapePlot();
	virtual void observe(SectionList* = NULL);
	virtual void erase_all();
	virtual void draw(Canvas*, const Allocation&) const;
	void variable(Symbol*);
	const char* varname()const;
	virtual void scale(float min, float max);
	virtual void save_phase1(ostream&);

	virtual void shape_plot();

	virtual void make_time_plot(Section*, float x);
	virtual void make_space_plot(Section* s1, float x1, Section* s2, float x2);
	virtual void flush();
	virtual void fast_flush();
	void update_ptrs();
private:
	ShapePlotImpl* spi_;
};

class ColorValue : public Resource, public Observable{
public:
	ColorValue();
	virtual ~ColorValue();
	void set_scale(float low, float high);
	const Color* get_color(float) const;
	const Color* no_value() const;
	float low() { return low_;}
	float high() { return high_;}
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
