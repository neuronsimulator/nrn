#ifndef rot3band_h
#define rot3band_h

/*
  3-D rubberband
  On the button press a globe appears centered at the mouse location
  Dragging the mouse causes the globe to rotate as though the mouse
  was always at the same location on the globe. If the mouse is dragged
  off the globe, the mouse is treated as though it is on the edge. The mouse
  is always treated as though it is on the front hemisphere.
  
  Well, maybe someday.
  Can type x, y, z to get immediate rotations in which axes are out of screen.
  
  Multiple invocations of the Rotate3Band accumulate rotations.
*/

#include "rubband.h"
#include "rotate3d.h"

class Rotate3Band : public Rubberband {
public:
	Rotate3Band(Rotation3d* = NULL, RubberAction* = NULL, Canvas* = NULL);
	virtual ~Rotate3Band();
	
	virtual void press(Event&);
	virtual void drag(Event&);
	virtual void draw(Coord, Coord);

	virtual bool event(Event&); // looks for x, y, z press
	Rotation3d* rotation();
	virtual void help();
private:
	Rotation3d* rot_;
	float x_old_, y_old_;
};

#endif
