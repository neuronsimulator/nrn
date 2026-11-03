#include <../../nrnconf.h>
#if HAVE_IV  // to end of file

#include <IV-look/kit.h>
#include <InterViews/layout.h>
#include <InterViews/telltale.h>
#include <InterViews/window.h>
#include <OS/list.h>
#include <OS/string.h>
#include <stdio.h>
#include "ivoc.h"
#include "mymath.h"
#include "scenevie.h"
#include "scenepic.h"
#include "rubband.h"
#include "apwindow.h"
#include "utility.h"
#include "oc2iv.h"
#include "utils/enumerate.h"

#define Scene_SceneMover_     "Translate Scene"
#define Scene_SceneZoom_      "ZoomInOut Scene"
#define Scene_RoundView_      "RoundView Scene"
#define Scene_WholeSceneView_ "WholeScene Scene"
#define Scene_WholePlotView_  "View_equal_Plot Scene"
#define Scene_ZoomOut10_      "ZoomOut10 Scene"
#define Scene_ZoomeIn10_      "ZoomIn10 Scene"
#define Scene_SpecView_       "SetView Scene"
#define Scene_SizeScene_      "Scene_equal_View Scene"
#define Scene_NewView_        "NewView Scene"
#define Scene_ShowMenu_       "Scene"
#define Scene_ObjectName_     "ObjectName"

MenuItem* K::menu_item(const char* name) {
    Glyph* g =
        LayoutKit::instance()->r_margin(WidgetKit::instance()->fancy_label(name), 0.0, fil, 0.0);
    return WidgetKit::instance()->menu_item(g);
}

MenuItem* K::check_menu_item(const char* name) {
    Glyph* g =
        LayoutKit::instance()->r_margin(WidgetKit::instance()->fancy_label(name), 0.0, fil, 0.0);
    return WidgetKit::instance()->check_menu_item(g);
}

MenuItem* K::radio_menu_item(TelltaleGroup* tg, const char* name) {
    Glyph* g =
        LayoutKit::instance()->r_margin(WidgetKit::instance()->fancy_label(name), 0.0, fil, 0.0);
    return WidgetKit::instance()->radio_menu_item(tg, g);
}

/*static*/ class ButtonItemInfo {
  public:
    ButtonItemInfo(const char* name, Action*, TelltaleState*, MenuItem* mi = NULL, Menu* m = NULL);
    virtual ~ButtonItemInfo();
    GlyphIndex menu_index();
    TelltaleState* s_;
    CopyString name_;
    Action* a_;
    Menu* parent_;
    MenuItem* mi_;
};
ButtonItemInfo::ButtonItemInfo(const char* name,
                               Action* a,
                               TelltaleState* s,
                               MenuItem* mi,
                               Menu* parent) {
    name_ = name;
    s_ = s;
    a_ = a;
    parent_ = parent;
    mi_ = mi;
}
ButtonItemInfo::~ButtonItemInfo() {}

GlyphIndex ButtonItemInfo::menu_index() {
    GlyphIndex i, cnt;
    if (parent_ && mi_) {
        cnt = parent_->item_count();
        for (i = 0; i < cnt; ++i) {
            if (parent_->item(i) == mi_) {
                return i;
            }
        }
    }
    return -1;
}

/*static*/ class SceneMover: public OcHandler {
  public:
    SceneMover();
    virtual ~SceneMover();
    virtual bool event(Event&);
    virtual void help();

  private:
    Coord x_, y_;
    XYView* view_;
};

/*static*/ class SceneZoom: public OcHandler {
  public:
    SceneZoom();
    virtual ~SceneZoom();
    virtual bool event(Event&);
    virtual void help();

  private:
    Coord x_, y_, xorg_, yorg_;
    XYView* view_;
};

/*static*/ class NewView: public RubberAction {
  public:
    NewView();
    virtual ~NewView();
    virtual void execute(Rubberband*);
    virtual void help();
};

/*static*/ class RoundView: public Action { virtual void execute(); };

/*static*/ class WholeSceneView: public Action { virtual void execute(); };

/*static*/ class WholePlotView: public Action { virtual void execute(); };

/*static*/ class SpecView: public Action { virtual void execute(); };

