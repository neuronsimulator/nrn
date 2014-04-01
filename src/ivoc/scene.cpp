#include <../../nrnconf.h>
#if HAVE_IV // to end of file

/* I have shamelessly hacked away at the page implementation to
   create the Scene class. There really isn't much in common anymore but
   I happily acknowlege the debt.
*/
/*
 * Copyright (c) 1987, 1988, 1989, 1990, 1991 Stanford University
 * Copyright (c) 1991 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and 
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Stanford and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Stanford and Silicon Graphics.
 * 
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
 *
 * IN NO EVENT SHALL STANFORD OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
 * OF THIS SOFTWARE.
 */

/*
 * Scene - arbitrary placements seen from several views
 */

#include <ivstream.h>
#include <stdio.h>
#include <assert.h>

#include <InterViews/canvas.h>
#include <InterViews/hit.h>
#include <InterViews/session.h>
#include <InterViews/style.h>
#include <InterViews/color.h>
#include <InterViews/brush.h>
#include <InterViews/background.h>
#include <OS/list.h>

#include "mymath.h"
#include "epsprint.h"
#include "scenevie.h"
#include "scenepic.h"
#include "idraw.h"
#include "ivoc.h"

#define Scene_Move_Text_	"MoveText Graph"
#define Scene_ChangeColor_	"ChangeColor Graph"
#define Scene_Delete_		"Delete Graph"

static const int SceneInfoShowing = 0x01;
static const int SceneInfoFixed = 0x02;
static const int SceneInfoViewFixed = 0x04;
static const int SceneInfoAllocated = 0x08;

class SceneInfo {
public:
    SceneInfo();
    SceneInfo(Glyph*, Coord x=0, Coord y=0);
    void pinfo();
    Glyph* glyph_;
    Allocation allocation_;
    Coord x_;
    Coord y_;
    short status_;
};

void SceneInfo::pinfo() {
	Allocation& a = allocation_;
	printf("allocation %g %g %g %g\n", a.left(), a.bottom(), a.right(), a.top());
}

SceneInfo::SceneInfo() {
	glyph_ = NULL;
	x_ = 0;
	y_ = 0;
	status_ = 0;
}

SceneInfo::SceneInfo(Glyph* g, Coord x, Coord y) {
    glyph_ = g;
    x_ = x;
    y_ = y;
    status_ = SceneInfoShowing;
}

declareList(SceneInfo_List,SceneInfo);
implementList(SceneInfo_List,SceneInfo);
declarePtrList(XYView_PtrList,XYView);
implementPtrList(XYView_PtrList,XYView);
declarePtrList(Scene_PtrList,Scene);
implementPtrList(Scene_PtrList,Scene);

static const float epsilon = 0.001;
static Scene_PtrList* scene_list;

Coord Scene::mbs_;
Coord Scene::mbs() const {return mbs_;}
static const Brush* mb_brush_;
static const Color* mb_color_;

void Scene::check_allocation(GlyphIndex index) {
	//will not redraw unless allocation is changed
	//use damage(index) to do a definite redraw on a constant allocation
	SceneInfo& info = info_->item_ref(index);
        Requisition s;
        info.glyph_->request(s);
	Allocation a_old = info.allocation_;
	Allocation& a = info.allocation_;
        Allotment ax = Allotment(
           info.x_,
           s.requirement(Dimension_X).natural(),
           s.requirement(Dimension_X).alignment()
        );
        Allotment ay = Allotment(
           info.y_,
           s.requirement(Dimension_Y).natural(),
           s.requirement(Dimension_Y).alignment()
        );
        a.allot(Dimension_X, ax);
        a.allot(Dimension_Y, ay);
	if (info.status_ & SceneInfoAllocated) {
		if (!a_old.equals(a, epsilon)) {
			damage(index, a_old);
			damage(index);
		}
	}else{
		damage(index);
	}
	info.status_ |= SceneInfoAllocated;
}

