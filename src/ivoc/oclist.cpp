#include <../../nrnconf.h>

#include <stdio.h>
#include "classreg.h"
#include "oclist.h"
#include "oc2iv.h"
#include "hoclist.h"
#include "ocobserv.h"
#include "oc_ansi.h"
#if HAVE_IV
#include <InterViews/adjust.h>
#include <InterViews/hit.h>
#include "ocglyph.h"
#include "checkpnt.h"
#include "apwindow.h"
#include "ocbrowsr.h"
#include "objcmd.h"
#endif

#include "gui-redirect.h"

#include "parse.hpp"
extern Object** hoc_temp_objptr(Object*);
extern Symlist* hoc_top_level_symlist;
int ivoc_list_count(Object*);

extern int hoc_return_type_code;

void handle_old_focus();

#if HAVE_IV
/*static*/ class OcListBrowser: public OcBrowser {
  public:
    OcListBrowser(OcList*, const char* = NULL, Object* pystract = NULL);
    OcListBrowser(OcList*, char**, const char*);
    virtual ~OcListBrowser();
    virtual void drag(const Event& e);
    virtual void select(GlyphIndex);
    virtual void dragselect(GlyphIndex);
    virtual void reload();
    virtual void reload(GlyphIndex);
    virtual void set_select_action(const char*, bool on_rel = false, Object* pyact = NULL);
    virtual void set_accept_action(const char*, Object* pyact = NULL);
    virtual void accept();
    virtual void release(const Event&);
    virtual InputHandler* focus_in();
    void load_item(GlyphIndex i);
    void ocglyph(Window* w);
    void ocglyph_unmap();

  private:
    void change_name(GlyphIndex);

  private:
    OcList* ocl_;
    HocCommand* select_action_;
    HocCommand* accept_action_;
    HocCommand* label_action_;
    HocCommand* label_pystract_;
    bool on_release_;
    char** plabel_;
    CopyString* items_;
    OcGlyph* ocg_;
    bool ignore_;
};
#else
class OcListBrowser {};
#endif

static Symbol* list_class_sym_;
static void chk_list(Object* o) {
    if (!o || o->ctemplate != list_class_sym_->u.ctemplate) {
        check_obj_type(o, "List");
    }
}

static double l_append(void* v) {
    OcList* o = (OcList*) v;
    Object* ob = *hoc_objgetarg(1);
    o->append(ob);
    return o->count();
}
void OcList::append(Object* ob) {
    if (!ob)
        return;
    oref(ob);
    oli_.push_back(ob);
#if HAVE_IV
    if (b_) {
        b_->load_item(count() - 1);
        b_->select_and_adjust(count() - 1);
    }
#endif
}

void OcList::oref(Object* ob) {
    if (!ct_) {
        ++ob->refcount;
    }
}

void OcList::ounref(Object* ob) {
    if (!ct_) {
        hoc_dec_refcount(&ob);
    }
}

void OcList::update(Observable* o) {
    ClassObservable* co = (ClassObservable*) o;
    Object* ob = co->object();
    // printf("notify %d %s\n", co->message(), hoc_object_name(ob));
    switch (co->message()) {
    case ClassObservable::Delete: {
        long i = index(ob);
        if (i >= 0) {
            remove(i);
        }
    } break;
    case ClassObservable::Create:
        append(ob);
        break;
    default:
#if HAVE_IV
        if (b_) {
            long i = index(ob);
            if (i >= 0) {
                b_->reload(i);
            }
        }
#endif
        break;
    }
}

static double l_prepend(void* v) {
    OcList* o = (OcList*) v;
    Object* ob = *hoc_objgetarg(1);
    o->prepend(ob);
    return o->count();
}
void OcList::prepend(Object* ob) {
    if (!ob)
        return;
    oref(ob);
    oli_.insert(oli_.begin(), ob);
#if HAVE_IV
    if (b_) {
        b_->reload();
    }
#endif
}

static double l_insert(void* v) {
    OcList* o = (OcList*) v;
    long i = long(chkarg(1, 0, o->count()));
    Object* ob = *hoc_objgetarg(2);
    o->insert(i, ob);
    return o->count();
}
void OcList::insert(long i, Object* ob) {
    if (!ob)
        return;
    oref(ob);
    oli_.insert(oli_.begin() + i, ob);
#if HAVE_IV
    if (b_) {
        b_->reload();
    }
#endif
}