/*static*/ class SizeScene: public Action { virtual void execute(); };

/*static*/ class ZoomOut10: public Action { virtual void execute(); };

/*static*/ class ZoomIn10: public Action { virtual void execute(); };

/*static*/ class SPObjectName: public Action { virtual void execute(); };

/*static*/ class ShowMenu: public Action {
  public:
    ShowMenu(Menu*);
    virtual ~ShowMenu();
    virtual void execute();

  private:
    Menu* m_;
};

ScenePicker* Scene::picker() {
    if (!picker_) {
        WidgetKit& k = *WidgetKit::instance();
        picker_ = new ScenePicker(this);

        SceneZoom* z = new SceneZoom();
        SceneMover* m = new SceneMover();

        Menu* men = k.pulldown();
        MenuItem* mi = K::menu_item("View . . .");
        mi->menu(men);
        picker_->add_menu(mi);

        picker_->add_menu("View = plot", new WholePlotView(), men);
        picker_->add_menu("Set View", new SpecView(), men);
        picker_->add_menu("10% Zoom out", new ZoomOut10(), men);
        picker_->add_menu("10% Zoom in", new ZoomIn10(), men);
        picker_->add_radio_menu("NewView", new RubberRect(new NewView()), 0, 0, men);
        picker_->add_radio_menu("Zoom in/out", z, 0, men);
        picker_->add_radio_menu("Translate", m, 0, men);
        picker_->add_menu("Round View", new RoundView(), men);
        picker_->add_menu("Whole Scene", new WholeSceneView(), men);
        picker_->add_menu("Scene=View", new SizeScene(), men);
        picker_->add_menu("Object Name", new SPObjectName(), men);
        //		picker_->add_menu("Show Menu", new ShowMenu(men), men);
        //		picker_->add_menu(k.menu_item("Undo"));
        //		picker_->add_menu(k.menu_item("Redo"));
        picker_->add_menu(k.menu_item_separator());
        picker_->bind_select(z);
        picker_->bind_adjust(m);
    }
    return picker_;
}

/*static*/ class ScenePickerImpl: public OcHandler {
  public:
    ScenePickerImpl(Scene*);
    virtual ~ScenePickerImpl();
    virtual bool event(Event&);
    long info_index(const char*);

  public:
    friend class ScenePicker;
    PopupMenu* menu_;
    Scene* scene_;
    TelltaleGroup* tg_;
    CopyString sel_name_;
    std::vector<ButtonItemInfo*>* bil_;
    static DismissableWindow* window_;
};

void ScenePicker::pick_menu(Glyph* glyph, int depth, Hit& h) {
    h.target(depth, glyph, 0, spi_);
}

DismissableWindow* ScenePickerImpl::window_;

/*static*/ class RadioSelect: public Action {
  public:
    RadioSelect(const char*, Action*, Scene*);
    virtual ~RadioSelect();
    virtual void execute();

  private:
    Action* a_;
    CopyString name_;
    Scene* s_;
};

/*static*/ class RubberTool: public Action {
  public:
    RubberTool(Action* sel, Rubberband*, ScenePicker*, int);
    virtual ~RubberTool();
    virtual void execute();

  private:
    Action* sel_;
    Rubberband* rb_;
    ScenePicker* sp_;
    int tool_;
};

/*static*/ class HandlerTool: public Action {
  public:
    HandlerTool(OcHandler*, ScenePicker*, int);
    virtual ~HandlerTool();
    virtual void execute();

  private:
    OcHandler* h_;
    ScenePicker* sp_;
    int tool_;
};

ScenePicker::ScenePicker(Scene* scene) {
    spi_ = new ScenePickerImpl(scene);
    spi_->ref();
    bind_menu(spi_);
}

DismissableWindow* ScenePicker::last_window() {
    return ScenePickerImpl::window_;
}

ScenePicker::~ScenePicker() {
    spi_->unref();
}


TelltaleGroup* ScenePicker::telltale_group() {
    return spi_->tg_;
}

MenuItem* ScenePicker::add_menu(MenuItem* mi, Menu* m) {
    if (m) {
        m->append_item(mi);
    } else {
        spi_->menu_->append_item(mi);
    }
    return mi;
}

