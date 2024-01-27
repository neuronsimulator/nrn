#include <../../nrnconf.h>
/*
 * Copyright (c) 1991 Stanford University
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

/* hacked by Michael Hines from filechooser.cpp */

/*
 * SymChooser -- select a name
 */

#if HAVE_IV

#include <IV-look/choice.h>
#include <IV-look/dialogs.h>
#include <IV-look/fbrowser.h>
#include <IV-look/kit.h>
#include <InterViews/action.h>
#include <InterViews/event.h>
#include <InterViews/font.h>
#include <InterViews/hit.h>
#include <InterViews/input.h>
#include <InterViews/layout.h>
#include <InterViews/scrbox.h>
#include <InterViews/style.h>
#include <InterViews/target.h>
#include <InterViews/session.h>
#include <InterViews/display.h>
#include <OS/string.h>
#include <stdio.h>

#include "symchoos.h"
#include "utility.h"
#include "symdir.h"

#include "oc2iv.h"
#include "parse.hpp"
#include "ivoc.h"
#endif /* HAVE_IV */

#include "classreg.h"
#include "gui-redirect.h"

#if HAVE_IV

class SymChooserImpl {
  private:
    friend class SymChooser;
    friend class SymBrowserAccept;
    SymChooserImpl(int nbrowser);
    ~SymChooserImpl();

    WidgetKit* kit_;
    SymChooser* fchooser_;
    int nbrowser_;
    int browser_index_;
    FileBrowser** fbrowser_;
    FieldEditor* editor_;
    FieldEditor* filter_;
    FieldEditor* directory_filter_;
    int* filter_map_;
    SymDirectory** dir_;
    SymChooserAction* action_;
    std::string selected_;
    CopyString last_selected_;
    int last_index_;
    Style* style_;
    Action* update_;

    void init(SymChooser*, Style*, SymChooserAction*);
    void scfree();
    void build();
    Menu* makeshowmenu();
    void clear(int);
    void load(int);
    void show(int);
    void show_all();
    void show_var();
    void show_objid();
    void show_objref();
    void show_sec();
    void show_pysec();
    double* selected_var();
    int selected_vector_count();
    FieldEditor* add_filter(Style*,
                            const char* pattern_attribute,
                            const char* default_pattern,
                            const char* caption_attribute,
                            const char* default_caption,
                            Glyph*,
                            FieldEditorAction*);
    bool filtered(const String&, FieldEditor*);
    void accept_browser();
    void accept_browser_index(int);
    void cancel_browser();
    void editor_accept(FieldEditor*);
    void filter_accept(FieldEditor*);
    bool chdir(int, int);
};

/*static*/ class SymBrowserAccept: public Action {
  public:
    SymBrowserAccept(SymChooserImpl*, int);
    virtual ~SymBrowserAccept();
    virtual void execute();

  private:
    SymChooserImpl* sci_;
    int browser_index_;
};

SymBrowserAccept::SymBrowserAccept(SymChooserImpl* sci, int browser_index) {
    sci_ = sci;
    browser_index_ = browser_index;
}

SymBrowserAccept::~SymBrowserAccept() {}

void SymBrowserAccept::execute() {
    sci_->accept_browser_index(browser_index_);
}
#endif /* HAVE_IV */

static void* scons(Object*) {
    TRY_GUI_REDIRECT_OBJ("SymChooser", NULL);
#if HAVE_IV
    SymChooser* sc = NULL;
    IFGUI
    const char* caption = "Choose a Variable Name or";
    if (ifarg(1)) {
        caption = gargstr(1);
    }
    Style* style = new Style(Session::instance()->style());
    style->attribute("caption", caption);
    if (ifarg(2)) {
        Symbol* sym = hoc_lookup(gargstr(2));
        int type = RANGEVAR;
        if (sym) {
            type = sym->type;
        }
        sc = new SymChooser(new SymDirectory(type), WidgetKit::instance(), style, NULL, 1);
    } else {
        sc = new SymChooser(NULL, WidgetKit::instance(), style);
    }
    Resource::ref(sc);
    ENDGUI
    return (void*) sc;
#else
    return nullptr;
#endif /* HAVE_IV */
}
static void sdestruct(void* v) {
    TRY_GUI_REDIRECT_NO_RETURN("~SymChooser", v);
#if HAVE_IV
    SymChooser* sc = (SymChooser*) v;
    Resource::unref(sc);
#endif /* HAVE_IV */
}
static double srun(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("SymChooser.run", v);
#if HAVE_IV
    bool b = false;
    IFGUI
    SymChooser* sc = (SymChooser*) v;
    Display* d = Session::instance()->default_display();
    Coord x, y;
    if (nrn_spec_dialog_pos(x, y)) {
        b = sc->post_at_aligned(x, y, 0.0, 0.0);
    } else {
        b = sc->post_at_aligned(d->width() / 2, d->height() / 2, .5, .5);
    }
    ENDGUI
    return double(b);
#else
    return 0.;
#endif /* HAVE_IV */
}
static double text(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("SymChooser.text", v);
#if HAVE_IV
    IFGUI
    SymChooser* sc = (SymChooser*) v;
    hoc_assign_str(hoc_pgargstr(1), sc->selected().c_str());
    ENDGUI
    return 0.;
#else
    return 0.;
#endif /* HAVE_IV */
}
static Member_func members[] = {{"run", srun}, {"text", text}, {0, 0}};
void SymChooser_reg() {
    class2oc("SymChooser", scons, sdestruct, members, NULL, NULL, NULL);
}
#if HAVE_IV

