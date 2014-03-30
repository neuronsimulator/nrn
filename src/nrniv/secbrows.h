#ifndef secbrowser_h
#define secbrowser_h

#include <InterViews/monoglyph.h>
#include "ocbrowsr.h"

class TelltaleState;
struct Object;
class SectionBrowserImpl;
class MechSelector;
class OcList;
class PPBImpl;
class HocCommand;
struct Section;

extern void section_menu(double, int, MechSelector* = NULL);

class MechVarType : public MonoGlyph {
public:
	MechVarType();
	virtual ~MechVarType();
	bool parameter_select();
	bool assigned_select();
	bool state_select();
private:
	bool select(int);
private:
	TelltaleState* tts_[3];
};

class MechSelector : public MonoGlyph {
public:
	MechSelector();
	virtual ~MechSelector();

	bool is_selected(int type);
	int begin();
	bool done();
	int next();
private:
	TelltaleState** tts_;
	int iterator_;
};

class OcSectionBrowser : public OcBrowser {
public:
	OcSectionBrowser(Object*);
	virtual ~OcSectionBrowser();
	virtual void accept();
	virtual void select_section(Section*);
	virtual void set_select_action(const char*);
	virtual void set_accept_action(const char*);
	virtual void select(GlyphIndex);
private:
	Section** psec_;
	int scnt_;
	HocCommand* select_;
	HocCommand* accept_;
};
class SectionBrowser : public OcBrowser {
public:
	SectionBrowser();
	SectionBrowser(Object*);
	virtual ~SectionBrowser();
	static void make_section_browser();
	void accept();

	virtual void select(GlyphIndex);
private:
	SectionBrowserImpl* sbi_;
};

class PointProcessBrowser : public OcBrowser {
public:
	PointProcessBrowser(OcList*);
	virtual ~PointProcessBrowser();
	static void make_point_process_browser(OcList*);
	void accept();
	
	virtual void select(GlyphIndex);
	virtual void add_pp(Object*);
	virtual void remove_pp();
	virtual void append_pp(Object*);
private:
	PPBImpl* ppbi_;
};

#endif