void Scene::modified(GlyphIndex index) {
	//will not redraw unless allocation is changed
	//use damage(index) to do a definite redraw on a constant allocation
	SceneInfo& info = info_->item_ref(index);
        Requisition s;
        info.glyph_->request(s);
	Allocation a_old = info.allocation_;
	Allocation& a = info.allocation_;
        Allotment ax = Allotment(
           info.x_,
           s.requirement(Dimension_X).natural(),
           s.requirement(Dimension_X).alignment()
        );
        Allotment ay = Allotment(
           info.y_,
           s.requirement(Dimension_Y).natural(),
           s.requirement(Dimension_Y).alignment()
        );
        a.allot(Dimension_X, ax);
        a.allot(Dimension_Y, ay);
//printf("Scene::modified(%d) allocation %g %g %g %g\n", index, a.left(), a.bottom(), a.right(), a.top());
	if ((info.status_ & SceneInfoAllocated) && !a_old.equals(a, epsilon)) {
//printf("damaged\n");
		damage(index, a_old);
	}
	damage(index);
	info.status_ |= SceneInfoAllocated;
}

static const Color* scene_background_;
static const Color* scene_foreground_;

const Color* Scene::default_background() {
	if (!scene_background_) {
		Style* s = Session::instance()->style();
		String c;
		if (!s->find_attribute("Scene_background", c )
		   || (scene_background_ = Color::lookup(
		       Session::instance()->default_display(), c)) == NULL)
		{
			scene_background_ = Color::lookup(
				Session::instance()->default_display(), "#ffffff" );
		}
		Resource::ref(scene_background_);
	}
	return scene_background_;
}

const Color* Scene::default_foreground() {
	if (!scene_foreground_) {
		Style* s = Session::instance()->style();
		String c;
		if (!s->find_attribute("Scene_foreground", c )
		   || (scene_foreground_ = Color::lookup(
		       Session::instance()->default_display(), c)) == NULL)
		{
			scene_foreground_ = Color::lookup(
				Session::instance()->default_display(), "#000000" );
		}
		Resource::ref(scene_foreground_);
	}
	return scene_foreground_;
}

Scene::Scene(Coord x1, Coord y1, Coord x2, Coord y2, Glyph* bg) : Glyph() {
    drawing_fixed_item_ = false;
    tool_ = NOTOOL;
    background_ = NULL;
    background(bg);
    info_ = new SceneInfo_List();
    views_ = new XYView_PtrList();
    x1_orig_ = x1;
    x2_orig_ = x2;
    y1_orig_ = y1;
    y2_orig_ = y2;
    x1_ = x1;
    x2_ = x2;
    y1_ = y1;
    y2_ = y2;
    if (!scene_list) {
    	scene_list = new Scene_PtrList;
    }
    if (mbs_ == 0.) {
#if MAC		
	mbs_ = 10.;
#endif
	Session::instance()->style()->find_attribute("scene_menu_box_size", mbs_);
	if (mbs_ > 0.) {
		mb_color_ = new Color(
			ColorIntensity(.5),
			ColorIntensity(.5),
			ColorIntensity(.5)
		);
		mb_brush_ = new Brush(1);
		Resource::ref(mb_color_);
		Resource::ref(mb_brush_);	
	}else{
		mbs_ = -1.;
	}
//	printf ("mbs_=%g\n", mbs_);
    }
    scene_list->append(this);
    picker_ = NULL;
    mark_ = false;
    hoc_obj_ptr_ = NULL;
}

void Scene::background(Glyph* bg) {
	Resource::unref(background_);
	if (bg) {
	    background_ = bg;
	}else{
		background_ = new Background(NULL, default_background());
	}
	Resource::ref(background_);
}

int Scene::tool() { return tool_; }
void Scene::tool(int t) { tool_ = t; notify(); }

void Scene::help() {
	switch (tool()) {
	case MOVE:
		Oc::help(Scene_Move_Text_);
		break;
	case DELETE:
		Oc::help(Scene_Delete_);
		break;
	case CHANGECOLOR:
		Oc::help(Scene_ChangeColor_);
		break;
	default:
		printf("No help for this tool\n");
		break;
	}
}

XYView* Scene::sceneview(int i) const {
	if(views_->count()) {
		return views_->item(i);
	}else{
		return NULL;
	}
}

