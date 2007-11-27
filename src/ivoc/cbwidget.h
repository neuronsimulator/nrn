#ifndef cbwidget_h
#define cbwidget_h

#include <InterViews/action.h>
class Graph;

class ColorBrushWidget : public Action , public Observer {
public:
	static void start(Graph*);
	virtual ~ColorBrushWidget();
	void execute();
	virtual void update(Observable*);
private:
	ColorBrushWidget(Graph*);
	void map();
private:
	Graph* g_;
	PolyGlyph* cb_;
	PolyGlyph* bb_;
	DismissableWindow* w_;
};

#endif