MenuItem* ScenePicker::add_menu(const char* name, MenuItem* mi, Menu* m) {
    Menu* mm;
    if (m) {
        mm = m;
    } else {
        mm = spi_->menu_->menu();
    }
    mm->append_item(mi);
    spi_->bil_->push_back(new ButtonItemInfo(name, mi->action(), mi->state(), mi, mm));
    return mi;
}

MenuItem* ScenePicker::add_menu(const char* name, Action* a, Menu* m) {
    MenuItem* mi = K::menu_item(name);
    mi->action(a);
    return add_menu(name, mi, m);
}
MenuItem* ScenePicker::add_radio_menu(const char* name, Action* a, Menu* m) {
    MenuItem* mi = K::radio_menu_item(spi_->tg_, name);
    mi->action(new RadioSelect(name, a, spi_->scene_));
    return add_menu(name, mi, m);
}
Button* ScenePicker::radio_button(const char* name, Action* a) {
    Button* mi = WidgetKit::instance()->radio_button(spi_->tg_,
                                                     name,
                                                     new RadioSelect(name, a, spi_->scene_));
    spi_->bil_->push_back(new ButtonItemInfo(name, mi->action(), mi->state()));
    return mi;
}
MenuItem* ScenePicker::add_radio_menu(const char* name,
                                      Rubberband* rb,
                                      Action* sel,
                                      int tool,
                                      Menu* m) {
    return add_radio_menu(name, new RubberTool(sel, rb, this, tool), m);
}
Button* ScenePicker::radio_button(const char* name, Rubberband* rb, Action* sel, int tool) {
    return radio_button(name, new RubberTool(sel, rb, this, tool));
}
MenuItem* ScenePicker::add_radio_menu(const char* name, OcHandler* h, int tool, Menu* m) {
    return add_radio_menu(name, new HandlerTool(h, this, tool), m);
}

long ScenePickerImpl::info_index(const char* name) {
    for (const auto&& [i, b]: enumerate(*bil_)) {
        if (strcmp(b->name_.string(), name) == 0) {
            return i;
        }
    }
    return -1;
}
void ScenePicker::exec_item(const char* name) {
    long i;
    Scene* s = spi_->scene_;
    if (s->view_count()) {
        XYView* v = s->sceneview(0);
        XYView::current_pick_view(v);
        if (v->canvas()) {
            ScenePickerImpl::window_ = (DismissableWindow*) v->canvas()->window();
        }
    } else {
        XYView::current_pick_view(NULL);
        ScenePickerImpl::window_ = NULL;
    }
    i = spi_->info_index(name);
    if (i > -1) {
        ButtonItemInfo* b = spi_->bil_->at(i);
        TelltaleState* t = b->s_;
        bool chosen = t->test(TelltaleState::is_chosen);
        bool act = !chosen;
        if (t->test(TelltaleState::is_toggle)) {
            t->set(TelltaleState::is_chosen, act);
            act = true;
        } else if (t->test(TelltaleState::is_choosable)) {
            t->set(TelltaleState::is_chosen, true);
        }
        t->notify();
        if (act && b->a_ != NULL) {
            b->a_->execute();
        }
    }
}

void ScenePicker::remove_item(const char* name) {
    long i = spi_->info_index(name);
    if (i > -1) {
        ButtonItemInfo* b = spi_->bil_->at(i);
        spi_->bil_->erase(spi_->bil_->begin() + i);
        GlyphIndex j = b->menu_index();
        if (j > -1) {
            b->parent_->remove_item(j);
        }
        //		if (b->mi_) {
        //			delete b->mi_;
        //		}
        delete b;
    }
}

void ScenePicker::insert_item(const char* insert, const char* name, MenuItem* mi) {
    long i;
    i = spi_->info_index(insert);
    if (i > -1) {
        ButtonItemInfo* b = spi_->bil_->at(i);
        GlyphIndex j = b->menu_index();
        if (j > -1) {
            b->parent_->insert_item(j, mi);
            spi_->bil_->insert(spi_->bil_->begin() + i,
                               new ButtonItemInfo(name, mi->action(), mi->state(), mi, b->parent_));
        }
    }
}

void ScenePicker::set_scene_tool(int t) {
    spi_->scene_->tool(t);
}