void Scene::new_size(Coord x1, Coord y1, Coord x2, Coord y2) {
#if 1
	if (x1 == x2) { x1 -= 1.; x2 += 1.;}
	if (y1 == y2) { y1 -= 1.; y2 += 1.;}
	x1_ = x1;
	y1_ = y1;
	x2_ = x2;
	y2_ = y2;
#endif
#if 1
	//resize first view
	if (views_->count()) {
		XYView* v = views_->item(0);
//		v->origin(x1, y1);
//		v->x_span(x2 - x1);
//		v->y_span(y2 - y1);
		v->box_size(x1, y1, x2, y2);
		if (v->canvas()) {
			v->damage_all();
		}
	}
#endif

#if 0
	//resize all views to correspond to the new size
	damage_all();
	for (long i = 0; i < views_->count(); ++i) {
		XYView* v = views_->item(i);
		v->x_span(x2 - x1);
		v->y_span(y2 - y1);
		v->origin(x1, y1);
	}
	GlyphIndex count = info_->count();
	for (i=0; i < count; ++i) {
		modified(i);
	}
#endif
	notify();
}

Scene::~Scene() {
//printf("~Scene\n");
    GlyphIndex count = info_->count();
    for (GlyphIndex i = 0; i < count; ++i) {
        SceneInfo& info = info_->item_ref(i);
	Resource::unref(info.glyph_);
    }
    delete info_;
    info_ = NULL;
    Resource::unref(background_);
    if (picker_) {
    	delete picker_;
    }
    //only xyview can manipulate this list. when xyview is deleted it
    //will remove itself from this list. There is no way to delete scene
    //without first deleteing all the views.
    assert(views_->count() == 0);
    	
#if 0
    count = views_->count();
    for (i = 0; i < count; ++i) {
    	XYView* view = views_->item(i);
    	Resource::unref(view);
    }
    views_->remove_all();
#endif
    for (long j = 0; j < scene_list->count(); ++j) {
    	if (scene_list->item(j) == this) {
    		scene_list->remove(j);
		break;
    	}
    }
   delete views_;
}

void Scene::wholeplot(Coord& l, Coord& b, Coord& r, Coord& t) const {
	l = x1();
	b = y1();
	r = x2();
	t = y2();
}

int Scene::view_count() const {
	return int(views_->count());
}

void Scene::append_view(XYView* v) {
	views_->append(v);
//	Resource::ref(v);
}

void Scene::remove_view(XYView* v) {
	long count = views_->count();
	for (long i = 0; i < count; ++i) {
		if (v == views_->item(i)) {
			views_->remove(i);
			break;
//			Resource::unref(v);
		}
	}
}

void Scene::dismiss() {
	long count = views_->count();
	for (long i = count - 1; i >= 0; --i) {
		OcViewGlyph* g = views_->item(i)->parent();
		if (g && g->has_window()) {
			g->window()->dismiss();
			g->window(NULL);
		}
	}
}

void Scene::damage(GlyphIndex index) {
	SceneInfo& info = info_->item_ref(index);
	Allocation& a = info.allocation_;
	long count = views_->count();
	for (long i = 0; i < count; ++i) {
//printf("damage view\n");
		XYView* view = views_->item(i);
		view->damage(info.glyph_, a,
				(info.status_ & SceneInfoFixed) != 0,
				(info.status_ & SceneInfoViewFixed) != 0
		);
	}
}

void Scene::damage(GlyphIndex index, const Allocation& a) {
	SceneInfo& info = info_->item_ref(index);
	long count = views_->count();
	for (long i = 0; i < count; ++i) {
		XYView* view = views_->item(i);
		view->damage(info.glyph_, a,
				(info.status_ & SceneInfoFixed) != 0,
				(info.status_ & SceneInfoViewFixed) != 0
		);
	}
}

void Scene::damage_all() {
	for (long i = 0; i < views_->count(); ++i) {
		XYView* v = views_->item(i);
		if (v->canvas()) {
			v->damage_all();
		}
	}
}

void Scene::damage(Coord x1, Coord y1, Coord x2, Coord y2) {
	long count = views_->count();
	for (long i = 0; i < count; ++i) {
		XYView* view = views_->item(i);
		view->damage(x1, y1, x2, y2);
	}
}

