#ifndef axis_h
#define axis_h

#include <InterViews/observe.h>
#include <InterViews/glyph.h>

class Scene;

class Axis : public Glyph, public Observer {
public:
	Axis(Scene*, DimensionName);
	Axis(Scene*, DimensionName, Coord x1, Coord x2);
	Axis(Scene*, DimensionName, Coord x1, Coord x2, Coord pos,
		int ntic = 1, int nminor = 0,
		int invert=0, bool number=true);
	virtual ~Axis();
	virtual void save(ostream&);
	virtual void update(Observable*);
	virtual void size(float&,float&);
private:
	void init(Coord x1, Coord x2, Coord pos=0.,
		int ntic = 1, int nminor = 0,
		int invert=0, bool number=true);
	bool set_range();
	void install();
	void location();
private:
	Scene* s_;
	Coord min_, max_;
	DimensionName d_;
	double amin_, amax_;
	int ntic_, nminor_;
	int invert_;
	bool number_;
	Coord pos_;
};

class BoxBackground : public Background {
public:
	BoxBackground();
	virtual ~BoxBackground();
	
	virtual void draw(Canvas*, const Allocation&) const;
	virtual void print(Printer*, const Allocation&) const;
private:
	void draw_help(Canvas*, const Allocation&) const;
	void tic_label(Coord x, Coord y, Coord val,
		float x_align, float y_align, Canvas*) const;
};

class AxisBackground : public Background {
public:
	AxisBackground();
	virtual ~AxisBackground();
	
	virtual void draw(Canvas*, const Allocation&) const;
	virtual void print(Printer*, const Allocation&) const;
private:
	void draw_help(Canvas*, const Allocation&) const;
	void tic_label(Coord x, Coord y, Coord val,
		float x_align, float y_align, Canvas*) const;
};

#endif
