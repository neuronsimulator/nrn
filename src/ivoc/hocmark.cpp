#include <../../nrnconf.h>
#if HAVE_IV // to end of file

#include <stdio.h>
#include <InterViews/color.h>
#include <InterViews/brush.h>
#include "hocmark.h"
#include "oc2iv.h"
#include "rect.h"
#include "idraw.h"

/*static*/ class HocMarkP : public HocMark {
public:
	HocMarkP(char, float, const Color*, const Brush*);
	virtual ~HocMarkP();
};



HocMark::HocMark(char style, float size, const Color* c, const Brush* b)
   : PolyGlyph(2)
{
	style_ = style;
	size_ = size;
	c_ = c;
	Resource::ref(c);
	b_ = b;
	Resource::ref(b);
}

HocMark::~HocMark() {
	Resource::unref(c_);
	Resource::unref(b_);
}

void HocMark::request(Requisition& req) const {
	float size = 1;
	if (b_) {
		size = b_->width();
	}
	Requirement rx(size_ + size, 0, 0, .5);
	Requirement ry(size_ + size, 0, 0, .5);
	req.require_x(rx);
	req.require_y(ry);
}
void HocMark::allocate(Canvas* c, const Allocation& a, Extension& e) {
	e.set(c, a);
}
void HocMark::draw(Canvas* c, const Allocation& a) const {
	IfIdraw(pict());
	for (long i = count() - 1; i >= 0; --i) {
		component(i)->draw(c, a);
	}
	IfIdraw(end());
}

HocMark* HocMark::instance(char style, float size,
   const Color* c, const Brush* b)
{
//printf("HocMark::instance\n");
	HocMark* m = search(style, size, c, b);
	if (!m) {
		switch (style) {
		case 0:
		case '+':
			m = new HocMarkP(style, size, c, b);
			break;
		case 1:
		case 'o':
			m = new HocMark(style, size, c, b);
			m->append(new Circle(size/2, false, c, b));
			break;
		case 2:
		case 's':
			m = new HocMark(style, size, c, b);
			m->append(new Rectangle(size, size, false, c, b));
		        break;
		case 3:
		case 't':
			m = new HocMark(style, size, c, b);
			m->append(new Triangle(size, false, c, b));
		        break;
		case 4:
		case 'O':
			m = new HocMark(style, size, c, b);
			m->append(new Circle(size/2, true, c, b));
			break;
		case 5:
		case 'S':
			m = new HocMark(style, size, c, b);
			m->append(new Rectangle(size, size, true, c, b));
		        break;
		case 6:
		case 'T':
			m = new HocMark(style, size, c, b);
			m->append(new Triangle(size, true, c, b));
			break;
		case 7:
		case '|':
			m = new HocMark(style, size, c, b);
			m->append(new Line(0, size, .5, .5, c, b));
			break;
		case 8:
		case '-':
			m = new HocMark(style, size, c, b);
			m->append(new Line(size, 0, .5, .5, c, b));
			break;
		default :
			hoc_execerror("implemented styles are + o t s O T S | -; waiting on x *", 0);
		}
		add(m);
	}
	return m;
}

void HocMark::add(HocMark* m) {
	if (!mark_list_) {
		mark_list_ = new PolyGlyph();
	}
	mark_list_->append(m);
	most_recent_ = m;
}

HocMark* HocMark::search(char style, float size,
   const Color* c, const Brush* b)
{
	HocMark* m;
	if (!most_recent_) {
		return NULL;
	}
	m = check(style, size, c, b);
	if (m) {
		return m;
	}
	for (long i = mark_list_->count() - 1; i >= 0; --i) {
		most_recent_ = (HocMark*)mark_list_->component(i);
		m = check(style, size, c, b);
		if (m) {
			return m;
		}
	}
	return NULL;
}

HocMark* HocMark::check(char style, float size,
   const Color* c, const Brush* b)
{
	if (
	     most_recent_->style_ == style
	  && most_recent_->size_ == size
	  && most_recent_->c_ == c
	  && most_recent_->b_ == b
	) {
		return most_recent_;
	}
	return NULL;
}

HocMark* HocMark::most_recent_;
PolyGlyph* HocMark::mark_list_;


HocMarkP::HocMarkP(char style, float size, const Color* c, const Brush* b)
    : HocMark(style, size, c, b)
{
//printf("new mark +\n");
	append(new Line(size, 0, .5, .5, c, b));
	append(new Line(0, size, .5, .5, c, b));
}
HocMarkP::~HocMarkP(){};
#endif
