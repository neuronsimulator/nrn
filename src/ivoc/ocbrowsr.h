#ifndef ocbrowser_h
#define ocbrowser_h

#include <IV-look/fbrowser.h>

class OcBrowser : public FileBrowser {
public:
	OcBrowser(Action* accept, Action* cancel);
	OcBrowser();
	virtual ~OcBrowser();
	virtual Glyph* standard_glyph();
	virtual void append_item(const char*);
	virtual void change_item(GlyphIndex, const char*);
	virtual void accept();
	virtual void select_and_adjust(GlyphIndex);
};

#endif
