#include <../../nrnconf.h>
#if HAVE_IV // to end of file

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

declarePtrList(HandlerList, ButtonHandler);
implementPtrList(HandlerList, ButtonHandler);

StandardPicker::StandardPicker()
{
	ms_ = unknown;
	for (int i=0; i < unknown; ++i) {
		handlers_[i] = new HandlerList(1);
	}
}
StandardPicker::~StandardPicker() {
	for (int i=0; i < unknown; ++i) {
		long cnt = handlers_[i]->count();
		for (long j=0; j < cnt; j++) {
			delete handlers_[i]->item(j);
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
	
	long cnt = handlers_[ms_]->count();
	for (long i=0; i < cnt; ++i) {
		ButtonHandler& b = *handlers_[ms_]->item(i);
		if (b.eb_ == Event::any || b.eb_ == mb_) {
			if (b.handler_) {
				h.target(depth, glyph, 0, b.handler_);
			}else{
				b.rband_->canvas(c);
				h.target(depth, glyph, 0, b.rband_);
			}
			return true;
		}
	}
	return false;
}

/* from /interviews/input.c */
void StandardPicker::event(const Event& e) {
	switch (e.type()) {
	case Event::down:
//printf("press\n");
		ms_ = press;
		mb_ = e.pointer_button();
		break;
	case Event::motion:
		if ((ms_ == drag || ms_ == press) &&
		  (e.left_is_down() || e.right_is_down() || e.middle_is_down())
		) {
//printf("drag\n");
			ms_ = drag;
		}else{
//printf("motion\n");
			ms_ = motion;
			mb_ = 0;
		}
		break;
	case Event::up:
//printf("release\n");
		ms_ = release;
		mb_ = e.pointer_button();
		break;
	}
}

void StandardPicker::unbind(int m, EventButton eb) {
	long cnt = handlers_[m]->count();
	long i, j;
	for (i=0, j=0; i < cnt; ++i) {
		ButtonHandler& b = *handlers_[m]->item(j);
		if (b.eb_ == Event::any || b.eb_ == eb) {
			delete handlers_[m]->item(j);
			handlers_[m]->remove(j);
		}else{
			++j;
		}
	}
}
	
void StandardPicker::bind(int m, EventButton eb, OcHandler* h) {
	unbind(m, eb);
	if (h) {
		handlers_[m]->append(new ButtonHandler(eb, h));
	}
}

void StandardPicker::bind_press(EventButton eb, Rubberband* rb) {
	int m = 1;
	unbind(m, eb);
	if (rb) {
		handlers_[m]->append(new ButtonHandler(eb, rb));
	}
}

void StandardPicker::remove_all(EventButton eb) {
	for (int m=0; m < unknown; ++m) {
		unbind(m, eb);
	}
}
#endif
