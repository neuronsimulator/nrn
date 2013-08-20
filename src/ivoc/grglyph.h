#ifndef grglyph_h
#define grglyph_h

#include <InterViews/transformer.h>
#include "graph.h"

struct Object;
class GrGlyph;

class GrGlyphItem : public GraphItem {
public:
	GrGlyphItem(Glyph* g, float scalex, float scaley, float rot);
	virtual ~GrGlyphItem();
	virtual void allocate(Canvas*, const Allocation&, Extension&);
	virtual void draw(Canvas*, const Allocation&) const;
	virtual void print(Printer*, const Allocation&) const;
private:
	Transformer t_;
};

class GrGlyph : public Glyph {
public:
	GrGlyph(Object*);
	virtual ~GrGlyph();

	virtual void request(Requisition&) const;
	virtual void draw(Canvas*, const Allocation&) const;
	
	void new_path();
	void move_to(Coord, Coord);
	void line_to(Coord, Coord);
	void control_point(Coord, Coord);
	void curve_to(Coord, Coord, Coord, Coord, Coord, Coord);
	void close_path();
	void circle(Coord x, Coord y, Coord r);
	void stroke(int color, int brush);
	void fill(int color);
	void erase();
	void gif(const char*);

	Object** temp_objvar();
private:
	DataVec* type_;
	DataVec* x_;
	DataVec* y_;
	Object* obj_;
	Glyph* gif_;
};

#endif