void Scene::show(GlyphIndex index, bool showing) {
    SceneInfo& info = info_->item_ref(index);
    if (((info.status_ & SceneInfoShowing) == SceneInfoShowing) != showing) {
//printf("show %d showing=%d want %d\n", index, (info.status_ & SceneInfoHidden) == 0, showing);
//info.pinfo();
        if (showing) {
            info.status_ |= SceneInfoShowing;
        } else {
            info.status_ &= ~SceneInfoShowing;
        }
	modified(index);
    }
}

bool Scene::showing(GlyphIndex index) const {
    return (info_->item_ref(index).status_ & SceneInfoShowing) != 0;
}

void Scene::move(GlyphIndex index, Coord x, Coord y) {
    SceneInfo& info = info_->item_ref(index);
    float x1=info.x_, y1=info.y_;
    info.x_ = x;
    info.y_ = y;

    if (!(info.status_ & SceneInfoAllocated) || x1 != x || y1 != y) {
	modified(index);
    }
}

void Scene::location(GlyphIndex index, Coord& x, Coord& y) const {
    SceneInfo& info = info_->item_ref(index);
    x = info.x_;
    y = info.y_;
}

GlyphIndex Scene::count() const {
    return info_->count();
}

Glyph* Scene::component(GlyphIndex index) const {
    return info_->item_ref(index).glyph_;
}

void Scene::allotment(GlyphIndex index, DimensionName res, Allotment& a) const {
    a = info_->item_ref(index).allocation_.allotment(res);
}

void Scene::change(GlyphIndex index) {
    modified(index);
}

void Scene::change_to_fixed(GlyphIndex index, XYView* v) {
	SceneInfo& info = info_->item_ref(index);
	if (info.status_ & SceneInfoViewFixed) {
		info.status_ &= ~SceneInfoViewFixed;
printf("changed to fixed\n");
		v->view_ratio(info.x_, info.y_, info.x_, info.y_);
		v->s2o().transform(info.x_, info.y_);
	}
	info.status_ |= SceneInfoFixed;
    modified(index);
}

void Scene::change_to_vfixed(GlyphIndex index, XYView* v) {
	SceneInfo& info = info_->item_ref(index);
	if (!(info.status_ & SceneInfoViewFixed)) {
		info.status_ |= SceneInfoViewFixed;
		info.status_ |= SceneInfoFixed;
printf("changed to vfixed\n");
		v->s2o().inverse_transform(info.x_, info.y_);
		v->ratio_view(info.x_, info.y_, info.x_, info.y_);
	}
    modified(index);
}

void Scene::append(Glyph* glyph) {
    SceneInfo info(glyph);
    info_->append(info);
    Resource::ref(glyph);
//    modified(info_->count() - 1);
}

void Scene::append_fixed(Glyph* glyph) {
    SceneInfo info(glyph);
    info.status_ |= SceneInfoFixed;
    info_->append(info);
    Resource::ref(glyph);
//    modified(info_->count() - 1);
}

void Scene::append_viewfixed(Glyph* glyph) {
//printf("Scene::append_viewfixed\n");
    SceneInfo info(glyph);
    info.status_ |= SceneInfoFixed | SceneInfoViewFixed;
    info_->append(info);
    Resource::ref(glyph);
//    modified(info_->count() - 1);
}

void Scene::prepend(Glyph* glyph) {
    SceneInfo info(glyph);
    info_->prepend(info);
    Resource::ref(glyph);
//    modified(0);
}

void Scene::insert(GlyphIndex index, Glyph* glyph) {
    SceneInfo info(glyph);
    info_->insert(index, info);
    Resource::ref(glyph);
//    modified(index);
}

void Scene::remove(GlyphIndex index) {
    SceneInfo& info = info_->item_ref(index);
    damage(index);
    Resource::unref(info.glyph_);
    info_->remove(index);
}

void Scene::replace(GlyphIndex index, Glyph* glyph) {
    SceneInfo& info = info_->item_ref(index);
    damage(index);
    Resource::ref(glyph);
    Resource::unref(info.glyph_);
    info.glyph_ = glyph;
    modified(index);
}

GlyphIndex Scene::glyph_index(const Glyph* g) {
	GlyphIndex i, cnt=info_->count();;
	for (i=0; i < cnt; ++i) {
		if (info_->item_ref(i).glyph_ == g) {
			return i;
		}
	}
	return -1;
}