static double l_count(void* v) {
    OcList* o = (OcList*) v;
    hoc_return_type_code = 1;
    return o->count();
}
long OcList::count() {
    return oli_.size();
}

static double l_remove(void* v) {
    OcList* o = (OcList*) v;
    long i = long(chkarg(1, 0, o->count() - 1));
    o->remove(i);
    return o->count();
}
void OcList::remove(long i) {
    Object* ob = oli_[i];
    oli_.erase(oli_.begin() + i);
#if HAVE_IV
    if (b_) {
        b_->select(-1);
        b_->remove_selectable(i);
        b_->remove(i);
        b_->refresh();
    }
#endif
    ounref(ob);
}

static double l_index(void* v) {
    OcList* o = (OcList*) v;
    Object* ob = *hoc_objgetarg(1);
    hoc_return_type_code = 1;  // integer
    return o->index(ob);
}
long OcList::index(Object* ob) {
    for (long i = 0; i < oli_.size(); ++i) {
        if (oli_[i] == ob) {
            return i;
        }
    }
    return -1;
}

static Object** l_object(void* v) {
    OcList* o = (OcList*) v;
    long i = long(chkarg(1, 0, o->count() - 1));
    return hoc_temp_objptr(o->object(i));
}
Object* OcList::object(long i) {
    return oli_[i];
}

static double l_remove_all(void* v) {
    OcList* o = (OcList*) v;
    o->remove_all();
    return o->count();
}
void OcList::remove_all() {
    for (auto& ob: oli_) {
        ounref(ob);
    }
    oli_.clear();
#if HAVE_IV
    if (b_) {
        b_->select(-1);
        b_->reload();
    }
#endif
}

static double l_browser(void* v) {
    TRY_GUI_REDIRECT_METHOD_ACTUAL_DOUBLE("List.browser", list_class_sym_, v);
#if HAVE_IV
    IFGUI
    char* s = 0;
    char* i = 0;
    char** p = 0;
    OcList* o = (OcList*) v;
    if (ifarg(1)) {
        s = gargstr(1);
    }
    if (ifarg(3)) {
        i = gargstr(3);
        p = hoc_pgargstr(2);
        o->create_browser(s, p, i);
        return 1.;
    }
    if (ifarg(2)) {
        if (hoc_is_object_arg(2)) {
            o->create_browser(s, NULL, *hoc_objgetarg(2));
            return 1.;
        }
        i = gargstr(2);
    }
    o->create_browser(s, i);
    ENDGUI
#endif
    return 1.;
}

static double l_select(void* v) {
    TRY_GUI_REDIRECT_METHOD_ACTUAL_DOUBLE("List.select", list_class_sym_, v);
#if HAVE_IV
    IFGUI
    OcListBrowser* b = ((OcList*) v)->browser();
    long i = (long) (*getarg(1));
    if (b) {
        b->select_and_adjust(i);
    }
    ENDGUI
#endif
    return 1.;
}
static double l_select_action(void* v) {
    TRY_GUI_REDIRECT_METHOD_ACTUAL_DOUBLE("List.select_action", list_class_sym_, v);
#if HAVE_IV
    IFGUI
    OcListBrowser* b = ((OcList*) v)->browser();
    if (b) {
        bool on_rel = false;
        if (ifarg(2)) {
            on_rel = (bool) chkarg(2, 0, 1);
        }
        if (hoc_is_object_arg(1)) {
            b->set_select_action(NULL, on_rel, *hoc_objgetarg(1));
        } else {
            b->set_select_action(gargstr(1), on_rel);
        }
    }
    ENDGUI
#endif
    return 1.;
}
static double l_selected(void* v) {
    hoc_return_type_code = 1;  // integer
    TRY_GUI_REDIRECT_METHOD_ACTUAL_DOUBLE("List.selected", list_class_sym_, v);
#if HAVE_IV
    long i = -1;
    IFGUI
    OcListBrowser* b = ((OcList*) v)->browser();
    if (b) {
        i = b->selected();
    } else {
        i = -1;
    }
    ENDGUI
    return (double) i;
#else
    return 0.;
#endif
}
static double l_accept_action(void* v) {
    TRY_GUI_REDIRECT_METHOD_ACTUAL_DOUBLE("List.accept_action", list_class_sym_, v);
#if HAVE_IV
    IFGUI
    OcListBrowser* b = ((OcList*) v)->browser();
    if (b) {
        if (hoc_is_object_arg(1)) {
            b->set_accept_action(NULL, *hoc_objgetarg(1));
        } else {
            b->set_accept_action(gargstr(1));
        }
    }
    ENDGUI
#endif
    return 1.;
}

