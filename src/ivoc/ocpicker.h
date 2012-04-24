#ifndef ocpicker_h
#define ocpicker_h

#include <InterViews/input.h>
#include <InterViews/event.h>
#include <InterViews/handler.h>
#include "rubband.h"

class HandlerList;
class Canvas;
class Allocation;
class Hit;

/* steer to the right method in response to a mouse action */

class StandardPicker {
public:
	StandardPicker();
	virtual ~StandardPicker();
	
	virtual bool pick(Canvas*, Glyph*, int depth, Hit& h);
	
	void bind_select(Rubberband* rb) {bind_press(Event::left, rb);}
	void bind_adjust(Rubberband* rb) {bind_press(Event::middle, rb);}
	void bind_menu(Rubberband* rb) {bind_press(Event::right, rb);}
	void bind_press(EventButton eb, Rubberband*);

	void bind_select(OcHandler* h) {bind_press(Event::left, h);}
	void bind_adjust(OcHandler* h) {bind_press(Event::middle, h);}
	void bind_menu(OcHandler* h) {bind_press(Event::right, h);}

	void bind_move(OcHandler* h) {bind(0, 0, h);}
	void bind_press(EventButton eb, OcHandler* h) {bind(1, eb, h);}
	void bind_drag(EventButton eb, OcHandler* h) {bind(2, eb, h);}
	void bind_release(EventButton eb, OcHandler* h) {bind(3, eb, h);}
	void bind(int, EventButton eb, OcHandler* h);

	void unbind(int, EventButton);
	void remove_all(EventButton);
private:
	void event(const Event&);
private:
	typedef int State;
	enum {motion, press, drag, release, unknown};
	State ms_;
	EventButton mb_;
	HandlerList* handlers_[unknown];
};

/*
 * Handler denoted by an object and member function to call on the object.
 * Used the FieldEditorAction as a template
 */
 
#if defined(__STDC__) || defined(__ANSI_CPP__)
#define __HandlerCallback(T) T##_HandlerCallback
#define HandlerCallback(T) __HandlerCallback(T)
#define __HandlerMemberFunction(T) T##_HandlerMemberFunction
#define HandlerMemberFunction(T) __HandlerMemberFunction(T)
#else
#define __HandlerCallback(T) T/**/_HandlerCallback
#define HandlerCallback(T) __HandlerCallback(T)
#define __HandlerMemberFunction(T) T/**/_HandlerMemberFunction
#define HandlerMemberFunction(T) __HandlerMemberFunction(T)
#endif

#define declareHandlerCallback(T) \
typedef bool (T::*HandlerMemberFunction(T))(Event&); \
class HandlerCallback(T) : public Handler { \
public: \
    HandlerCallback(T)(T*, HandlerMemberFunction(T)); \
    virtual ~HandlerCallback(T)(); \
\
    virtual bool event(Event&); \
private: \
    T* obj_; \
    HandlerMemberFunction(T) func_; \
};

#define implementHandlerCallback(T) \
HandlerCallback(T)::HandlerCallback(T)( \
    T* obj, HandlerMemberFunction(T) func \
) { \
    obj_ = obj; \
    func_ = func; \
} \
\
HandlerCallback(T)::~HandlerCallback(T)() { } \
\
bool HandlerCallback(T)::event(Event& e) { \
    return (obj_->*func_)(e); \
}

#endif