void Scene::request(Requisition& req) const {
//printf("Scene::request\n");
#if 0
    if (background_ != NULL) {
        background_->request(requisition);
    }
#endif
	Requirement rx(x2() - x1(),0,0,-x1()/(x2() - x1()));
	Requirement ry(y2() - y1(),0,0,-y1()/(y2() - y1()));
	req.require_x(rx);
	req.require_y(ry);
}

void Scene::allocate(Canvas* c, const Allocation& a, Extension& ext) {
//printf("Scene::allocate\n");
    GlyphIndex count = info_->count();
    for (GlyphIndex index = 0; index < count; ++index) {
	check_allocation(index);
    }
    ext.set(c, a);
}

#if 0
#include <IV-X11/xcanvas.h>
#include <InterViews/transformer.h>
void candam(Canvas* c) {
	const CanvasDamage& cd = c->rep()->damage_;
	printf("damage %g %g %g %g\n", cd.left, cd.bottom, cd.right, cd.top);
	const Transformer& t = c->transformer();
	Coord x1, y1, x2, y2;
	t.inverse_transform(cd.left, cd.bottom, x1, y1);
	t.inverse_transform(cd.right, cd.top, x2, y2);
	printf("  model %g %g %g %g\n", x1, y1, x2, y2);
}
#endif

void Scene::draw(Canvas* canvas, const Allocation& a) const {
//printf("Scene::draw");
//candam(canvas);
    if (background_ != NULL) {
    	background_->draw(canvas, a);
    }
	// the menu selection area
	if (mbs() > 0.) {
		Coord l,t;
		canvas->transformer().transform(a.left(), a.top(), l, t);
		if (canvas->damaged(l, t-mbs_, l+mbs_, t)) {
//			printf("draw box at corner (%g, %g)\n", l, t);
			canvas->push_transform();
			Transformer tr;
			canvas->transformer(tr);
			canvas->rect(l, t-mbs_, l+mbs_, t, mb_color_, mb_brush_);
			canvas->pop_transform();
		}
	}
    GlyphIndex count = info_->count();
    bool are_fixed = false;
    for (GlyphIndex index = 0; index < count; ++index) {
        SceneInfo& info = info_->item_ref(index);
	if (info.status_ & SceneInfoFixed) {
	    are_fixed = true;
	}else if (info.glyph_ != NULL && (info.status_ & SceneInfoShowing)) {
            Allocation& a = info.allocation_;
            Extension b;
	    b.set(canvas, a);
//printf("%d alloc %g %g %g %g\n", index, a.left(), a.bottom(), a.right(), a.top());
//printf("%d exten %g %g %g %g\n", index, b.left(), b.bottom(), b.right(), b.top());
            if (canvas->damaged(b)) {
                info.glyph_->draw(canvas, a);
            }
        }
    }

 if (are_fixed) {
    ((Scene*)this)->drawing_fixed_item_ = true;
    canvas->push_transform();
    //Transformer tv;
    //view_transform(canvas, 2, tv);
    const Transformer& tv = XYView::current_draw_view()->s2o();
    canvas->transform(tv);
    IfIdraw(pict(tv));
    for (GlyphIndex index = 0; index < count; ++index) {
        SceneInfo& info = info_->item_ref(index);
	if ((info.status_ & SceneInfoFixed)
	  && info.glyph_ != NULL && (info.status_ & SceneInfoShowing)) {
		Allocation a = info.allocation_;
		Coord x, y;
		if (!(info.status_ & SceneInfoViewFixed)) {
			tv.inverse_transform(a.x(), a.y(), x, y);
		}else{
			XYView::current_draw_view()->view_ratio(a.x(), a.y(), x, y);
		}
		a.x_allotment().origin(x);
		a.y_allotment().origin(y);
		Extension b;
		b.set(canvas, a);
		if (canvas->damaged(b)) {
			info.glyph_->draw(canvas, a);
		}
//printf("%d alloc %g %g %g %g\n", index, a.left(), a.bottom(), a.right(), a.top());
//printf("%d exten %g %g %g %g\n", index, b.left(), b.bottom(), b.right(), b.top());
        }
    }
    ((Scene*)this)->drawing_fixed_item_ = false;
    canvas->pop_transform();
    IfIdraw(end());
  }
}

