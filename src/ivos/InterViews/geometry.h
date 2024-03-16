/*
 * Copyright (c) 1987, 1988, 1989, 1990, 1991 Stanford University
 * Copyright (c) 1991 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and 
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Stanford and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Stanford and Silicon Graphics.
 * 
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
 *
 * IN NO EVENT SHALL STANFORD OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
 * OF THIS SOFTWARE.
 */

#ifndef iv_geometry_h
#define iv_geometry_h

#include <InterViews/coord.h>

#include <InterViews/_enter.h>

/*
 * Can't make DimensionName an enum because we want to be able
 * to iterate from 0 to number_of_dimensions.  Alas,
 * C++ does not allow arithmetic on enums.
 */
typedef unsigned int DimensionName;

enum {
    Dimension_X = 0, Dimension_Y, Dimension_Z, Dimension_Undefined
};

class CoordinateSpace {
public:
    enum { x = 0, y, z, dimensions };
#ifdef _DELTA_EXTENSIONS
#pragma __static_class
#endif
};

class Requirement {
public:
    Requirement();
    Requirement(Coord natural);
    Requirement(Coord natural, Coord stretch, Coord shrink, float alignment);
    Requirement(
        Coord natural_lead, Coord max_lead, Coord min_lead,
        Coord natural_trail, Coord max_trail, Coord min_trail
    );

    bool equals(const Requirement&, float epsilon) const;
    bool defined() const;

    void natural(Coord);
    Coord natural() const;

    void stretch(Coord);
    Coord stretch() const;

    void shrink(Coord);
    Coord shrink() const;

    void alignment(float);
    float alignment() const;
private:
    Coord natural_;
    Coord stretch_;
    Coord shrink_;
    float alignment_;
#ifdef _DELTA_EXTENSIONS
#pragma __static_class
#endif
};

class Requisition {
public:
    Requisition();
    Requisition(const Requisition&);

    void penalty(int);
    int penalty() const;

    bool equals(const Requisition&, float epsilon) const;
    void require(DimensionName, const Requirement&);
    void require_x(const Requirement&);
    void require_y(const Requirement&);
    const Requirement& requirement(DimensionName) const;
    const Requirement& x_requirement() const;
    const Requirement& y_requirement() const;
    Requirement& requirement(DimensionName);
    Requirement& x_requirement();
    Requirement& y_requirement();
private:
    int penalty_;
    Requirement x_;
    Requirement y_;
#ifdef _DELTA_EXTENSIONS
#pragma __static_class
#endif
};

class Allotment {
public:
    Allotment();
    Allotment(Coord origin, Coord span, float alignment);

    bool equals(const Allotment&, float epsilon) const;

    void origin(Coord);
    void offset(Coord);
    Coord origin() const;
    void span(Coord);
    Coord span() const;

    void alignment(float);
    float alignment() const;

    Coord begin() const;
    Coord end() const;
private:
    Coord origin_;
    Coord span_;
    float alignment_;
#ifdef _DELTA_EXTENSIONS
#pragma __static_class
#endif
};

class Allocation {
public:
    Allocation();
    Allocation(const Allocation&);

    bool equals(const Allocation&, float epsilon) const;

    void allot(DimensionName, const Allotment&);
    void allot_x(const Allotment&);
    void allot_y(const Allotment&);
    Allotment& allotment(DimensionName);
    const Allotment& allotment(DimensionName) const;
    Allotment& x_allotment();
    Allotment& y_allotment();
    const Allotment& x_allotment() const;
    const Allotment& y_allotment() const;

    Coord x() const;
    Coord y() const;
    Coord left() const;
    Coord right() const;
    Coord bottom() const;
    Coord top() const;
private:
    Allotment x_;
    Allotment y_;
#ifdef _DELTA_EXTENSIONS
#pragma __static_class
#endif
};

class Canvas;

class Extension {
public:
    Extension();
    Extension(const Extension&);

    void operator =(const Extension&);

    static void transform_xy(
	Canvas*, Coord& left, Coord& bottom, Coord& right, Coord& top
    );

    void set(Canvas*, const Allocation&);
    void set_xy(Canvas*, Coord left, Coord bottom, Coord right, Coord top);
    void clear();

