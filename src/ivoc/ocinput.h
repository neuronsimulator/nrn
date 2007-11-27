#ifndef ocinput_h
#define ocinput_h

#include <InterViews/input.h>
#include <InterViews/event.h>
#include <InterViews/handler.h>

class HandlerList;

public StandardInputHandler : public InputHandler {
public:
	StandardInputHandler(Glyph*, Style*);
	virtual ~StandardInputHandler();
	
	virtual void bind_select(Handler* h) {bind_press(Event::left, h);}
	virtual void bind_adjust(Handler* h) {bind_press(Event::middle, h);}
	virtual void bind_menu(Handler* h) {bind_press(Event::right, h);}

	virtual void move(const Event& e) { mouse(0, e); }
	virtual void press(const Event& e) { mouse(1, e); }
	virtual void drag(const Event& e) { mouse(2, e); }
	virtual void release(const Event& e) { mouse(3, e); }
	void mouse(int, const Event&);

	void bind_move(EventButton eb, Handler* h) {bind(0, eb, h);}
	void bind_press(EventButton eb, Handler* h) {bind(0, eb, h);}
	void bind_drag(EventButton eb, Handler* h) {bind(0, eb, h);}
	void bind_release(EventButton eb, Handler* h) {bind(0, eb, h);}
	void bind(int, EventButton eb, Handler* h) {bind(0, eb, h);}
	void remove_all(EventButton);
private:
	HandlerList* handlers_[4];
};

/*
 * Handler denoted by an object and member function to call on the object.
 * Used the FieldEditorAction as a template
 */
 
#if defined(__STDC__) || defined(__ANSI_CPP__) || defined (WIN32)
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
typedef void (T::*HandlerMemberFunction(T))(Event&); \
class HandlerCallback(T) : public Handler { \
public: \
    HandlerCallback(T)(T*, HandlerMemberFunction(T)); \
    virtual ~HandlerCallback(T)(); \
\
    virtual void event(Event&); \
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
void HandlerCallback(T)::event(Event& e) { \
    (obj_->*func_)(e); \
}

#endif