void Scene::print(Printer* canvas, const Allocation& a) const {
    if (background_ != NULL) {
    	background_->print(canvas, a);
    }
    GlyphIndex count = info_->count();
    bool are_fixed = false;
    for (GlyphIndex index = 0; index < count; ++index) {
        SceneInfo& info = info_->item_ref(index);
	if (info.status_ & SceneInfoFixed) {
	    are_fixed = true;
	}else if (info.glyph_ != NULL && (info.status_ & SceneInfoShowing)) {
            Allocation& a = info.allocation_;
            Extension b;
	    b.set(canvas, a);
            if (canvas->damaged(b)) {
                info.glyph_->print(canvas, a);
            }
        }
    }

 if (are_fixed) {
    ((Scene*)this)->drawing_fixed_item_ = true;
    canvas->push_transform();
    //Transformer tv;
    //view_transform(canvas, 2, tv);
    const Transformer& tv = XYView::current_draw_view()->s2o();
    canvas->transform(tv);
    for (GlyphIndex index = 0; index < count; ++index) {
        SceneInfo& info = info_->item_ref(index);
	if ((info.status_ & SceneInfoFixed)
	  && info.glyph_ != NULL && (info.status_ & SceneInfoShowing)) {
		Allocation a = info.allocation_;
		Coord x, y;
		if (!(info.status_ & SceneInfoViewFixed)) {
			tv.inverse_transform(a.x(), a.y(), x, y);
		}else{
			XYView::current_draw_view()->view_ratio(a.x(), a.y(), x, y);
		}
		a.x_allotment().origin(x);
		a.y_allotment().origin(y);
		Extension b;
		b.set(canvas, a);
		if (canvas->damaged(b)) {
			info.glyph_->print(canvas, a);
		}
        }
    }
    ((Scene*)this)->drawing_fixed_item_ = false;
    canvas->pop_transform();
  }
}

void Scene::pick(Canvas* c, const Allocation& a, int depth, Hit& h) {
	menu_picked_ = false;
	if (mbs() > 0. && picker_ && h.event() && h.event()->type() == Event::down) {
		Coord ax, ay, ex, ey;
		c->transformer().transform(h.left(), h.top(), ex, ey);
		c->transformer().transform(a.left(), a.top(), ax, ay);
		//printf("a=(%g,%g) e=(%g,%g)\n", ax, ay, ex, ey);
		if (MyMath::inside(ex, ey, ax, ay-mbs_, ax+mbs_, ay)) {
			picker()->pick_menu(this, depth, h);
			menu_picked_ = true;
			return;
		}
	}
    if (picker_ && picker()->pick(c, this, depth, h)) {
    	return;
    }
    if (background_ != NULL) {
        background_->pick(c, a, depth, h);
    }
    GlyphIndex count = info_->count();
#if 0
    for (GlyphIndex index = 0; index < count; ++index) {
        SceneInfo& info = info_->item_ref(index);
        if (info.glyph_ != NULL && (info.status_ & SceneInfoShowing)) {
            Allocation& a = info.allocation_;
            if (
                h.right() >= a.left() && h.left() < a.right()
                && h.top() >= a.bottom() && h.bottom() < a.top()
            ) {
		h.begin(depth, this, index);
                info.glyph_->pick(c, a, depth + 1, h);
		h.end();
            }
        }
    }
#else
    // pick with some extra epsilon in canvas coords
	Coord epsx = XYView::current_pick_view()->x_pick_epsilon();
	Coord epsy = XYView::current_pick_view()->y_pick_epsilon();
	
    bool are_fixed = false;
    for (GlyphIndex index = 0; index < count; ++index) {
        SceneInfo& info = info_->item_ref(index);
	if (info.status_ & SceneInfoFixed) {
	    are_fixed = true;
	}else if (info.glyph_ != NULL && (info.status_ & SceneInfoShowing)) {
            Allocation& a = info.allocation_;
            if (
                h.right() >= a.left() - epsx  && h.left() < a.right() + epsx
                && h.top() >= a.bottom() - epsy && h.bottom() < a.top() + epsy
            ) {
		h.begin(depth, this, index);
                info.glyph_->pick(c, a, depth + 1, h);
		h.end();
            }
        }
    }

 if (are_fixed) {
    //Transformer tv;
    //view_transform(c, 2, tv);
    const Transformer& tv = XYView::current_pick_view()->s2o();
    float scx, scy, tmp;
    tv.matrix(scx,tmp,tmp,scy,tmp,tmp);
    for (GlyphIndex index = 0; index < count; ++index) {
        SceneInfo& info = info_->item_ref(index);
	if ((info.status_ & SceneInfoFixed)
	  && info.glyph_ != NULL && (info.status_ & SceneInfoShowing)) {
		Allocation a = info.allocation_;
		Coord l,r,t,b;
		if (info.status_ & SceneInfoViewFixed) {
			Coord x, y;
			XYView::current_pick_view()->view_ratio(a.x(), a.y(), x, y);
			a.x_allotment().origin(x);
			a.y_allotment().origin(y);
			tv.transform(a.left(), a.bottom(), l, b);
			tv.transform(a.right(), a.top(), r, t);
		}else{
			l = (a.left() - a.x())*scx + a.x();
			r = (a.right() - a.x())*scx + a.x();
			t = (a.top() - a.y())*scy + a.y();
			b = (a.bottom() - a.y())*scy + a.y();
		}
//printf("%g %g %g %g  %g %g %g %g  %g %g\n", a.left(), a.bottom(), a.right(),
//a.top(), l,r,t,b, h.left(), h.bottom());
            if (
                h.right() >= l && h.left() < r
                && h.top() >= b && h.bottom() < t
            ) {
		h.begin(depth, this, index);
                info.glyph_->pick(c, a, depth + 1, h);
		h.end();
            }
        }
    }
  }
#endif
}