    void merge(const Extension&);
    void merge(Canvas*, const Allocation&);
    void merge_xy(Canvas*, Coord left, Coord bottom, Coord right, Coord top);

    Coord left() const;
    Coord bottom() const;
    Coord right() const;
    Coord top() const;
private:
    Coord x_begin_;
    Coord x_end_;
    Coord y_begin_;
    Coord y_end_;
#ifdef _DELTA_EXTENSIONS
#pragma __static_class
#endif
};

inline Requirement::Requirement() {
    natural_ = -fil;
    stretch_ = 0;
    shrink_ = 0;
    alignment_ = 0;
}

inline Requirement::Requirement(Coord natural) {
    natural_ = natural;
    stretch_ = 0;
    shrink_ = 0;
    alignment_ = 0;
}

inline Requirement::Requirement(
    Coord natural, Coord stretch, Coord shrink, float alignment
) {
    natural_ = natural;
    stretch_ = stretch;
    shrink_ = shrink;
    alignment_ = alignment;
}

inline bool Requirement::defined() const {
    return natural_ != -fil;
}

inline void Requirement::natural(Coord c) { natural_ = c; }
inline Coord Requirement::natural() const { return natural_; }
inline void Requirement::stretch(Coord c) { stretch_ = c; }
inline Coord Requirement::stretch() const { return stretch_; }
inline void Requirement::shrink(Coord c) { shrink_ = c; }
inline Coord Requirement::shrink() const { return shrink_; }
inline void Requirement::alignment(float a) { alignment_ = a; }
inline float Requirement::alignment() const { return alignment_; }

inline int Requisition::penalty() const { return penalty_; }
inline void Requisition::penalty(int penalty) { penalty_ = penalty; }

inline void Requisition::require_x(const Requirement& r) { x_ = r; }
inline void Requisition::require_y(const Requirement& r) { y_ = r; }
inline const Requirement& Requisition::x_requirement() const { return x_; }
inline const Requirement& Requisition::y_requirement() const { return y_; }
inline Requirement& Requisition::x_requirement() { return x_; }
inline Requirement& Requisition::y_requirement() { return y_; }

inline Allotment::Allotment() {
    origin_ = 0;
    span_ = 0;
    alignment_ = 0;
}

inline Allotment::Allotment(Coord origin, Coord span, float alignment) {
    origin_ = origin;
    span_ = span;
    alignment_ = alignment;
}

inline void Allotment::origin(Coord o) { origin_ = o; }
inline void Allotment::offset(Coord o) { origin_ += o; }
inline Coord Allotment::origin() const { return origin_; }
inline void Allotment::span(Coord c) { span_ = c; }
inline Coord Allotment::span() const { return span_; }
inline void Allotment::alignment(float a) { alignment_ = a; }
inline float Allotment::alignment() const { return alignment_; }

inline Coord Allotment::begin() const {
    return origin_ - Coord(alignment_ * span_);
}

inline Coord Allotment::end() const {
    return origin_ - Coord(alignment_ * span_) + span_;
}

inline void Allocation::allot_x(const Allotment& a) { x_ = a; }
inline void Allocation::allot_y(const Allotment& a) { y_ = a; }

inline Allotment& Allocation::x_allotment() { return x_; }
inline Allotment& Allocation::y_allotment() { return y_; }
inline const Allotment& Allocation::x_allotment() const { return x_; }
inline const Allotment& Allocation::y_allotment() const { return y_; }

inline Coord Allocation::x() const { return x_.origin(); }
inline Coord Allocation::y() const { return y_.origin(); }
inline Coord Allocation::left() const { return x_.begin(); }
inline Coord Allocation::right() const { return x_.end(); }
inline Coord Allocation::bottom() const { return y_.begin(); }
inline Coord Allocation::top() const { return y_.end(); }

inline Coord Extension::left() const { return x_begin_; }
inline Coord Extension::bottom() const { return y_begin_; }
inline Coord Extension::right() const { return x_end_; }
inline Coord Extension::top() const { return y_end_; }

#include <InterViews/_leave.h>

#endif
