#ifndef ocbox_h
#define ocbox_h

#include "ocglyph.h"
#include <ivstream.h>

class OcBoxImpl;
class BoxAdjust;
struct Object;

class OcBox : public OcGlyphContainer {
public:
	enum {H, V};
        enum {INSET, OUTSET, BRIGHT_INSET, FLAT};
        OcBox(int type, int frame = INSET, boolean scroll = false);
	virtual ~OcBox();
	
	virtual void box_append(OcGlyph*);
	virtual void save(ostream&);
	virtual void save_action(const char*, Object*);
	virtual void adjuster(Coord natural);
	virtual void adjust(Coord natural, int);
	virtual void adjust(Coord natural, BoxAdjust*);
	boolean full_request();
	void full_request(boolean);

	virtual void premap();
	virtual void dismiss_action(const char*, Object* pyact=nil);
	virtual void no_parents();
	void keep_ref(Object*);
	Object* keep_ref();

	boolean dismissing();
	void dismissing(boolean);
private:
	OcBoxImpl* bi_;
};

#endif
