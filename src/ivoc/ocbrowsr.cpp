#include <../../nrnconf.h>
#if HAVE_IV // to end of file

#include <InterViews/layout.h>
#include <InterViews/target.h>
#include <InterViews/adjust.h>
#include <IV-look/choice.h>
#include <IV-look/kit.h>
#include <OS/string.h>
#include "ocbrowsr.h"
#include <stdio.h>
#include "apwindow.h"

//--------------------------------------------------------------------
/*static*/ class OcBrowserAccept : public Action {
public:
	OcBrowserAccept(OcBrowser*);
	virtual ~OcBrowserAccept();
	virtual void execute();
private:
	OcBrowser* sb_;
};
OcBrowserAccept::OcBrowserAccept(OcBrowser* sb){sb_ = sb;}
OcBrowserAccept::~OcBrowserAccept(){}
void OcBrowserAccept::execute(){sb_->accept();}

//---------------------------------------------------------
OcBrowser::OcBrowser(Action* accept, Action* cancel)
   : FileBrowser(WidgetKit::instance(), accept, cancel)
{}

OcBrowser::OcBrowser()
   : FileBrowser(WidgetKit::instance(), new OcBrowserAccept(this) ,NULL)
{}

OcBrowser::~OcBrowser(){}

Glyph* OcBrowser::standard_glyph() {
	LayoutKit& lk = *LayoutKit::instance();
	WidgetKit& wk = *WidgetKit::instance();

	return	lk.hbox(
		   lk.vcenter(
//			wk.inset_frame(
			    lk.margin(
			     	lk.natural_span(this, 100, 100),
			     	5
//			    )
			),
			1.0
		   ),
		   lk.hspace(4),
		   wk.vscroll_bar(this->adjustable())
		);
}
void OcBrowser::accept(){};

void OcBrowser::append_item(const char* cname) {
	WidgetKit& kit = *WidgetKit::instance();
	LayoutKit& layout = *LayoutKit::instance();
	Glyph* name = kit.label(cname);
	Glyph* label = new Target(
	   layout.h_margin(name, 3.0, 0.0, 0.0, 15.0, fil, 0.0),
	   TargetPrimitiveHit
	);
	TelltaleState* t = new TelltaleState(TelltaleState::is_enabled);
	append_selectable(t);
	append(new ChoiceItem(t, label, kit.bright_inset_frame(label)));
}

void OcBrowser::change_item(GlyphIndex i, const char* cname) {
	WidgetKit& kit = *WidgetKit::instance();
	LayoutKit& layout = *LayoutKit::instance();
	Glyph* name = kit.label(cname);
	Glyph* label = new Target(
	   layout.h_margin(name, 3.0, 0.0, 0.0, 15.0, fil, 0.0),
	   TargetPrimitiveHit
	);
	TelltaleState* t = new TelltaleState(TelltaleState::is_enabled);
	replace_selectable(i, t);
	replace(i, new ChoiceItem(t, label, kit.bright_inset_frame(label)));
	refresh();
}

void OcBrowser::select_and_adjust(GlyphIndex i){
	FileBrowser::select(i);
#if 0
	if (i >= 0) {
		GlyphIndex j = (i > 3) ? i+3 : i;
		Adjustable* a = adjustable();
		a->scroll_to(Dimension_Y, Coord(count() - j));
	}
#endif
}

#endif
