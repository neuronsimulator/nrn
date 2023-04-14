#include <../../nrnconf.h>
#if HAVE_IV  // to end of file

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fstream>

#include <InterViews/dialog.h>
#include <InterViews/session.h>
#include <InterViews/display.h>
#include <InterViews/action.h>
#include <InterViews/layout.h>
#include <InterViews/style.h>
#include <InterViews/hit.h>
#include <InterViews/event.h>
#include <IV-look/kit.h>
#include <IV-look/dialogs.h>
#include <OS/string.h>

#include "graph.h"
#include "utility.h"
#include "oc2iv.h"
#include "ivoc.h"

bool nrn_spec_dialog_pos(Coord& x, Coord& y) {
    Style* s = Session::instance()->style();
    if (s->value_is_on("dialog_spec_position")) {
        s->find_attribute("dialog_left_position", x);
        s->find_attribute("dialog_bottom_position", y);
        return true;
    }
    return false;
}

bool oc_post_dialog(Dialog* d, Coord x, Coord y) {
    if (nrn_spec_dialog_pos(x, y)) {
        return d->post_at_aligned(x, y, 0.0, 0.0);
    }
    if (x != 400. || y != 400.) {
        return d->post_at_aligned(x, y, .5, .5);
    } else {
        Display* dis = Session::instance()->default_display();
        return d->post_at_aligned(dis->width() / 2, dis->height() / 2, .5, .5);
    }
}

bool ok_to_write(const String& s, Window* w) {
    return ok_to_write(s.string(), w);
}
bool ok_to_read(const String& s, Window* w) {
    return ok_to_read(s.string(), w);
}

class OcGlyphDialog: public Dialog {
  public:
    OcGlyphDialog(Glyph*, Style*);
    virtual ~OcGlyphDialog();
    virtual void pick(Canvas*, const Allocation&, int depth, Hit&);
};

/*static*/ class DialogAction: public Action {
  public:
    DialogAction(Dialog*, bool);
    virtual ~DialogAction();
    virtual void execute() {
        d_->dismiss(accept_);
    }

  private:
    Dialog* d_;
    bool accept_;
};
DialogAction::DialogAction(Dialog* d, bool accept) {
    accept_ = accept;
    d_ = d;
}
DialogAction::~DialogAction() {
    // printf("~DialogAction\n");
}

bool boolean_dialog(const char* label,
                    const char* accept,
                    const char* cancel,
                    Window* w,
                    Coord x,
                    Coord y) {
    WidgetKit& k = *WidgetKit::instance();
    LayoutKit& l = *LayoutKit::instance();
    PolyGlyph* vbox = l.vbox();
    bool ok;
    Dialog* d = new Dialog(k.outset_frame(l.margin(vbox, 5)), Session::instance()->style());
    d->ref();
    vbox->append(l.hcenter(k.inset_frame(l.margin(k.label(label), 10))));
    vbox->append(l.hcenter(l.hbox(k.push_button(accept, new DialogAction(d, true)),
                                  l.hspace(10),
                                  k.push_button(cancel, new DialogAction(d, false)))));
    if (w) {
        ok = d->post_for(w);
    } else {
        ok = oc_post_dialog(d, x, y);
    }
    d->unref();
    return ok;
}

void continue_dialog(const char* label, Window* w, Coord x, Coord y) {
    WidgetKit& k = *WidgetKit::instance();
    LayoutKit& l = *LayoutKit::instance();
    PolyGlyph* vbox = l.vbox();
    Dialog* d = new Dialog(k.outset_frame(l.margin(vbox, 5)), Session::instance()->style());
    d->ref();
    vbox->append(l.hcenter(k.inset_frame(l.margin(k.label(label), 10))));
    vbox->append(l.hcenter(k.push_button("Continue", new DialogAction(d, true))));
    if (w) {
        d->post_for(w);
    } else {
        oc_post_dialog(d, x, y);
    }
    d->unref();
}

static bool ok_if_already_exists(const char* s, Window* w) {
    char buf[256];
    Sprintf(buf, "%s already exists: Write?", s);
    return boolean_dialog(buf, "Go Ahead", "Don't", w);
}

static void open_fail(const char* s, Window* w, const char* io) {
    char buf[256];
    Sprintf(buf, "Couldn't open %s for %sing", s, io);
    continue_dialog(buf, w);
}