void ScenePicker::help() {
    spi_->scene_->help();
}

void ScenePicker::select_name(const char* name) {
    spi_->sel_name_ = name;
}

const char* ScenePicker::select_name() {
    return spi_->sel_name_.string();
}

ScenePickerImpl::ScenePickerImpl(Scene* scene)
    : sel_name_("") {
    menu_ = new PopupMenu();
    menu_->ref();
    tg_ = new TelltaleGroup();
    tg_->ref();
    scene_ = scene;  // not ref'ed since picker deleted when scene is deleted
    bil_ = new std::vector<ButtonItemInfo*>();
    bil_->reserve(20);
}

ScenePickerImpl::~ScenePickerImpl() {
    Resource::unref(menu_);
    Resource::unref(tg_);
    for (ButtonItemInfo* bii: *bil_) {
        delete bii;
    }
    delete bil_;
}

bool ScenePickerImpl::event(Event& e) {
    // printf("ScenePickerImpl::event\n");
    window_ = (DismissableWindow*) e.window();
    menu_->event(e);
    return false;
}

RubberTool::RubberTool(Action* sel, Rubberband* rb, ScenePicker* sp, int tool) {
    sel_ = sel;
    sp_ = sp;
    rb_ = rb;
    Resource::ref(rb_);
    Resource::ref(sel_);
    tool_ = tool;
}

RubberTool::~RubberTool() {
    Resource::unref(rb_);
    Resource::unref(sel_);
}

void RubberTool::execute() {
    sp_->bind_select(rb_);
    sp_->set_scene_tool(tool_);
    if (Oc::helpmode()) {
        rb_->help();
    } else if (sel_) {
        sel_->execute();
    }
}

RadioSelect::RadioSelect(const char* name, Action* a, Scene* s)
    : name_(name) {
    a_ = a;
    Resource::ref(a_);
    s_ = s;  // not referenced
}

RadioSelect::~RadioSelect() {
    Resource::unref(a_);
}

void RadioSelect::execute() {
    handle_old_focus();
    a_->execute();
    s_->picker()->select_name(this->name_.string());
    for (int i = 0; i < s_->view_count(); ++i) {
        XYView* v = s_->sceneview(i);
        v->notify();
    }
}

HandlerTool::HandlerTool(OcHandler* h, ScenePicker* sp, int tool) {
    sp_ = sp;
    h_ = h;
    Resource::ref(h_);
    tool_ = tool;
}

HandlerTool::~HandlerTool() {
    Resource::unref(h_);
}

void HandlerTool::execute() {
    sp_->bind_select(h_);
    sp_->set_scene_tool(tool_);
    if (Oc::helpmode()) {
        if (h_) {
            h_->help();
        } else {
            sp_->help();
        }
    }
}

SceneMover::SceneMover() {
    x_ = 0;
    y_ = 0;
}

SceneMover::~SceneMover() {}

void SceneMover::help() {
    Oc::help(Scene_SceneMover_);
}

bool SceneMover::event(Event& e) {
    if (Oc::helpmode()) {
        if (e.type() == Event::down) {
            help();
        }
    }
    Coord xold = x_, yold = y_;
    x_ = e.pointer_x();
    y_ = e.pointer_y();

    switch (e.type()) {
    case Event::down:
        view_ = XYView::current_pick_view();
        //		view_->rebind();
        e.grab(this);
#ifdef WIN32
        e.window()->grab_pointer();
#endif
        break;
    case Event::motion:
        view_->move_view(x_ - xold, y_ - yold);
        break;
    case Event::up:
        e.ungrab(this);
#ifdef WIN32
        e.window()->ungrab_pointer();
#endif
        break;
    }
    return true;
}

SceneZoom::SceneZoom() {
    x_ = 0;
    y_ = 0;
    xorg_ = 0;
    yorg_ = 0;
}

SceneZoom::~SceneZoom() {}

void SceneZoom::help() {
    Oc::help(Scene_SceneZoom_);
}

