#include <../../nrnconf.h>
#if HAVE_IV  // to end of file

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

/*
 * FieldSEditor -- simple editor for text fields
 */

#include <Dispatch/dispatcher.h>
#include <Dispatch/iocallback.h>
//#include <IV-look/field.h>
#include "field.h"
#include <IV-look/kit.h>
#include <InterViews/background.h>
#include <InterViews/canvas.h>
#include <InterViews/display.h>
#include <InterViews/font.h>
#include <InterViews/event.h>
#include <InterViews/hit.h>
#include <InterViews/layout.h>
#include <InterViews/printer.h>
#include <InterViews/selection.h>
#include <InterViews/style.h>
#include <InterViews/window.h>
#include <IV-2_6/InterViews/button.h>
#include <IV-2_6/InterViews/painter.h>
#include <IV-2_6/InterViews/sensor.h>
#include <IV-2_6/InterViews/streditor.h>
#include <IV-2_6/InterViews/textdisplay.h>
#include <OS/math.h>
#include <OS/string.h>
#include <stdio.h>

class FieldStringSEditor: public StringEditor {
  public:
    FieldStringSEditor(ButtonState* bs, const char* sample, WidgetKit*, Style*);
    virtual ~FieldStringSEditor();

    virtual void print(Printer*, const Allocation&) const;
    virtual void pick(Canvas*, const Allocation&, int depth, Hit&);

    void press(const Event&);
    void drag(const Event&);
    void release(const Event&);
    bool keystroke(const Event&);
    void cursor_on();
    void cursor_off();
    void focus_in();
    void focus_out();
    void cut(SelectionManager*);
    void paste(SelectionManager*);

    void Select(int pos);
    void Select(int left, int right);
    void selection(int& start, int& index);

  protected:
    virtual void Reconfig();

  private:
    WidgetKit* kit_;
    Style* style_;
    int start_;
    int index_;
    int origin_;
    int width_;

    void do_select(Event&);
    void do_grab_scroll(Event&);
    void do_rate_scroll(Event&);
};

declareSelectionCallback(FieldStringSEditor)
implementSelectionCallback(FieldStringSEditor)

FieldStringSEditor::FieldStringSEditor(ButtonState* bs,
                                       const char* sample,
                                       WidgetKit* kit,
                                       Style* style)
    : StringEditor(bs, sample) {
    kit_ = kit;
    style_ = style;
    Resource::ref(style);
    delete input;
    input = NULL;
    start_ = index_ = -1;
}

FieldStringSEditor::~FieldStringSEditor() {
    Resource::unref(style_);
}

void FieldStringSEditor::Select(int pos) {
    start_ = index_ = pos;
    StringEditor::Select(pos);
}
void FieldStringSEditor::Select(int left, int right) {
    start_ = left;
    index_ = right;
    StringEditor::Select(left, right);
}
void FieldStringSEditor::selection(int& start, int& index) {
    start = start_;
    index = index_;
}

void FieldStringSEditor::print(Printer* p, const Allocation& a) const {
    const Font* f = output->GetFont();
    const Color* fg = output->GetFgColor();
    FontBoundingBox b;
    f->font_bbox(b);
    Coord x = a.left(), y = a.bottom() + b.font_descent();
    FieldStringSEditor* e = (FieldStringSEditor*) this;
    for (const char* s = e->Text(); *s != '\0'; s++) {
        Coord w = f->width(*s);
        p->character(f, *s, w, fg, x, y);
        x += w;
    }
}

void FieldStringSEditor::pick(Canvas*, const Allocation& a, int depth, Hit& h) {
    const Event* ep = h.event();
    if (ep != NULL && h.left() < a.right() && h.right() >= a.left() && h.bottom() < a.top() &&
        h.top() >= a.bottom()) {
        h.target(depth, this, 0);
    }
}

void FieldStringSEditor::press(const Event& event) {
    Event e;
    display->Draw(output, canvas);
    switch (event.pointer_button()) {
    case Event::left:
    case Event::middle:
    case Event::right:
        origin_ = display->Left(0, 0);
        width_ = display->Width();
        Poll(e);
        start_ = display->LineIndex(0, e.x);
        do_select(e);
        break;
#if 0
    case Event::middle:
	do_grab_scroll(e);
	break;
    case Event::right:
	do_rate_scroll(e);
	break;
#endif
    }
}

void FieldStringSEditor::drag(const Event&) {
    Event e;
    // I have no idea why the event.pointer_button() is 0.
    Poll(e);
    //    if (e.leftmouse)
    { do_select(e); }
}

