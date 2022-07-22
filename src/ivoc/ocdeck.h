#ifndef ocdeck_h
#define ocdeck_h

#include "ocglyph.h"
#include <ivstream.h>

class OcDeckImpl;
struct Object;

class OcDeck: public OcGlyphContainer {
  public:
    OcDeck();
    virtual ~OcDeck();

    virtual void box_append(OcGlyph*);
    virtual void save(std::ostream&);
    virtual void save_action(const char*, Object*);
    virtual void flip_to(int);
    virtual void remove_last();
    virtual void remove(long);
    virtual void move_last(int);  // make last item the i'th item
  private:
    OcDeckImpl* bi_;
};

#endif