bool ok_to_write(const char* s, Window* w) {
    std::filebuf obuf;
    if (obuf.open(s, std::ios::in)) {
        obuf.close();
        if (!ok_if_already_exists(s, w)) {
            errno = 0;
            return false;
        }
    }
    if (obuf.open(s, std::ios::app | std::ios::out)) {
        obuf.close();
    } else {
        open_fail(s, w, "writ");
        errno = 0;
        return false;
    }
    errno = 0;
    return true;
}

bool ok_to_read(const char* s, Window* w) {
    std::filebuf obuf;
    if (obuf.open(s, std::ios::in)) {
        obuf.close();
        errno = 0;
        return true;
    } else {
        open_fail(s, w, "read");
        errno = 0;
        return false;
    }
}

bool var_pair_chooser(const char* caption, float& x, float& y, Window* w, Coord x2, Coord y2) {
    char buf[200];
    float x1 = x, y1 = y;
    for (;;) {
        Sprintf(buf, "%g %g", x, y);
        if (str_chooser(caption, buf, w, x2, y2)) {
            if (sscanf(buf, "%f%f", &x1, &y1) == 2) {
                x = x1;
                y = y1;
                return true;
            } else {
                continue_dialog("Invalid entry: Enter pair of numbers separated by space.", w);
            }
        } else {
            return false;
        }
    }
}

bool str_chooser(const char* caption, char* buf, Window* w, Coord x, Coord y) {
    WidgetKit& k = *WidgetKit::instance();
    LayoutKit& l = *LayoutKit::instance();
    Style* style = new Style(k.style());
    style->attribute("caption", caption);
    bool ok;
    FieldDialog* d = FieldDialog::field_dialog_instance(buf, style);
    d->ref();
    if (w) {
        ok = d->post_for(w);
    } else {
        ok = oc_post_dialog(d, x, y);
    }
    if (ok) {
        strcpy(buf, d->text()->string());
    }
    d->unref();
    return ok;
}

declareFieldEditorCallback(FieldDialog);
implementFieldEditorCallback(FieldDialog);

FieldDialog::FieldDialog(Glyph* g, Style* s)
    : Dialog(g, s) {}

FieldDialog::~FieldDialog() {
    Resource::unref(fe_);
}

bool FieldDialog::run() {
    fe_->select(0, fe_->text()->length());
    return Dialog::run();
}
void FieldDialog::dismiss(bool accept) {
    if (accept) {
        s_ = *fe_->text();
    } else {
        fe_->field(s_);
    }
    Dialog::dismiss(accept);
}

FieldDialog* FieldDialog::field_dialog_instance(const char* str, Style* style, Glyph* extra) {
    Glyph* g;
    WidgetKit& widgets = *WidgetKit::instance();
    DialogKit& dialogs = *DialogKit::instance();
    LayoutKit& layout = *LayoutKit::instance();

    String caption("");
    String accept("Accept");
    String cancel("Cancel");
    style->find_attribute("caption", caption);
    style->find_attribute("accept", accept);
    style->find_attribute("cancel", cancel);
    PolyGlyph* hb = layout.hbox(5);
    PolyGlyph* vb = layout.vbox(5);
    g = widgets.inset_frame(layout.margin(layout.flexible(vb, fil, 0), 10.0));

    FieldDialog* fd = new FieldDialog(g, style);
    FieldEditor* fe = dialogs.field_editor(
        str,
        style,
        new FieldEditorCallback(FieldDialog)(fd, &FieldDialog::accept, &FieldDialog::cancel));

    fd->fe_ = fe;
    Resource::ref(fe);
    fd->s_ = *fe->text();

    vb->append(layout.flexible(widgets.label(caption)));
    vb->append(layout.vglue(10));
    vb->append(fd->fe_);
    if (extra) {
        vb->append(layout.vglue(10));
        vb->append(extra);
    }
    vb->append(layout.vglue(10));
    vb->append(hb);

    hb->append(layout.hglue(20, fil, 20));
    hb->append(widgets.default_button(accept, new DialogAction(fd, true)));
    hb->append(layout.hglue(5));
    hb->append(widgets.push_button(cancel, new DialogAction(fd, false)));
    hb->append(layout.hglue(20, fil, 20));

    return fd;
}

void FieldDialog::accept(FieldEditor*) {
    dismiss(true);
}
void FieldDialog::cancel(FieldEditor*) {
    dismiss(false);
}


