#ifndef cbwidget_h
#define cbwidget_h

#include <InterViews/action.h>
class Graph;
class Scene;

class ColorBrushWidget: public Action {
  public:
    static void start(Graph*);
    virtual ~ColorBrushWidget();
    void execute();
    void update(Scene*);

  private:
    ColorBrushWidget(Graph*);
    void map();

  private:
    std::size_t slotId{};
    Graph* g_;
    PolyGlyph* cb_;
    PolyGlyph* bb_;
    DismissableWindow* w_;
};

#endif