long Scene::scene_list_index(Scene* s) {
	long i, cnt = scene_list->count();
	for (i = 0; i < cnt; ++i) {
		if (s == scene_list->item(i)) {
			return i;
		}
	}
	return -1;
}

void Scene::save_all(ostream& o) {
	char buf[200];
	o << "objectvar save_window_, rvp_" << endl;
	if (!scene_list) {
		return;
	}
	long count = scene_list->count();
	if (count) {
		sprintf(buf, "objectvar scene_vector_[%ld]", count);
		o << buf << endl;
	}
	for (long i = 0; i < count; ++i) {
		Scene* s = scene_list->item(i);
		s->mark(false);
	}
}

void Scene::save_class(ostream& o, const char* s) {
	long count = views_->count();
//	PrintableWindow* w = (PrintableWindow*)canvas()->window();
	o << "save_window_ = new " << s << "(0)" << endl;
	char buf[256];
	Coord left, top, right, bottom;
	if (view_count()) {
		sceneview(0)->zin(left, bottom, right, top);
	}else{
		left = x1();
		right = x2();
		bottom = y1();
		top = y2();
	}
		
	sprintf(buf, "save_window_.size(%g,%g,%g,%g)", left, right, bottom, top);
	o << buf << endl;
}

void Scene::save_phase1(ostream&) {}
void Scene::save_phase2(ostream&) {}

void Scene::printfile(const char* fname) {
	if (view_count()) {
		views_->item(0)->printfile(fname);
	}
}

void XYView::printfile(const char* fname) {
	filebuf obuf;
	if (!obuf.open(fname, IOS_OUT)) {
		return;
	}
	ostream o(&obuf);
	EPSPrinter* pr = new EPSPrinter(&o);
	Allocation a;
	Allotment ax(0, xsize_, 0);
	Allotment ay(0, ysize_, 0);
	a.allot_x(ax);
	a.allot_y(ay);
	pr->eps_prolog(o, xsize_, ysize_);
	pr->resize(0, 0, xsize_, ysize_);
	pr->clip_rect(0, 0, xsize_, ysize_);
	pr->damage_all();
	print(pr, a);
	pr->epilog();
	undraw();		
	obuf.close();
	delete pr;
	PrintableWindowManager::current()->psfilter(fname);
}

#endif
