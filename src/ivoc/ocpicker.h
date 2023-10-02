#ifndef ocpicker_h
#define ocpicker_h

#include <vector>

#include <InterViews/input.h>
#include <InterViews/event.h>
#include <InterViews/handler.h>
#include "rubband.h"

class Canvas;
class Allocation;
class Hit;
class ButtonHandler;

/* steer to the right method in response to a mouse action */

class StandardPicker {
  public:
    StandardPicker();
    virtual ~StandardPicker();

    virtual bool pick(Canvas*, Glyph*, int depth, Hit& h);

    void bind_select(Rubberband* rb) {
        bind_press(Event::left, rb);
    }
    void bind_adjust(Rubberband* rb) {
        bind_press(Event::middle, rb);
    }
    void bind_menu(Rubberband* rb) {
        bind_press(Event::right, rb);
    }
    void bind_press(EventButton eb, Rubberband*);

    void bind_select(OcHandler* h) {
        bind_press(Event::left, h);
    }
    void bind_adjust(OcHandler* h) {
        bind_press(Event::middle, h);
    }
    void bind_menu(OcHandler* h) {
        bind_press(Event::right, h);
    }

    void bind_move(OcHandler* h) {
        bind(0, 0, h);
    }
    void bind_press(EventButton eb, OcHandler* h) {
        bind(1, eb, h);
    }
    void bind_drag(EventButton eb, OcHandler* h) {
        bind(2, eb, h);
    }
    void bind_release(EventButton eb, OcHandler* h) {
        bind(3, eb, h);
    }
    void bind(int, EventButton eb, OcHandler* h);

    void unbind(int, EventButton);
    void remove_all(EventButton);

  private:
    void event(const Event&);

  private:
    typedef int State;
    enum { motion, press, drag, release, unknown };
    State ms_;
    EventButton mb_;
    std::vector<ButtonHandler*>* handlers_[unknown];
};
#endif
