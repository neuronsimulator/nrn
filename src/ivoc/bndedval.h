#ifndef bounded_value_h
#define bounded_value_h

#include <InterViews/adjust.h>

class BoundedValue : public Adjustable {
protected:
    BoundedValue();
public:
    BoundedValue(Coord lower, Coord upper);
    virtual ~BoundedValue();

    virtual void lower_bound(Coord);
    virtual void upper_bound(Coord);
    virtual void current_value(Coord);
    virtual void scroll_incr(Coord);
    virtual void page_incr(Coord);

    virtual Coord lower(DimensionName) const;
    virtual Coord upper(DimensionName) const;
    virtual Coord length(DimensionName) const;
    virtual Coord cur_lower(DimensionName) const;
    virtual Coord cur_upper(DimensionName) const;
    virtual Coord cur_length(DimensionName) const;

    virtual void scroll_to(DimensionName, Coord position);
    virtual void scroll_forward(DimensionName);
    virtual void scroll_backward(DimensionName);
    virtual void page_forward(DimensionName);
    virtual void page_backward(DimensionName);
private:
    Coord curvalue_;
    Coord lower_;
    Coord span_;
    Coord scroll_incr_;
    Coord page_incr_;
};


#endif
