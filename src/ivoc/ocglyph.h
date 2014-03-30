#ifndef ocglyph_h
#define ocglyph_h

#include <InterViews/monoglyph.h>
#include <ivstream.h>
class PrintableWindow;
class Dialog;

/* 
	A glyph that can be saved and restored using oc classes.
	Can be mapped in its own window or be part of a tray.
*/

class OcGlyph : public MonoGlyph {
public:
	OcGlyph(Glyph* body=NULL);
	virtual ~OcGlyph();

	virtual void save(ostream&);
	virtual bool has_window();
	virtual PrintableWindow* window();
	virtual void window(PrintableWindow*);
	virtual PrintableWindow*  make_window(Coord left = -1, Coord bottom = -1,
		Coord width = -1, Coord height = -1);

	virtual void no_parents();
	void parents(bool);

	virtual bool dialog(const char* label,
		const char* accept, const char* cancel);
	bool dialog_dismiss(bool b);
	virtual void def_size(Coord& w, Coord& h)const;
	int session_priority() { return session_priority_; }
	void session_priority(int i) { session_priority_ = i; }
private:
	PrintableWindow* w_;	
	int parents_;
	Coord def_w_, def_h_;
	Dialog* d_;
	int session_priority_;
};

class OcGlyphContainer : public OcGlyph {
public:
	OcGlyphContainer();
	virtual void intercept(bool);
	virtual void box_append(OcGlyph*) = 0;
	virtual void request(Requisition&) const;
private:
	OcGlyphContainer* parent_;
	bool recurse_;
};

#endif