void FieldStringSEditor::release(const Event& event) {
    Event e;
    switch (event.pointer_button()) {
    case Event::left:
    case Event::middle:
    case Event::right: {
        Poll(e);
        do_select(e);
        SelectionManager* s = e.display()->primary_selection();
        s->own(new SelectionCallback(FieldStringSEditor)(this, &FieldStringSEditor::cut));
    } break;
    }
}

void FieldStringSEditor::do_select(Event& e) {
    if (e.x < 0) {
        origin_ = Math::min(0, origin_ - e.x);
    } else if (e.x > xmax) {
        origin_ = Math::max(xmax - width_, origin_ - (e.x - xmax));
    }
    display->Scroll(0, origin_, ymax);
    index_ = display->LineIndex(0, e.x);
    DoSelect(start_, index_);
}

void FieldStringSEditor::do_grab_scroll(Event& e) {
    Window* w = canvas->window();
    Cursor* c = w->cursor();
    w->cursor(kit_->hand_cursor());
    int origin = display->Left(0, 0);
    int width = display->Width();
    Poll(e);
    int x = e.x;
    do {
        origin += e.x - x;
        origin = Math::min(0, Math::max(Math::min(0, xmax - width), origin));
        display->Scroll(0, origin, ymax);
        x = e.x;
        Poll(e);
    } while (e.middlemouse);
    w->cursor(c);
}

void FieldStringSEditor::do_rate_scroll(Event& e) {
    Window* w = canvas->window();
    Cursor* c = w->cursor();
    WidgetKit& kit = *kit_;
    Cursor* left = kit.lfast_cursor();
    Cursor* right = kit.rfast_cursor();
    int origin = display->Left(0, 0);
    int width = display->Width();
    Poll(e);
    int x = e.x;
    do {
        origin += x - e.x;
        origin = Math::min(0, Math::max(Math::min(0, xmax - width), origin));
        display->Scroll(0, origin, ymax);
        if (e.x - x < 0) {
            w->cursor(left);
        } else {
            w->cursor(right);
        }
        Poll(e);
    } while (e.rightmouse);
    w->cursor(c);
}

bool FieldStringSEditor::keystroke(const Event& e) {
    char c;
    return e.mapkey(&c, 1) != 0 && HandleChar(c) && c == '\t';
}

void FieldStringSEditor::cursor_on() {
    if (canvas != NULL) {
        display->CaretStyle(BarCaret);
    }
}

void FieldStringSEditor::cursor_off() {
    if (canvas != NULL) {
        display->CaretStyle(NoCaret);
    }
}

void FieldStringSEditor::focus_in() {}
void FieldStringSEditor::focus_out() {}

void FieldStringSEditor::cut(SelectionManager* s) {
    //    s->put_value(Text() + start_, index_ - start_);
    int st = Math::min(start_, index_);
    int i = Math::max(index_, start_);
    s->put_value(Text() + st, i - st);
}

void FieldStringSEditor::paste(SelectionManager*) {
    /* unimplemented */
}

void FieldStringSEditor::Reconfig() {
    kit_->push_style();
    kit_->style(style_);
    Painter* p = new Painter(output);
    p->SetColors(kit_->foreground(), kit_->background());
    p->SetFont(kit_->font());
    Resource::unref(output);
    output = p;
    StringEditor::Reconfig();
    kit_->pop_style();
}

class FieldSButton: public ButtonState {
  public:
    FieldSButton(FieldSEditor*, FieldSEditorAction*);
    virtual ~FieldSButton();

    virtual void Notify();

  private:
    FieldSEditor* editor_;
    FieldSEditorAction* action_;
};

class FieldSEditorImpl {
  private:
    friend class FieldSEditor;

    WidgetKit* kit_;
    FieldStringSEditor* editor_;
    FieldSButton* bs_;
    String text_;
    bool cursor_is_on_;
    IOHandler* blink_handler_;
    long flash_rate_;

    void build(FieldSEditor*, const char*, FieldSEditorAction*);
    void blink_cursor(long, long);
    void stop_blinking();
};

declareIOCallback(FieldSEditorImpl)
implementIOCallback(FieldSEditorImpl)

FieldSEditor::FieldSEditor(const String& sample,
                           WidgetKit* kit,
                           Style* s,
                           FieldSEditorAction* action)
    : InputHandler(NULL, s) {
    impl_ = new FieldSEditorImpl;
    impl_->kit_ = kit;
    NullTerminatedString ns(sample);
    impl_->build(this, ns.string(), action);
}

FieldSEditor::~FieldSEditor() {
    FieldSEditorImpl* i = impl_;
    i->stop_blinking();
    Resource::unref(i->editor_);
    Resource::unref(i->bs_);
    delete i->blink_handler_;
    delete i;
}