declareActionCallback(SymChooserImpl)
implementActionCallback(SymChooserImpl)

declareFieldEditorCallback(SymChooserImpl)
implementFieldEditorCallback(SymChooserImpl)
#define SHOW_SECTION 1

SymChooser::SymChooser(SymDirectory* dir,
                       WidgetKit* kit,
                       Style* s,
                       SymChooserAction* a,
                       int nbrowser)
    : Dialog(NULL, s) {
    impl_ = new SymChooserImpl(nbrowser);
    SymChooserImpl& fc = *impl_;
    if (dir) {
        fc.dir_[0] = dir;
    } else {
#if SHOW_SECTION
        fc.dir_[0] = new SymDirectory(SECTION);
#else
        fc.dir_[0] = new SymDirectory(-1);
#endif
    }
    Resource::ref(dir);
    fc.kit_ = kit;
    fc.init(this, s, a);
}

SymChooser::~SymChooser() {
    impl_->scfree();
    delete impl_;
}

const std::string& SymChooser::selected() const {
    return impl_->selected_;
}

double* SymChooser::selected_var() {
    return impl_->selected_var();
}

int SymChooser::selected_vector_count() {
    return impl_->selected_vector_count();
}

void SymChooser::reread() {
    SymChooserImpl& fc = *impl_;
#if 0
    if (!fc.chdir(fc.dir_->path())) {
	/* should generate an error message */
    }
#endif
}

void SymChooser::dismiss(bool accept) {
    Dialog::dismiss(accept);
    SymChooserImpl& fc = *impl_;
    if (fc.action_ != NULL) {
        fc.action_->execute(this, accept);
    }
}

/** class SymChooserImpl **/
SymChooserImpl::SymChooserImpl(int nbrowser) {
    nbrowser_ = nbrowser;
    dir_ = new SymDirectory*[nbrowser];
    fbrowser_ = new FileBrowser*[nbrowser];
    last_index_ = -1;
    for (int i = 0; i < nbrowser_; ++i) {
        dir_[i] = NULL;
        fbrowser_[i] = NULL;
    }
}
SymChooserImpl::~SymChooserImpl() {
    delete[] dir_;
    delete[] fbrowser_;
}

void SymChooserImpl::init(SymChooser* chooser, Style* s, SymChooserAction* a) {
    fchooser_ = chooser;
    filter_map_ = NULL;
    Resource::ref(a);
    action_ = a;
    style_ = new Style(s);
    Resource::ref(style_);
    style_->alias("FileChooser");
    style_->alias("Dialog");
    update_ = new ActionCallback(SymChooserImpl)(this, &SymChooserImpl::build);
    style_->add_trigger_any(update_);
    build();
}

void SymChooserImpl::scfree() {
    for (int i = nbrowser_ - 1; i >= 0; --i) {
        Resource::unref(dir_[i]);
    }
    delete[] filter_map_;
    Resource::unref(action_);
    style_->remove_trigger_any(update_);
    Resource::unref(style_);
}

