#ifndef ppshape_h
#define ppshape_h

// shape class for viewing point processes

#include "shape.h"
#include "graph.h"

class PPShapeImpl;
class OcList;
struct Object;

class PointProcessGlyph : public GLabel {
public:
	PointProcessGlyph(Object*);
	virtual ~PointProcessGlyph();
	virtual Object* object() { return ob_;}
private:
	Object* ob_;
};

class PPShape : public ShapeScene {
public:
	PPShape(OcList*);
	virtual ~PPShape();

	virtual void pp_append(Object*);
	virtual void install(Object*);
	virtual void pp_remove(PointProcessGlyph*);
	virtual void pp_move(PointProcessGlyph*);
	virtual void examine(PointProcessGlyph*);
private:
	PPShapeImpl* si_;
};

#endif