void FieldSEditor::undraw() {
    FieldSEditorImpl& f = *impl_;
    f.stop_blinking();
    InputHandler::undraw();
}

void FieldSEditor::press(const Event& e) {
    impl_->editor_->press(e);
}

void FieldSEditor::drag(const Event& e) {
    impl_->editor_->drag(e);
}
void FieldSEditor::release(const Event& e) {
    impl_->editor_->release(e);
}

void FieldSEditor::keystroke(const Event& e) {
    FieldSEditorImpl& f = *impl_;
    if (f.editor_->keystroke(e)) {
        select(text()->length());
        next_focus();
    }
}

InputHandler* FieldSEditor::focus_in() {
    FieldSEditorImpl& f = *impl_;
    f.blink_cursor(0, 0);
    f.editor_->focus_in();
    return InputHandler::focus_in();
}

void FieldSEditor::focus_out() {
    FieldSEditorImpl& f = *impl_;
    f.stop_blinking();
    f.editor_->cursor_off();
    f.editor_->focus_out();
    InputHandler::focus_out();
}

void FieldSEditor::field(const char* str) {
    impl_->editor_->Message(str);
}

void FieldSEditor::field(const String& s) {
    NullTerminatedString ns(s);
    impl_->editor_->Message(ns.string());
}

void FieldSEditor::select(int pos) {
    impl_->editor_->Select(pos);
}

void FieldSEditor::select(int l, int r) {
    impl_->editor_->Select(l, r);
}

void FieldSEditor::selection(int& start, int& index) const {
    impl_->editor_->selection(start, index);
}

void FieldSEditor::edit() {
    impl_->editor_->Edit();
}

void FieldSEditor::edit(const char* str, int left, int right) {
    impl_->editor_->Edit(str, left, right);
}

void FieldSEditor::edit(const String& str, int left, int right) {
    NullTerminatedString ns(str);
    impl_->editor_->Edit(ns.string(), left, right);
}

const String* FieldSEditor::text() const {
    impl_->text_ = String(impl_->editor_->Text());
    return &impl_->text_;
}

/** class FieldSEditorImpl **/

void FieldSEditorImpl::build(FieldSEditor* e, const char* str, FieldSEditorAction* a) {
    WidgetKit& kit = *kit_;
    kit.begin_style("FieldEditor");
    Style* s = kit.style();
    bs_ = new FieldSButton(e, a);
    editor_ = new FieldStringSEditor(bs_, str, kit_, s);
    Glyph* g = editor_;
    if (s->value_is_on("beveled")) {
        g = kit.inset_frame(
            new Background(LayoutKit::instance()->h_margin(editor_, 2.0), kit.background()));
    }
    e->body(g);
    cursor_is_on_ = false;
    blink_handler_ = new IOCallback(FieldSEditorImpl)(this, &FieldSEditorImpl::blink_cursor);
    float sec = 0.5;
    s->find_attribute("cursorFlashRate", sec);
    flash_rate_ = long(sec * 1000000);
    kit.end_style();
}

void FieldSEditorImpl::blink_cursor(long, long) {
    if (cursor_is_on_) {
        editor_->cursor_off();
        cursor_is_on_ = false;
    } else {
        editor_->cursor_on();
        cursor_is_on_ = true;
    }
    if (flash_rate_ > 10) {
        Dispatcher::instance().startTimer(0, flash_rate_, blink_handler_);
    }
}

void FieldSEditorImpl::stop_blinking() {
    Dispatcher::instance().stopTimer(blink_handler_);
    editor_->cursor_off();
    cursor_is_on_ = false;
}

/** class FieldSButton **/

FieldSButton::FieldSButton(FieldSEditor* editor, FieldSEditorAction* action) {
    editor_ = editor;
    action_ = action;
    Resource::ref(action_);
}

FieldSButton::~FieldSButton() {
    Resource::unref(action_);
}

/*
 * We need to reset the button state's value so that we'll get
 * notified again.  If we call SetValue, then it will call
 * Notify again!  Alas, we must access the protected value member (sigh).
 */

void FieldSButton::Notify() {
    int v;
    GetValue(v);
    value = 0;
    if (action_ != NULL) {
        switch (v) {
        case '\r':
            action_->accept(editor_);
            break;
        case /*  ^G */ '\007':
        case /* Esc */ '\033':
            action_->cancel(editor_);
            break;
        default:
            break;
        }
    }
}

/** class FieldSEditorAction **/

FieldSEditorAction::FieldSEditorAction() {}
FieldSEditorAction::~FieldSEditorAction() {}
void FieldSEditorAction::accept(FieldSEditor*) {}
void FieldSEditorAction::cancel(FieldSEditor*) {}
#endif