void SymChooserImpl::build() {
    WidgetKit& kit = *kit_;
    const LayoutKit& layout = *LayoutKit::instance();
    Style* s = style_;
    kit.push_style();
    kit.style(s);
    String caption("");
    s->find_attribute("caption", caption);
    String subcaption("Enter  Symbol name:");
    s->find_attribute("subcaption", subcaption);
    String open("Accept");
    s->find_attribute("open", open);
    String close("Cancel");
    s->find_attribute("cancel", close);
    long rows = 10;
    s->find_attribute("rows", rows);
    const Font* f = kit.font();
    FontBoundingBox bbox;
    f->font_bbox(bbox);
    Coord height = rows * (bbox.ascent() + bbox.descent()) + 1.0;
    Coord width;
    if (!s->find_attribute("width", width)) {
        width = 16 * f->width('m') + 3.0;
    }

    int i;
    Action* accept = new ActionCallback(SymChooserImpl)(this, &SymChooserImpl::accept_browser);
    Action* cancel = new ActionCallback(SymChooserImpl)(this, &SymChooserImpl::cancel_browser);
    editor_ = DialogKit::instance()->field_editor(
        "", s, new FieldEditorCallback(SymChooserImpl)(this, &SymChooserImpl::editor_accept, NULL));
    browser_index_ = 0;
    for (i = 0; i < nbrowser_; ++i) {
        fbrowser_[i] = new FileBrowser(kit_, new SymBrowserAccept(this, i), NULL);
    }
    fchooser_->remove_all_input_handlers();
    fchooser_->append_input_handler(editor_);
    for (i = 0; i < nbrowser_; ++i) {
        fchooser_->append_input_handler(fbrowser_[i]);
    }
    fchooser_->next_focus();

    Glyph* g = layout.vbox();
    if (caption.length() > 0) {
        g->append(layout.r_margin(kit.fancy_label(caption), 5.0, fil, 0.0));
    }
    if (subcaption.length() > 0) {
        g->append(layout.r_margin(kit.fancy_label(subcaption), 5.0, fil, 0.0));
    }
    g->append(layout.vglue(5.0, 0.0, 2.0));
    g->append(editor_);
    g->append(layout.vglue(5.0, 0.0, 2.0));
    g->append(makeshowmenu());
    g->append(layout.vglue(15.0, 0.0, 12.0));
    PolyGlyph* h = layout.hbox(nbrowser_);
    Glyph* b;
    for (i = 0; i < nbrowser_; ++i) {
        b = layout.hbox(layout.vcenter(kit.inset_frame(layout.margin(
                                           layout.natural_span(fbrowser_[i], width, height), 1.0)),
                                       1.0),
                        layout.hspace(4.0),
                        kit.vscroll_bar(fbrowser_[i]->adjustable()));
        h->append(b);
    }
    g->append(h);
    g->append(layout.vspace(15.0));
    if (s->value_is_on("filter")) {
        FieldEditorAction* action = new FieldEditorCallback(
            SymChooserImpl)(this, &SymChooserImpl::filter_accept, NULL);
        filter_ = add_filter(s, "filterPattern", "", "filterCaption", "Filter:", g, action);
        if (s->value_is_on("directoryFilter")) {
            directory_filter_ = add_filter(s,
                                           "directoryFilterPattern",
                                           "",
                                           "directoryFilterCaption",
                                           "Name Filter:",
                                           g,
                                           action);
        } else {
            directory_filter_ = NULL;
        }
    } else {
        filter_ = NULL;
        directory_filter_ = NULL;
    }
    g->append(layout.hbox(layout.hglue(10.0),
                          layout.vcenter(kit.default_button(open, accept)),
                          layout.hglue(10.0, 0.0, 5.0),
                          layout.vcenter(kit.push_button(close, cancel)),
                          layout.hglue(10.0)));

    fchooser_->body(layout.vcenter(kit.outset_frame(layout.margin(g, 5.0)), 1.0));
    kit.pop_style();
    load(0);
}

Menu* SymChooserImpl::makeshowmenu() {
    WidgetKit& k = *WidgetKit::instance();
    Menu* mb = k.menubar();
    MenuItem* mi = k.menubar_item("Show");
    mb->append_item(mi);
    Menu* mp = k.pulldown();
    mi->menu(mp);
    TelltaleGroup* ttg = new TelltaleGroup();

    mi = K::radio_menu_item(ttg, "All");
    mi->action(new ActionCallback(SymChooserImpl)(this, &SymChooserImpl::show_all));
    mp->append_item(mi);
    mi->state()->set(TelltaleState::is_chosen, true);

    mi = K::radio_menu_item(ttg, "Variables");
    mi->action(new ActionCallback(SymChooserImpl)(this, &SymChooserImpl::show_var));
    mp->append_item(mi);

    mi = K::radio_menu_item(ttg, "Object refs");
    mi->action(new ActionCallback(SymChooserImpl)(this, &SymChooserImpl::show_objref));
    mp->append_item(mi);

    mi = K::radio_menu_item(ttg, "Objects");
    mi->action(new ActionCallback(SymChooserImpl)(this, &SymChooserImpl::show_objid));
    mp->append_item(mi);

    mi = K::radio_menu_item(ttg, "Sections");
    mi->action(new ActionCallback(SymChooserImpl)(this, &SymChooserImpl::show_sec));
    mp->append_item(mi);
#if SHOW_SECTION
    mi->state()->set(TelltaleState::is_chosen, true);
#endif
    mi = K::radio_menu_item(ttg, "Python Sections");
    mi->action(new ActionCallback(SymChooserImpl)(this, &SymChooserImpl::show_pysec));
    mp->append_item(mi);

    return mb;
}

