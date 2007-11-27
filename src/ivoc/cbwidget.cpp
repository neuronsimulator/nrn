#include <../../nrnconf.h>
#if HAVE_IV // to end of file

#include <InterViews/window.h>
#include <InterViews/event.h>
#include <InterViews/layout.h>
#include <InterViews/background.h>
#include <InterViews/style.h>
#include <IV-look/kit.h>
#include <stdio.h>
#include "apwindow.h"
#include "graph.h"
#include "cbwidget.h"
#include "rect.h"
#include "scenepic.h"

ColorBrushWidget::ColorBrushWidget(Graph* g) {
	g_ = g;
	Resource::ref(g_);
	g->attach(this);
}

ColorBrushWidget::~ColorBrushWidget() {
	//printf("~ColorBrushWidget\n");
	g_->detach(this);
	Resource::unref(g_);
}

void ColorBrushWidget::execute() {
	GlyphIndex i, cnt;
	cnt = cb_->count();
	for (i=0; i < cnt; ++i) {
		Button* b = (Button*)((MonoGlyph*)cb_->component(i))->body();
		if (b->state()->test(TelltaleState::is_chosen)) {
			g_->color(int(i));
			break;
		}
	}
	cnt = bb_->count();
	for (i=0; i < cnt; ++i) {
		Button* b = (Button*)((MonoGlyph*)bb_->component(i))->body();
		if (b->state()->test(TelltaleState::is_chosen)) {
			g_->brush(int(i));
			break;
		}
	}
}

void ColorBrushWidget::map() {
	int i; long colorsize=10, brushsize=10;
	WidgetKit& wk = *WidgetKit::instance();
	LayoutKit& lk = *LayoutKit::instance();
	
	wk.style()->find_attribute("CBWidget_ncolor", colorsize);
	wk.style()->find_attribute("CBWidget_nbrush", brushsize);
	cb_ = lk.vbox(colorsize);
	bb_ = lk.vbox(brushsize);
	Glyph* g = lk.hflexible(lk.hbox(cb_, bb_)) ;
	TelltaleGroup* ttg1 = new TelltaleGroup();
	TelltaleGroup* ttg2 = new TelltaleGroup();
	for (i=0; i < colorsize; ++i) {
		cb_->append(lk.margin(wk.radio_button(ttg1,
		  new Line(50, 0, colors->color(i), brushes->brush(4)),
		  this
		  ), 2)
		 );
	}
	for (i=0; i < brushsize; ++i) {
		bb_->append(lk.margin(wk.radio_button(ttg2,
		  new Line(50, 0, colors->color(1), brushes->brush(i)),
		  this
		  ), 2)
		 );
	}
	
	DismissableWindow* w = new DismissableWindow(new Background(g, wk.background()));
	w_ = w;
	DismissableWindow* p = ScenePicker::last_window();
	if (p) {
		w->transient_for(p);
		w->place(p->left(), p->bottom());
	}
	w->map();
}

void ColorBrushWidget::start(Graph* g) {
	ColorBrushWidget* cb = new ColorBrushWidget(g);
	cb->map();
}

void ColorBrushWidget::update(Observable*) {
	if (g_->tool() != Scene::CHANGECOLOR) {
		//printf("dismiss window of ColorBrushWidget\n");
		w_->dismiss();
	}
}

#endif
