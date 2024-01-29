#include <../../nrnconf.h>

#if HAVE_IV
#include <OS/string.h>
#include <InterViews/deck.h>
#include <InterViews/patch.h>
#include <InterViews/layout.h>
#include <InterViews/background.h>
#include <IV-look/kit.h>
#include <stdio.h>
#include "ocdeck.h"
#include "apwindow.h"
#include "oc2iv.h"
#endif /* HAVE_IV */
#include "classreg.h"
#include "gui-redirect.h"

#if HAVE_IV
class SpecialPatch: public Patch {
  public:
    SpecialPatch(Glyph*);
    virtual ~SpecialPatch();
    virtual void request(Requisition&) const;
    virtual void allocate(Canvas*, const Allocation&, Extension&);
    virtual void draw(Canvas*, const Allocation&) const;
};

SpecialPatch::SpecialPatch(Glyph* g)
    : Patch(g) {}
SpecialPatch::~SpecialPatch() {}
void SpecialPatch::request(Requisition& req) const {
    Patch::request(req);
}
void SpecialPatch::allocate(Canvas* c, const Allocation& a, Extension& e) {
#if 0
	Allocation aa = a;
	if (aa.bottom() < 0.) {
		Allotment& y = aa.y_allotment();
		y.span((y.origin() - 0.)/y.alignment());
//printf("allotment %g %g %g %g %g\n", y.origin(), y.span(), y.alignment(), y.begin(), y.end());
//printf("SpecialPatch::allocate a.bottom=%g aa.bottom=%g\n", a.bottom(), aa.bottom());
}
	Patch::allocate(c, aa, e);
#else
    Patch::allocate(c, a, e);
#endif
}

void SpecialPatch::draw(Canvas* c, const Allocation& a) const {
#if 1
    Allocation aa = a;
    if (aa.bottom() < 0.) {
        Allotment& y = aa.y_allotment();
        y.span((y.origin() - 0.) / y.alignment());
    }
    Patch::draw(c, aa);
#else
    Patch::draw(c, a);
#endif
}

/*static*/ class OcDeckImpl {
  public:
    PolyGlyph* ocglyph_list_;
    Deck* deck_;
    Object* oc_ref_;  // reference to oc "this"
    CopyString* save_action_;
};
#endif /* HAVE_IV */

static void* cons(Object*) {
    TRY_GUI_REDIRECT_OBJ("Deck", NULL);
#if HAVE_IV
    OcDeck* b = NULL;
    IFGUI
    b = new OcDeck();
    b->ref();
    ENDGUI
    return (void*) b;
#else
    return nullptr;
#endif /* HAVE_IV  */
}

static void destruct(void* v) {
    TRY_GUI_REDIRECT_NO_RETURN("~Deck", v);
#if HAVE_IV
    IFGUI
    OcDeck* b = (OcDeck*) v;
    if (b->has_window()) {
        b->window()->dismiss();
    }
    b->unref();
    ENDGUI
#endif /* HAVE_IV  */
}

static double intercept(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Deck.intercept", v);
#if HAVE_IV
    bool b = int(chkarg(1, 0., 1.));
    IFGUI((OcDeck*) v)->intercept(b);
    ENDGUI
    return double(b);
#else
    return 0.;
#endif /* HAVE_IV  */
}

static double map(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Deck.map", v);
#if HAVE_IV
    IFGUI
    OcDeck* b = (OcDeck*) v;
    PrintableWindow* w;
    if (ifarg(3)) {
        w = b->make_window(float(*getarg(2)),
                           float(*getarg(3)),
                           float(*getarg(4)),
                           float(*getarg(5)));
    } else {
        w = b->make_window();
    }
    if (ifarg(1)) {
        char* name = gargstr(1);
        w->name(name);
    }
    w->map();
    ENDGUI
    return 1.;
#else
    return 0.;
#endif /* HAVE_IV  */
}

static double unmap(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Deck.unmap", v);
#if HAVE_IV
    IFGUI
    OcDeck* b = (OcDeck*) v;
    if (b->has_window()) {
        b->window()->dismiss();
    }
    ENDGUI
    return 0.;
#else
    return 0.;
#endif /* HAVE_IV  */
}

static double save(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Deck.save", v);
#if HAVE_IV
    IFGUI
    OcDeck* b = (OcDeck*) v;
#if 0
	int i;
	Object* o[4];
	for (i=0; i < 4; ++i) {
		if (ifarg(i+1)) {
			o[i] = *hoc_objgetarg(i+1);
		}else{
			o[i] = NULL;
		}
	}
	b->save_action(gargstr(1), o[0]);
#else
    b->save_action(gargstr(1), 0);
#endif
    ENDGUI
    return 1.;
#else
    return 0.;
#endif /* HAVE_IV  */
}

static double flip_to(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Deck.flip_to", v);
#if HAVE_IV
    int i = -1;
    IFGUI
    OcDeck* b = (OcDeck*) v;
    i = int(chkarg(1, -1, b->count() - 1));
    b->flip_to(i);
    ENDGUI
    return double(i);
#else
    return 0.;
#endif /* HAVE_IV  */
}

static double remove_last(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Deck.remove_last", v);
#if HAVE_IV
    IFGUI((OcDeck*) v)->remove_last();
    ENDGUI
    return 0.;
#else
    return 0.;
#endif /* HAVE_IV  */
}