void SymChooserImpl::show(int i) {
    Resource::unref(dir_[0]);
    dir_[0] = new SymDirectory(i);
    clear(0);
    load(0);
}

void SymChooserImpl::show_all() {
    show(-1);
}
void SymChooserImpl::show_var() {
    show(VAR);
}
void SymChooserImpl::show_objref() {
    show(OBJECTVAR);
}
void SymChooserImpl::show_objid() {
    show(TEMPLATE);
}
void SymChooserImpl::show_sec() {
    show(SECTION);
}
void SymChooserImpl::show_pysec() {
    show(PYSEC);
}

void SymChooserImpl::clear(int bindex) {
    for (int bi = bindex; bi < nbrowser_; ++bi) {
        // printf("clearing %d\n", bi);
        FileBrowser& b = *fbrowser_[bi];
        b.select(-1);
        GlyphIndex n = b.count();
        for (GlyphIndex i = 0; i < n; i++) {
            b.remove_selectable(0);
            b.remove(0);
        }
        b.refresh();
        //	b.undraw(); // with this the slider gets changed. albeit the mouse has
        // to run over it. But the new load doesn't appear til
        // the mouse runs over it
    }
}

void SymChooserImpl::load(int bindex) {
    SymDirectory& d = *dir_[bindex];
    Browser& b = *fbrowser_[bindex];
    WidgetKit& kit = *kit_;
    kit.push_style();
    kit.style(style_);
    const LayoutKit& layout = *LayoutKit::instance();
    int dircount = d.count();
    delete[] filter_map_;
    int* index = new int[dircount];
    filter_map_ = index;
    // printf("loading %d\n", bindex);
    for (int i = 0; i < dircount; i++) {
        const String& f = d.name(i).c_str();
        bool is_dir = d.is_directory(i);
        if ((is_dir && filtered(f, directory_filter_)) || (!is_dir && filtered(f, filter_))) {
            Glyph* name = kit.label(f);
            if (is_dir) {
                if (d.symbol(i) && d.symbol(i)->type == TEMPLATE) {
                    name = layout.hbox(name, kit.label("_"));
                } else {
                    name = layout.hbox(name, kit.label("."));
                }
            }
            Glyph* label = new Target(layout.h_margin(name, 3.0, 0.0, 0.0, 15.0, fil, 0.0),
                                      TargetPrimitiveHit);
            TelltaleState* t = new TelltaleState(TelltaleState::is_enabled);
            b.append_selectable(t);
            b.append(new ChoiceItem(t, label, kit.bright_inset_frame(label)));
            *index++ = i;
        }
    }
    // the order of the following two statements is important for carbon
    // to avoid a premature browser request which ends up showing an
    // empty list.
    fbrowser_[bindex]->refresh();
    editor_->field(d.path().c_str());
    kit.pop_style();
}

FieldEditor* SymChooserImpl::add_filter(Style* s,
                                        const char* pattern_attribute,
                                        const char* default_pattern,
                                        const char* caption_attribute,
                                        const char* default_caption,
                                        Glyph* body,
                                        FieldEditorAction* action) {
    String pattern(default_pattern);
    s->find_attribute(pattern_attribute, pattern);
    String caption(default_caption);
    s->find_attribute(caption_attribute, caption);
    FieldEditor* e = DialogKit::instance()->field_editor(pattern, s, action);
    fchooser_->append_input_handler(e);
    WidgetKit& kit = *kit_;
    LayoutKit& layout = *LayoutKit::instance();
    body->append(layout.hbox(layout.vcenter(kit.fancy_label(caption), 0.5),
                             layout.hspace(2.0),
                             layout.vcenter(e, 0.5)));
    body->append(layout.vspace(10.0));
    return e;
}

bool SymChooserImpl::filtered(const String& name, FieldEditor* e) {
    if (e == NULL) {
        return true;
    }
    const String* s = e->text();
    if (s == NULL || s->length() == 0) {
        return true;
    }
    return s == NULL || s->length() == 0 || SymDirectory::match(name.string(), s->string());
}

