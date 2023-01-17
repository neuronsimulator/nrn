#ifndef hocmark_h
#define hocmark_h

#undef check

#include <InterViews/polyglyph.h>

class Color;
class Brush;

class HocMark: public PolyGlyph {
  public:
    virtual ~HocMark();
    static HocMark* instance(char style, float size, const Color*, const Brush*);

    virtual void request(Requisition&) const;
    virtual void allocate(Canvas*, const Allocation&, Extension&);
    virtual void draw(Canvas*, const Allocation&) const;
    //	virtual void print(Printer*, const Allocation&) const;
    virtual void pick(Canvas*, const Allocation&, int depth, Hit&);
    //	virtual void save(std::ostream&);
  protected:
    HocMark(char style, float size, const Color*, const Brush*);

  protected:
    float size_;
    const Color* c_;
    const Brush* b_;
    char style_;

  private:
    static void add(HocMark*);
    static HocMark* search(char style, float size, const Color*, const Brush*);
    static HocMark* check(char style, float size, const Color*, const Brush*);

  private:
    static PolyGlyph* mark_list_;
    static HocMark* most_recent_;
};

#endif