bool SceneZoom::event(Event& e) {
    if (Oc::helpmode()) {
        if (e.type() == Event::down) {
            help();
        }
    }
    Coord xold = x_, yold = y_;
    x_ = e.pointer_x();
    y_ = e.pointer_y();

    switch (e.type()) {
    case Event::down:
        view_ = XYView::current_pick_view();
        //		view_->rebind();
        e.grab(this);
#ifdef WIN32
        e.window()->grab_pointer();
#endif
        xorg_ = x_;
        yorg_ = y_;
        break;
    case Event::motion:
        xold = (x_ - xold) / 50;
        yold = (y_ - yold) / 50;
        if (xold > .5)
            xold = .5;
        if (yold > .5)
            yold = .5;
        if (xold < -.5)
            xold = -.5;
        if (yold < -.5)
            yold = -.5;
        view_->scale_view(xorg_, yorg_, xold, yold);
        break;
    case Event::up:
        e.ungrab(this);
#ifdef WIN32
        e.window()->ungrab_pointer();
#endif
        break;
    }
    return true;
}

void RoundView::execute() {
    if (Oc::helpmode()) {
        Oc::help(Scene_RoundView_);
        return;
    }
    XYView* v = XYView::current_pick_view();
    if (!v)
        return;
    Coord x1, y1, x2, y2;
    v->zin(x1, y1, x2, y2);
    double d1, d2;
    int ntic;
    MyMath::round_range_down(x1, x2, d1, d2, ntic);
    x1 = d1;
    x2 = d2;
    MyMath::round_range_down(y1, y2, d1, d2, ntic);
    y1 = d1;
    y2 = d2;

    v->size(x1, y1, x2, y2);
    v->zout(x1, y1, x2, y2);
    v->size(x1, y1, x2, y2);
    v->damage_all();
}

void WholeSceneView::execute() {
    if (Oc::helpmode()) {
        Oc::help(Scene_WholeSceneView_);
        return;
    }
    XYView* v = XYView::current_pick_view();
    if (!v)
        return;
    Scene& s = *v->scene();
    v->size(s.x1(), s.y1(), s.x2(), s.y2());
    Coord x1, y1, x2, y2;
    v->zout(x1, y1, x2, y2);
    v->size(x1, y1, x2, y2);
    v->damage_all();
}

void WholePlotView::execute() {
    if (Oc::helpmode()) {
        Oc::help(Scene_WholePlotView_);
        return;
    }
    XYView* v = XYView::current_pick_view();
    if (!v)
        return;
    Scene& s = *v->scene();
    Coord x1, y1, x2, y2;
    s.wholeplot(x1, y1, x2, y2);
    MyMath::round(x1, x2, MyMath::Expand, 2);
    MyMath::round(y1, y2, MyMath::Expand, 2);
    v->box_size(x1, y1, x2, y2);
    v->zout(x1, y1, x2, y2);
    v->box_size(x1, y1, x2, y2);
    v->damage_all();
}

void ZoomOut10::execute() {
    if (Oc::helpmode()) {
        Oc::help(Scene_ZoomOut10_);
        return;
    }
    XYView* v = XYView::current_pick_view();
    Coord x1, x2, y1, y2;
    v->zout(x1, y1, x2, y2);
    v->size(x1, y1, x2, y2);
    v->damage_all();
}

void ZoomIn10::execute() {
    if (Oc::helpmode()) {
        Oc::help(Scene_ZoomeIn10_);
        return;
    }
    XYView* v = XYView::current_pick_view();
    if (!v)
        return;
    Coord x1, x2, y1, y2;
    v->zin(x1, y1, x2, y2);
    v->size(x1, y1, x2, y2);
    v->damage_all();
}

void SPObjectName::execute() {
    if (Oc::helpmode()) {
        Oc::help(Scene_ObjectName_);
    }
    Scene* s = XYView::current_pick_view()->scene();
    printf("%s\n", hoc_object_name(s->hoc_obj_ptr()));
}

void SpecView::execute() {
    if (Oc::helpmode()) {
        Oc::help(Scene_SpecView_);
    }
    XYView* v = XYView::current_pick_view();
    if (!v)
        return;
    Coord x1, x2, y1, y2;
    v->zin(x1, y1, x2, y2);
    bool bx = var_pair_chooser("X size", x1, x2, v->canvas()->window());
    bool by = var_pair_chooser("Y size", y1, y2, v->canvas()->window());
    v->size(x1, y1, x2, y2);
    v->zout(x1, y1, x2, y2);
    v->size(x1, y1, x2, y2);
    v->damage_all();
}