static double remove(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Deck.remove", v);
#if HAVE_IV
    IFGUI
    OcDeck* b = (OcDeck*) v;
    b->remove((int) chkarg(1, 0, b->count() - 1));
    ENDGUI
    return 0.;
#else
    return 0.;
#endif /* HAVE_IV  */
}

static double move_last(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Deck.move_last", v);
#if HAVE_IV
    IFGUI
    OcDeck* b = (OcDeck*) v;
    b->move_last((int) chkarg(1, 0, b->count() - 1));
    ENDGUI
    return 0.;
#else
    return 0.;
#endif /* HAVE_IV  */
}

static Member_func members[] = {{"flip_to", flip_to},
                                {"intercept", intercept},
                                {"save", save},
                                {"map", map},
                                {"unmap", unmap},
                                {"remove_last", remove_last},
                                {"remove", remove},
                                {"move_last", move_last},
                                {0, 0}};

void OcDeck_reg() {
    class2oc("Deck", cons, destruct, members, NULL, NULL, NULL);
}
#if HAVE_IV
OcDeck::OcDeck()
    : OcGlyphContainer() {
    WidgetKit& wk = *WidgetKit::instance();
    LayoutKit& lk = *LayoutKit::instance();
    bi_ = new OcDeckImpl;
    bi_->ocglyph_list_ = new PolyGlyph();
    bi_->deck_ = new Deck(2);
    Resource::ref(bi_->ocglyph_list_);
    Resource::ref(bi_->deck_);
    body(new SpecialPatch(new Background(
        //			wk.inset_frame(
        lk.flexible(bi_->deck_
                    //			  )
                    ),
        wk.background())));
    bi_->oc_ref_ = NULL;
    bi_->save_action_ = NULL;
}

OcDeck::~OcDeck() {
    Resource::unref(bi_->ocglyph_list_);
    Resource::unref(bi_->deck_);
    if (bi_->oc_ref_) {
        hoc_dec_refcount(&bi_->oc_ref_);
    }
    if (bi_->save_action_) {
        delete (bi_->save_action_);
    }
    delete bi_;
}

void OcDeck::flip_to(int i) {
    bi_->deck_->flip_to(GlyphIndex(i));
    ((SpecialPatch*) body())->reallocate();
    ((SpecialPatch*) body())->redraw();
}

void OcDeck::box_append(OcGlyph* g) {
    WidgetKit& wk = *WidgetKit::instance();
    LayoutKit& lk = *LayoutKit::instance();
    bi_->ocglyph_list_->append(g);
    bi_->deck_->append(g);
}

void OcDeck::remove_last() {
    GlyphIndex last = bi_->ocglyph_list_->count() - 1;
    if (last < 0) {
        return;
    }
    if (bi_->deck_->card() == last) {
        flip_to(-1);
    }
    bi_->ocglyph_list_->remove(last);
    bi_->deck_->remove(last);
}

void OcDeck::remove(long i) {
    if (bi_->deck_->card() == i) {
        flip_to(-1);
    }
    bi_->ocglyph_list_->remove(i);
    bi_->deck_->remove(i);
}

void OcDeck::move_last(int i) {
    int last = bi_->ocglyph_list_->count() - 1;
    if (i == last) {
        return;
    }
    OcGlyph* g = (OcGlyph*) bi_->ocglyph_list_->component(last);
    bi_->ocglyph_list_->insert(i, g);
    bi_->deck_->insert(i, g);
    last = bi_->ocglyph_list_->count() - 1;
    bi_->ocglyph_list_->remove(last);
    bi_->deck_->remove(last);
}

void OcDeck::save_action(const char* creat, Object* o) {
    bi_->save_action_ = new CopyString(creat);
    if (o) {
        bi_->oc_ref_ = o;
        ++bi_->oc_ref_->refcount;
    }
}

void OcDeck::save(std::ostream& o) {
    char buf[256];
    if (bi_->save_action_) {
        Sprintf(buf, "{ocbox_ = %s", bi_->save_action_->string());
        o << buf << std::endl;
    } else {
        o << "{ocbox_ = new Deck()" << std::endl;
        o << "ocbox_list_.prepend(ocbox_)" << std::endl;
        o << "ocbox_.intercept(1)}" << std::endl;
        long i, cnt = bi_->ocglyph_list_->count();
        for (i = 0; i < cnt; ++i) {
            ((OcGlyph*) bi_->ocglyph_list_->component(i))->save(o);
        }
        o << "{ocbox_ = ocbox_list_.object(0)" << std::endl;
        o << "ocbox_list_.remove(0)" << std::endl;
        o << "ocbox_.intercept(0)" << std::endl;
    }
    if (has_window()) {
        Sprintf(buf,
                "ocbox_.map(\"%s\", %g, %g, %g, %g)}",
                window()->name(),
                window()->save_left(),
                window()->save_bottom(),
                window()->width(),
                window()->height());
        o << buf << std::endl;
    } else {
        o << "ocbox_.map()}" << std::endl;
    }
    if (bi_->oc_ref_) {
        Sprintf(buf, "%s = ocbox_", hoc_object_pathname(bi_->oc_ref_));
        o << buf << std::endl;
    }
}
#endif /* HAVE_IV */