static double l_scroll_pos(void* v) {
    TRY_GUI_REDIRECT_METHOD_ACTUAL_DOUBLE("List.scroll_pos", list_class_sym_, v);
#if HAVE_IV
    IFGUI
    OcList* o = (OcList*) v;
    OcListBrowser* b = o->browser();
    if (b) {
        Adjustable* a = b->adjustable();
        if (ifarg(1)) {
            Coord c = (Coord) chkarg(1, 0, 1e9);
            c = (double) o->count() - a->cur_length(Dimension_Y) - c;
            a->scroll_to(Dimension_Y, c);
        }
        // printf("%g %g %g %g\n", (double)o->count(), a->cur_lower(Dimension_Y),
        // a->cur_upper(Dimension_Y), a->cur_length(Dimension_Y));
        return (double) (o->count() - 1) - (double) a->cur_upper(Dimension_Y);
    }
    ENDGUI
#endif
    return -1.;
}


static Member_func l_members[] = {{"append", l_append},
                                  {"prepend", l_prepend},
                                  {"insrt", l_insert},
                                  {"remove", l_remove},
                                  {"remove_all", l_remove_all},
                                  {"index", l_index},
                                  {"count", l_count},
                                  {"browser", l_browser},
                                  {"selected", l_selected},
                                  {"select", l_select},
                                  {"select_action", l_select_action},
                                  {"accept_action", l_accept_action},
                                  {"scroll_pos", l_scroll_pos},
                                  {nullptr, nullptr}};

static Member_ret_obj_func l_retobj_members[] = {{"object", l_object},
                                                 {"o", l_object},
                                                 {nullptr, nullptr}};

static void* l_cons(Object*) {
    OcList* o;
    if (ifarg(1)) {
        if (hoc_is_str_arg(1)) {
            o = new OcList(gargstr(1));
        } else {
            o = new OcList(long(chkarg(1, 0, 1e8)));
        }
    } else {
        o = new OcList();
    }
    o->ref();
    return (void*) o;
}

int ivoc_list_count(Object* olist) {
    chk_list(olist);
    OcList* list = (OcList*) olist->u.this_pointer;
    return list->count();
}

Object* ivoc_list_item(Object* olist, int i) {
    chk_list(olist);
    OcList* list = (OcList*) olist->u.this_pointer;
    if (i >= 0 && i < list->count()) {
        return list->object(i);
    } else {
        return 0;
    }
}

OcList::OcList(long n) {
    b_ = nullptr;
    ct_ = nullptr;
}

OcList::OcList(const char* name) {
    Symbol* s = hoc_lookup(name);
    if (!s) {
        s = hoc_table_lookup(name, hoc_top_level_symlist);
    }
    if (!s || s->type != TEMPLATE) {
        hoc_execerror(name, "is not a template name");
    }
    ct_ = s->u.ctemplate;
    int cnt = ct_->count;
    cnt = (cnt) ? cnt : 5;
    b_ = nullptr;
    hoc_Item* q;
    ITERATE(q, ct_->olist) {
        append(OBJ(q));
    }
    ClassObservable::Attach(ct_, this);
}

static void l_destruct(void* v) {
    OcList* o = (OcList*) v;
    o->unref();
}
OcList::~OcList() {
    if (ct_) {
        ClassObservable::Detach(ct_, this);
    }
#if HAVE_IV
    if (b_) {
        b_->ocglyph_unmap();
    }
    Resource::unref(b_);
#endif
    b_ = NULL;
    remove_all();
}

