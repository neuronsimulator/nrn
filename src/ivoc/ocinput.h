#ifndef ocinput_h
#define ocinput_h

#include <InterViews/input.h>
#include <InterViews/event.h>
#include <InterViews/handler.h>

class HandlerList;

public
StandardInputHandler: public InputHandler {
  public:
    StandardInputHandler(Glyph*, Style*);
    virtual ~StandardInputHandler();

    virtual void bind_select(Handler * h) {
        bind_press(Event::left, h);
    }
    virtual void bind_adjust(Handler * h) {
        bind_press(Event::middle, h);
    }
    virtual void bind_menu(Handler * h) {
        bind_press(Event::right, h);
    }

    virtual void move(const Event& e) {
        mouse(0, e);
    }
    virtual void press(const Event& e) {
        mouse(1, e);
    }
    virtual void drag(const Event& e) {
        mouse(2, e);
    }
    virtual void release(const Event& e) {
        mouse(3, e);
    }
    void mouse(int, const Event&);

    void bind_move(EventButton eb, Handler * h) {
        bind(0, eb, h);
    }
    void bind_press(EventButton eb, Handler * h) {
        bind(0, eb, h);
    }
    void bind_drag(EventButton eb, Handler * h) {
        bind(0, eb, h);
    }
    void bind_release(EventButton eb, Handler * h) {
        bind(0, eb, h);
    }
    void bind(int, EventButton eb, Handler* h) {
        bind(0, eb, h);
    }
    void remove_all(EventButton);

  private:
    HandlerList* handlers_[4];
};