void SizeScene::execute() {
    if (Oc::helpmode()) {
        Oc::help(Scene_SizeScene_);
        return;
    }
    XYView* v = XYView::current_pick_view();
    if (!v)
        return;
    Coord x1, x2, y1, y2;
    v->zin(x1, y1, x2, y2);
    v->scene()->new_size(x1, y1, x2, y2);
    v->zout(x1, y1, x2, y2);
    v->size(x1, y1, x2, y2);
}

NewView::NewView() {}
NewView::~NewView() {}
void NewView::help() {
    Oc::help(Scene_NewView_);
}

void NewView::execute(Rubberband* rb) {
    if (Oc::helpmode()) {
        help();
        return;
    }
    const Event& e = rb->event();
    XYView* cv = XYView::current_pick_view();
    Coord l, b, r, t;
    Coord x1, y1, x2, y2;
    ((RubberRect*) rb)->get_rect_canvas(l, b, r, t);
    ((RubberRect*) rb)->get_rect(x1, y1, x2, y2);
    XYView* v = cv->new_view(x1, y1, x2, y2);
    ViewWindow* w = new ViewWindow(v, ((PrintableWindow*) (cv->canvas()->window()))->type());
    w->place(l + e.pointer_root_x() - e.pointer_x(), b + e.pointer_root_y() - e.pointer_y());
    w->map();
}

ShowMenu::ShowMenu(Menu* m) {
    m_ = m;
    m->ref();
}
ShowMenu::~ShowMenu() {
    Resource::unref(m_);
}
void ShowMenu::execute() {
    if (Oc::helpmode()) {
        Oc::help(Scene_ShowMenu_);
        return;
    }
    XYView* v = XYView::current_pick_view();
    v->parent()->viewmenu(m_);
}

// following doesn't work correctly yet
void OcViewGlyph::viewmenu(Glyph* m) {
    printf("OcViewGlyph::viewmenu()\n");
    if (!g_) {
        g_ = body();
        Resource::ref(g_);
        LayoutKit& lk = *LayoutKit::instance();
        WidgetKit& wk = *WidgetKit::instance();
        PolyGlyph* hbox = lk.hbox(2);
        hbox->append(lk.center(m, 0, 1));
        hbox->append(lk.center(view(), 0, 1));
        body(hbox);
        printf("add menu\n");
    } else {
        printf("delete menu\n");
        body(g_);
        Resource::unref(g_);
        g_ = NULL;
    }
}

PopupMenu::PopupMenu() {
    menu_ = WidgetKit::instance()->pulldown();
    menu_->ref();
    w_ = NULL;
    grabbed_ = false;
}

PopupMenu::~PopupMenu() {
    Resource::unref(menu_);
    if (w_) {
        delete w_;
    }
}

bool PopupMenu::event(Event& e) {
    if (!w_) {
        w_ = new PopupWindow(menu_);
    }
    switch (e.type()) {
    case Event::down:
        if (!grabbed_) {
            Coord l, b;
            w_->place(e.pointer_root_x(), e.pointer_root_y());
            w_->align(.8, .9);
#if defined(WIN32)
            l = w_->left();
            b = w_->bottom();
            if (b < 0. || l < 0.) {
                w_->align(0., 0.);
                w_->place((l > 0.) ? l : 1., (b > 0.) ? b : 20.);
            }
            w_->map();
#else
            w_->map();
            l = w_->left();
            b = w_->bottom();
            if (b < 0. || l < 0.) {
                w_->unmap();
                w_->align(0., 0.);
                w_->place((l > 0.) ? l : 1., (b > 0.) ? b : 20.);
                w_->map();
            }
#endif
            e.grab(this);
            grabbed_ = true;
            menu_->press(e);
        }
        break;
    case Event::motion:
        if (grabbed_) {
            menu_->drag(e);
        }
        break;
    case Event::up:
        if (grabbed_) {
            e.ungrab(this);
            grabbed_ = false;
            w_->unmap();
            menu_->release(e);
        }
        break;
    }
    return true;
}

void PopupMenu::append_item(MenuItem* mi) {
    menu_->append_item(mi);
}
#endif
