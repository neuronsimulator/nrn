#include <../../nrnconf.h>
#if HAVE_IV // to end of file

#include "bndedval.h"
#include <stdio.h>

BoundedValue::BoundedValue() {
    scroll_incr_ = 0.0;
    page_incr_ = 0.0;
}

BoundedValue::BoundedValue(Coord lower, Coord upper) {
    lower_ = lower;
    span_ = upper - lower;
    scroll_incr_ = span_ * 0.04;
    page_incr_ = span_ * 0.4;
    curvalue_ = (lower + upper) * 0.5;
}

BoundedValue::~BoundedValue() { }

void BoundedValue::lower_bound(Coord c) { lower_ = c; }
void BoundedValue::upper_bound(Coord c) { span_ = c - lower_; }

void BoundedValue::current_value(Coord value) {
    curvalue_ = value;
    constrain(Dimension_X, curvalue_);
    notify(Dimension_X);
    notify(Dimension_Y);
}

void BoundedValue::scroll_incr(Coord c) { scroll_incr_ = c; }
void BoundedValue::page_incr(Coord c) { page_incr_ = c; }

#define access_function(name,value) \
Coord BoundedValue::name(DimensionName) const { \
    return value; \
}

access_function(lower,lower_)
access_function(upper,lower_ + span_)
access_function(length,span_)
access_function(cur_lower,curvalue_)
access_function(cur_upper,curvalue_)
access_function(cur_length,0)

void BoundedValue::scroll_to(DimensionName d, Coord position) {
    Coord p = position;
    constrain(d, p);
    if (p != curvalue_) {
	curvalue_ = p;
	notify(Dimension_X);
	notify(Dimension_Y);
    }
}

#define scroll_function(name,expr) \
void BoundedValue::name(DimensionName d) { \
    scroll_to(d, curvalue_ + expr); \
}

scroll_function(scroll_forward,+scroll_incr_)
scroll_function(scroll_backward,-scroll_incr_)
scroll_function(page_forward,+page_incr_)
scroll_function(page_backward,-page_incr_)

#endif
