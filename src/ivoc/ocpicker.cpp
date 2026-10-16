#include <../../nrnconf.h>
#if HAVE_IV  // to end of file

#include <algorithm>
#include <OS/list.h>
#include <InterViews/hit.h>
#include <stdio.h>
#include "ocpicker.h"
#include "rubband.h"

/*static*/ class ButtonHandler {
  public:
    ButtonHandler(EventButton, OcHandler*);
    ButtonHandler(EventButton, Rubberband*);
    ~ButtonHandler();
    OcHandler* handler_;
    Rubberband* rband_;
    EventButton eb_;
};

ButtonHandler::ButtonHandler(EventButton eb, OcHandler* a) {
    eb_ = eb;
    handler_ = a;
    rband_ = NULL;
    Resource::ref(a);
}

ButtonHandler::ButtonHandler(EventButton eb, Rubberband* rb) {
    eb_ = eb;
    handler_ = NULL;
    rband_ = rb;
    Resource::ref(rb);
}

ButtonHandler::~ButtonHandler() {
    Resource::unref(handler_);
    Resource::unref(rband_);
}

StandardPicker::StandardPicker() {
    ms_ = unknown;
    for (int i = 0; i < unknown; ++i) {
        handlers_[i] = new std::vector<ButtonHandler*>();
    }
}
StandardPicker::~StandardPicker() {
    for (int i = 0; i < unknown; ++i) {
        for (auto& item: *handlers_[i]) {
            delete item;
        }
        delete handlers_[i];
    }
}
bool StandardPicker::pick(Canvas* c, Glyph* glyph, int depth, Hit& h) {
    if (!h.event()) {
        return false;
    }
    const Event& e = *h.event();
    if (e.grabber()) {
        h.target(depth, glyph, 0, e.grabber());
        return true;
    }
    event(e);

    for (const auto& b: *handlers_[ms_]) {
        if (b->eb_ == Event::any || b->eb_ == mb_) {
            if (b->handler_) {
                h.target(depth, glyph, 0, b->handler_);
            } else {
                b->rband_->canvas(c);
                h.target(depth, glyph, 0, b->rband_);
            }
            return true;
        }
    }
    return false;
}

/* from /interviews/input.cpp */
void StandardPicker::event(const Event& e) {
    switch (e.type()) {
    case Event::down:
        // printf("press\n");
        ms_ = press;
        mb_ = e.pointer_button();
        break;
    case Event::motion:
        if ((ms_ == drag || ms_ == press) &&
            (e.left_is_down() || e.right_is_down() || e.middle_is_down())) {
            // printf("drag\n");
            ms_ = drag;
        } else {
            // printf("motion\n");
            ms_ = motion;
            mb_ = 0;
        }
        break;
    case Event::up:
        // printf("release\n");
        ms_ = release;
        mb_ = e.pointer_button();
        break;
    }
}

void StandardPicker::unbind(int m, EventButton eb) {
    for (auto& b: *handlers_[m]) {
        if (b->eb_ == Event::any || b->eb_ == eb) {
            delete b;
            b = nullptr;
        }
    }
    handlers_[m]->erase(std::remove(handlers_[m]->begin(), handlers_[m]->end(), nullptr),
                        handlers_[m]->end());
}

void StandardPicker::bind(int m, EventButton eb, OcHandler* h) {
    unbind(m, eb);
    if (h) {
        handlers_[m]->push_back(new ButtonHandler(eb, h));
    }
}

void StandardPicker::bind_press(EventButton eb, Rubberband* rb) {
    int m = 1;
    unbind(m, eb);
    if (rb) {
        handlers_[m]->push_back(new ButtonHandler(eb, rb));
    }
}

void StandardPicker::remove_all(EventButton eb) {
    for (int m = 0; m < unknown; ++m) {
        unbind(m, eb);
    }
}
#endif