void hoc_boolean_dialog() {
    bool b = false;
    if (auto* const result = neuron::python::methods.try_gui_helper("boolean_dialog", nullptr)) {
        hoc_ret();
        hoc_pushx(neuron::python::methods.object_to_double(*result));
        return;
    }
    IFGUI
    if (ifarg(3)) {
        b = boolean_dialog(gargstr(1), gargstr(2), gargstr(3));
    } else {
        b = boolean_dialog(gargstr(1), "Yes", "No");
    }
    ENDGUI
    hoc_ret();
    hoc_pushx(double(b));
}
void hoc_continue_dialog() {
    TRY_GUI_REDIRECT_DOUBLE("continue_dialog", NULL);
    IFGUI
    continue_dialog(gargstr(1));
    ENDGUI
    hoc_ret();
    hoc_pushx(1.);
}
void hoc_string_dialog() {
    TRY_GUI_REDIRECT_DOUBLE_SEND_STRREF("string_dialog", NULL);
    bool b = false;
    IFGUI
    char buf[256];
    Sprintf(buf, "%s", gargstr(2));
    b = str_chooser(gargstr(1), buf);
    if (b) {
        hoc_assign_str(hoc_pgargstr(2), buf);
    }
    ENDGUI
    hoc_ret();
    hoc_pushx(double(b));
}


/*static*/ class LabelChooserAction: public Action {
  public:
    LabelChooserAction(GLabel*);
    virtual ~LabelChooserAction();
    void state(TelltaleState*);
    virtual void execute();

  private:
    TelltaleState* ts_;
    GLabel* gl_;
};

bool Graph::label_chooser(const char* caption, char* buf, GLabel* gl, Coord x, Coord y) {
    WidgetKit& k = *WidgetKit::instance();
    LayoutKit& l = *LayoutKit::instance();
    Style* style = new Style(k.style());
    style->attribute("caption", caption);
    bool ok;
    LabelChooserAction* lca = new LabelChooserAction(gl);
    Button* b = k.check_box("vfixed", lca);
    lca->state(b->state());

    FieldDialog* d = FieldDialog::field_dialog_instance(buf, style, b);

    d->ref();
    ok = oc_post_dialog(d, x, y);
    if (ok) {
        strcpy(buf, d->text()->string());
    }
    d->unref();
    return ok;
}

LabelChooserAction::LabelChooserAction(GLabel* gl) {
    gl_ = gl;
    gl->ref();
    ts_ = NULL;
}
LabelChooserAction::~LabelChooserAction() {
    gl_->unref();
    ts_->unref();
}
void LabelChooserAction::state(TelltaleState* t) {
    t->ref();
    ts_ = t;
    if (gl_->fixed()) {
        ts_->set(TelltaleState::is_chosen, false);
    } else {
        ts_->set(TelltaleState::is_chosen, true);
    }
}

void LabelChooserAction::execute() {
    if (ts_->test(TelltaleState::is_chosen)) {
        if (gl_->fixed()) {
            gl_->vfixed(gl_->scale());
        }
    } else {
        if (!gl_->fixed()) {
            gl_->fixed(gl_->scale());
        }
    }
}

bool OcGlyph::dialog_dismiss(bool accept) {
    if (d_) {
        d_->dismiss(accept);
        return true;
    }
    return false;
}

bool OcGlyph::dialog(const char* label, const char* accept, const char* cancel) {
    WidgetKit& k = *WidgetKit::instance();
    LayoutKit& l = *LayoutKit::instance();
    PolyGlyph* vbox = l.vbox();
    bool ok;
    d_ = new OcGlyphDialog(k.outset_frame(l.margin(vbox, 5)), Session::instance()->style());
    d_->ref();
    vbox->append(l.hcenter(l.hflexible(l.margin(k.label(label), 10), fil, 0)));
    vbox->append(l.hcenter(this));
    vbox->append(l.hcenter(l.hflexible(l.hbox(k.push_button(accept, new DialogAction(d_, true)),
                                              l.hspace(10),
                                              k.push_button(cancel, new DialogAction(d_, false))),
                                       fil,
                                       0)));
    handle_old_focus();
    ok = oc_post_dialog(d_, 400., 400.);
    handle_old_focus();
    d_->unref();
    d_ = NULL;
    return ok;
}

OcGlyphDialog::OcGlyphDialog(Glyph* g, Style* s)
    : Dialog(g, s) {}
OcGlyphDialog::~OcGlyphDialog() {}
void OcGlyphDialog::pick(Canvas* c, const Allocation& a, int depth, Hit& h) {
    const Event* e = h.event();
    EventType t = (e == NULL) ? Event::undefined : e->type();
    switch (t) {
    case Event::key:
        if (e && inside(*e)) {
            body()->pick(c, a, depth + 1, h);
        }
        break;
    default:
        Dialog::pick(c, a, depth, h);
    }
}
#endif