static int l_chkpt(void** vp) {
#if HAVE_IV
    OcList* o;
    Checkpoint& chk = *Checkpoint::instance();
    if (chk.out()) {
        long cnt;
        o = (OcList*) (*vp);
        cnt = o->count();
        CKPT(chk, cnt);
        for (long i = 0; i < cnt; ++i) {
            Object* item = o->object(i);
            CKPT(chk, item);
        }
    } else {
        long cnt;
        CKPT(chk, cnt);
        o = new OcList(cnt);
        o->ref();
        for (long i = 0; i < cnt; ++i) {
            Object* item;
            CKPT(chk, item);
            o->append(item);
        }
        *vp = (void*) o;
    }
#endif
    return 1;
}

void OcList_reg() {
    // printf("Oclist_reg\n");
    class2oc("List", l_cons, l_destruct, l_members, l_chkpt, l_retobj_members, NULL);
    list_class_sym_ = hoc_lookup("List");
}

extern bool hoc_objectpath_impl(Object* ob, Object* oblook, char* path, int depth);
extern void hoc_path_prepend(char*, const char*, const char*);
int ivoc_list_look(Object* ob, Object* oblook, char* path, int) {
    if (oblook->ctemplate->constructor == l_cons) {
        OcList* o = (OcList*) oblook->u.this_pointer;
        long i, cnt = o->count();
        Object* obj;
        for (i = 0; i < cnt; ++i) {
            obj = o->object(i);
#if 0
			if (obj && obj != oblook &&
			   hoc_objectpath_impl(ob, obj, path, depth) ) {
#else
            if (obj == ob) {
#endif
            auto const tmp = "object(" + std::to_string(i) + ")";
            hoc_path_prepend(path, tmp.c_str(), "");
            return 1;
        }
    }
}
return 0;
}

void OcList::create_browser(const char* name, const char* items, Object* pystract) {
#if HAVE_IV
    if (b_) {
        b_->ocglyph_unmap();
    }
    Resource::unref(b_);
    b_ = new OcListBrowser(this, items, pystract);
    b_->ref();
    PrintableWindow* w = new StandardWindow(b_->standard_glyph());
    b_->ocglyph(w);
    if (name) {
        w->name(name);
    }
    w->map();
#endif
}

void OcList::create_browser(const char* name, char** pstr, const char* action) {
#if HAVE_IV
    if (b_) {
        b_->ocglyph_unmap();
    }
    Resource::unref(b_);
    b_ = new OcListBrowser(this, pstr, action);
    b_->ref();
    PrintableWindow* w = new StandardWindow(b_->standard_glyph());
    b_->ocglyph(w);
    if (name) {
        w->name(name);
    }
    w->map();
#endif
}

OcListBrowser* OcList::browser() {
    return b_;
}

//-----------------------------------------
#if HAVE_IV

OcListBrowser::OcListBrowser(OcList* ocl, const char* items, Object* pystract)
    : OcBrowser() {
    ocl_ = ocl;   // not reffed because this is reffed by ocl
    ocg_ = NULL;  // do not ref
    select_action_ = NULL;
    accept_action_ = NULL;
    plabel_ = NULL;
    label_action_ = NULL;
    label_pystract_ = NULL;
    if (pystract) {
        label_pystract_ = new HocCommand(pystract);
    }
    on_release_ = false;
    ignore_ = false;
    if (items) {
        items_ = new CopyString(items);
    } else {
        items_ = NULL;
    }
    reload();
}

OcListBrowser::OcListBrowser(OcList* ocl, char** pstr, const char* action)
    : OcBrowser() {
    ocl_ = ocl;
    ocg_ = NULL;
    select_action_ = NULL;
    accept_action_ = NULL;
    on_release_ = false;
    ignore_ = false;
    plabel_ = pstr;
    items_ = NULL;
    label_action_ = new HocCommand(action);
    label_pystract_ = NULL;
    reload();
}

OcListBrowser::~OcListBrowser() {
    if (select_action_) {
        delete select_action_;
    }
    if (accept_action_) {
        delete accept_action_;
    }
    if (label_action_) {
        delete label_action_;
    }
    if (label_pystract_) {
        delete label_pystract_;
    }
    if (items_) {
        delete items_;
    }
}

void OcListBrowser::release(const Event& e) {
    OcBrowser::release(e);
    if (select_action_ && on_release_) {
        GlyphIndex i = selected();
        handle_old_focus();
        hoc_ac_ = double(i);
        select_action_->execute();
    }
}

InputHandler* OcListBrowser::focus_in() {
    // works around the problem that the first time a list is used
    // the InputHandler calls focus in on the press, which then
    // selects item 0. Perhaps that is necessary for some kind of initialization
    // but for us with select actions it causes problems. So we temporarily
    // turn off the select actions during focus in handling.
    ignore_ = true;
    InputHandler* ih = OcBrowser::focus_in();
    ignore_ = false;
    return ih;
}

void OcListBrowser::select(GlyphIndex i) {
    OcBrowser::select(i);
    // printf("select %d ignore=%d\n", i, ignore_);
    if (select_action_ && !on_release_ && !ignore_) {
        handle_old_focus();
        hoc_ac_ = (double) i;
        select_action_->execute();
    }
}
void OcListBrowser::dragselect(GlyphIndex i) {
    GlyphIndex old = selected();
    OcBrowser::select(i);
    // printf("select %d old=%d ignore=%d\n", i, old, ignore_);
    if (old != i && select_action_ && !on_release_ && !ignore_) {
        handle_old_focus();
        hoc_ac_ = (double) i;
        select_action_->execute();
    }
}
// mostly copied from src/InterViews/browser.cpp
void OcListBrowser::drag(const Event& e) {
    if (inside(e)) {
        Hit h(&e);
        repick(0, h);
        if (h.any()) {
            dragselect(h.index(0));
            return;
        }
    }
    dragselect(-1);
}

void OcListBrowser::reload() {
    GlyphIndex i, cnt;
    cnt = count();
    for (i = 0; i < cnt; ++i) {
        remove_selectable(0);
        remove(0);
    }
    cnt = ocl_->count();
    for (i = 0; i < cnt; ++i) {
        load_item(i);
    }
    refresh();
}
void OcListBrowser::reload(GlyphIndex i) {
    change_name(i);
}

void OcListBrowser::load_item(GlyphIndex i) {
    append_item("");
    change_name(i);
}

void OcListBrowser::change_name(GlyphIndex i) {
    if (label_pystract_) {
        hoc_ac_ = i;
        char buf[256];
        if (label_pystract_->exec_strret(buf, 256, bool(false))) {
            change_item(i, buf);
        } else {
            change_item(i, "label error");
        }
    } else if (plabel_) {
        hoc_ac_ = i;
        if (label_action_->execute(bool(false)) == 0) {
            change_item(i, *plabel_);
        } else {
            change_item(i, "label error");
        }
    } else if (items_) {
        char* pstr = Oc2IV::object_str(items_->string(), ocl_->object(i));
        if (pstr) {
            change_item(i, pstr);
        } else {
            change_item(i, hoc_object_name(ocl_->object(i)));
        }
    } else {
        change_item(i, hoc_object_name(ocl_->object(i)));
    }
}

void OcListBrowser::set_select_action(const char* s, bool on_rel, Object* pyact) {
    if (select_action_) {
        delete select_action_;
    }
    if (pyact) {
        select_action_ = new HocCommand(pyact);
    } else {
        select_action_ = new HocCommand(s);
    }
    on_release_ = on_rel;
}
void OcListBrowser::set_accept_action(const char* s, Object* pyact) {
    if (accept_action_) {
        delete accept_action_;
    }
    if (pyact) {
        accept_action_ = new HocCommand(pyact);
    } else {
        accept_action_ = new HocCommand(s);
    }
}
void OcListBrowser::accept() {
    if (accept_action_) {
        long i = selected();
        if (i < 0) {
            return;
        }
        handle_old_focus();
        hoc_ac_ = (double) i;
        accept_action_->execute();
    }
}
void OcListBrowser::ocglyph(Window* w) {
    ocg_ = (OcGlyph*) (w->glyph());
    Resource::ref(ocg_);
}

void OcListBrowser::ocglyph_unmap() {
    OcGlyph* o = ocg_;
    ocg_ = NULL;
    if (o) {
        if (o->has_window()) {
            delete o->window();
        }
        Resource::unref(o);
    }
}

#endif