void SymChooserImpl::accept_browser_index(int bindex) {
    int i = int(fbrowser_[bindex]->selected());
    if (i == -1) {
        return;
    }
    //    i = filter_map_[i];
    SymDirectory* dir = dir_[bindex];
    const String& path = dir->path().c_str();
    const String& name = dir->name(i).c_str();
    Symbol* sym = dir->symbol(i);
    int length = path.length() + name.length();
    auto const tmp_len = length + 2;
    char* tmp = new char[tmp_len];
    std::snprintf(
        tmp, tmp_len, "%.*s%.*s", path.length(), path.string(), name.length(), name.string());
    editor_->field(tmp);
    last_selected_ = tmp;
    last_index_ = i;
    selected_ = editor_->text()->string();
    if (dir->is_directory(i)) {
        if (chdir(bindex, i)) {
            fchooser_->focus(editor_);
        } else {
            /* should generate an error message */
        }
    } else {
        clear(bindex + 1);
        browser_index_ = bindex;
        //	fchooser_->dismiss(true);
    }
    delete[] tmp;
}

double* SymChooserImpl::selected_var() {
    if (last_index_ != -1 && selected_ == last_selected_.string()) {
        SymDirectory* dir = dir_[browser_index_];
        return dir->variable(last_index_);
    } else {
        return NULL;
    }
}

int SymChooserImpl::selected_vector_count() {
    if (last_index_ != -1 && selected_ == last_selected_.string()) {
        SymDirectory* dir = dir_[browser_index_];
        return dir->whole_vector(last_index_);
    } else {
        return 0;
    }
}

void SymChooserImpl::accept_browser() {
    int bi = browser_index_;
    int i = int(fbrowser_[bi]->selected());
    if (i == -1) {
        editor_accept(editor_);
        return;
    }
    //    i = filter_map_[i];
    const String& path = dir_[bi]->path().c_str();
    const String& name = dir_[bi]->name(i).c_str();
    int length = path.length() + name.length();
    char* tmp = new char[length + 1];
    std::snprintf(
        tmp, length + 1, "%.*s%.*s", path.length(), path.string(), name.length(), name.string());
    // printf("accept_browser %s\n", tmp);
    editor_->field(tmp);
    selected_ = editor_->text()->string();
    if (dir_[bi]->is_directory(i)) {
        if (chdir(bi, i)) {
            fchooser_->focus(editor_);
        } else {
            /* should generate an error message */
        }
    } else {
        fchooser_->dismiss(true);
    }
    delete[] tmp;
}

void SymChooserImpl::cancel_browser() {
    selected_.clear();
    fchooser_->dismiss(false);
}

void SymChooserImpl::editor_accept(FieldEditor* e) {
    if (int i = dir_[browser_index_]->index(e->text()->string()); i >= 0) {
        if (!chdir(browser_index_, i)) {
            selected_ = dir_[browser_index_]->name(i);
            fchooser_->dismiss(true);
        }
        return;
    } else {
        selected_ = e->text()->string();
        fchooser_->dismiss(true);
    }
}

void SymChooserImpl::filter_accept(FieldEditor*) {
    clear(0);
    load(0);
}

bool SymChooserImpl::chdir(int bindex, int index) {
    if (dir_[bindex]->is_directory(index)) {
        int bi;
        SymDirectory* d;
        if (dir_[bindex]->obj(index)) {
            d = new SymDirectory(dir_[bindex]->obj(index));
            bi = bindex;
        } else if (dir_[bindex]->is_pysec(index)) {
            d = dir_[bindex]->newsymdir(index);
            bi = bindex + 1;
        } else {
            d = new SymDirectory(dir_[bindex]->path(),
                                 dir_[bindex]->object(),
                                 dir_[bindex]->symbol(index),
                                 dir_[bindex]->array_index(index));
            bi = bindex + 1;
        }
        if (bi > nbrowser_ - 1) {
            bi = nbrowser_ - 1;
            // rotate
        }
        Resource::ref(d);
        browser_index_ = bi;
        Resource::unref(dir_[bi]);
        dir_[bi] = d;
        clear(bi);
        load(bi);
        return true;
    }
    return false;
}

/** class SymChooserAction **/

SymChooserAction::SymChooserAction() {}
SymChooserAction::~SymChooserAction() {}
void SymChooserAction::execute(SymChooser*, bool) {}
#endif /* HAVE_IV */
