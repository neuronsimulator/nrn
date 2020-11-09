#include <../../nrnconf.h>
#if HAVE_IV // to end of file

#include <IV-X11/xcanvas.h>
#include <IV-X11/xwindow.h>
#include <IV-X11/xdisplay.h>
#include <InterViews/display.h>
#include <InterViews/session.h>
#include <InterViews/style.h>

#include <stdio.h>
#include <stdlib.h>
#include "rubband.h"
#include "scenevie.h"
#include "apwindow.h"

void get_position(XDisplay* dpy, XWindow window, int* rx, int* ry);

static bool r_unbound;
void Rubberband::rubber_on(Canvas* c) {
        if (c->rep()->copybuffer_ == CanvasRep::unbound) {
                r_unbound = true;
                return;
        }
	c->rep()->unbind();
	c->rep()->bind(0);
}

void Rubberband::rubber_off(Canvas* c) {
        if (r_unbound == true) {
                r_unbound = false;
                return;
        }
	c->rep()->unbind();
	c->rep()->bind(1);
	c->damage_all();
}

void XYView::rebind(){
	canvas()->rep()->unbind();
	canvas()->rep()->bind(1);
}

//-----------------------------------

void PrintableWindow::hide() {
	if (bound()) {
		if (is_mapped()) {
			xplace(xleft(), xtop());
//printf("hide %p %d %d\n", this, xleft_, xtop_);
			WindowRep& w = *((Window*)this)->rep();
			XWithdrawWindow(display()->rep()->display_, w.xwindow_, display()->rep()->screen_);
		}
	}
}

int PrintableWindow::xleft() const {
	int x, y;
	if (bound()) {
		WindowRep& w = *((Window*)this)->rep();
		get_position(display()->rep()->display_, w.xwindow_, &x, &y);
	}else if (xplace_) {
		x = xleft_;
	}else{
		x = 0;
	}
	return x;
}

int PrintableWindow::xtop() const {
	int x, y;
	if (bound()) {
		WindowRep& w = *((Window*)this)->rep();
		get_position(display()->rep()->display_, w.xwindow_, &x, &y);
	}else if (xplace_) {
		y = xtop_;
	}else{
		y = 0;
	}
	return y;
}

void PrintableWindow::xplace(int left, int top) {
	xplace_ = true;
	xleft_ = left;
	xtop_ = top;
}

#if 1
static int xoff = -999, yoff = -999;

void PrintableWindow::xmove(int left1, int top) {
	const Display& d = *display();
	WindowRep& w = *((Window*)this)->rep();
	Style* q = Session::instance()->style();
        Coord WMOffsetX, WMOffsetY;

	if (xoff == -999 && yoff == -999) {
	    if (!q->find_attribute("window_manager_offset_x", WMOffsetX)) {
		WMOffsetX = 5.;
	    }
	    if (!q->find_attribute("window_manager_offset_y", WMOffsetY)) {
		WMOffsetY = 26.;
	    }
	    xoff = (int)WMOffsetX;
	    yoff = (int)WMOffsetY;
	}
xoff = yoff = 0;
//printf("%p xmove(%d,%d) XMoveWindow(%d,%d)\n", this,left1, top, left1+xoff, top+yoff);
	XMoveWindow(d.rep()->display_, w.xwindow_, left1+xoff, top+yoff);
}
#else
static Bool WaitForEvent(XDisplay *dpy, XEvent *event, char *type) {
	return (Bool)(event->type == (int)type);
}

void PrintableWindow::xmove(int left1, int top) {
	const Display& d = *display();
	XEvent event;

	int x = 0, y = 0;
	static int xoff = 0, yoff = 0;
	WindowRep& w = *((Window*)this)->rep();
	get_position(d.rep()->display_, w.xwindow_, &x, &y);

	for (int i= 0; (left1 != x || top != y) && i < 10; i++) {
		XMoveWindow(d.rep()->display_, w.xwindow_,
				left1+xoff, top+yoff);
		XPeekIfEvent(d.rep()->display_, &event, WaitForEvent,
				(char *)ConfigureNotify);
		get_position(d.rep()->display_, w.xwindow_, &x, &y);

		if (left1 != x || top != y) {
			XIfEvent(d.rep()->display_, &event, WaitForEvent,
					(char *)ConfigureNotify);
			xoff = left1 - x;
			yoff = top - y;
		}
printf("x=%d y=%d, left1=%d top=%d xoff = %d yoff = %d\n",
	x, y, left1, top, xoff, yoff);
	}
}
#endif

void PrintableWindow::default_geometry() {
    WindowRep& w = *Window::rep();
    const Display& d = *w.display_;
    w.glyph_->request(w.shape_);
    Coord width = w.shape_.requirement(Dimension_X).natural();
    Coord height = w.shape_.requirement(Dimension_Y).natural();
	((OcGlyph*)glyph())->def_size(width, height);
	
    w.canvas_->size(width, height);
	if (xplace_) {
		w.placed_ = true;
		w.left_ = d.to_coord(xleft_);
		w.bottom_ = d.to_coord(d.pheight() - xtop_ - w.canvas_->pheight());
	}
    w.xpos_ = d.to_pixels(w.left_);
    w.ypos_ = d.pheight() - d.to_pixels(w.bottom_) - w.canvas_->pheight();
    if (w.aligned_) {
	w.xpos_ -= d.to_pixels(w.xalign_ * width);
	w.ypos_ += d.to_pixels(w.yalign_ * height);
    }
    if (w.placed_) {
	Display& d = *w.display_;
	PixelCoord l = w.xpos_, b = w.ypos_;
	PixelCoord pw = d.to_pixels(width), ph = d.to_pixels(height);
	l = (l < d.pwidth() - pw) ? l : d.pwidth() - pw;
	b = (b < d.pheight() - ph) ? b : d.pheight() - ph;
	l = (l > 0) ? l : 0;
	b = (b > 0) ? b : 0;
	w.xpos_ = l;
	w.ypos_ = b;
    }
}

//---------------------------
#if 0
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xos.h>
#include <X11/extensions/shape.h>
#include <X11/Xmu/WinUtil.h>
#include <stdio.h>
#endif

void get_position(XDisplay* dpy, XWindow window, int* rx, int* ry)
{
  XWindowAttributes win_attributes;
  XWindow junkwin;

  if (!XGetWindowAttributes(dpy, window, &win_attributes)) {
      fprintf(stderr, "Can't get window attributes.");
      exit(1);
  } 
  (void) XTranslateCoordinates (dpy, window, win_attributes.root, 
				-win_attributes.x,
				-win_attributes.y,
				rx, ry, &junkwin);

//printf("get_position %p %d %d\n", window, *rx, *ry);
//if (xoff != -999) {
//  *rx -= xoff;
//  *ry -= yoff;
//}
}

#endif
