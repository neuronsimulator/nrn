#include <../../nrnconf.h>
#if HAVE_IV // to end of file

#include <stdio.h>
#include <InterViews/iv3text.h>
#include <InterViews/layout.h>
#include <InterViews/background.h>
#include <InterViews/event.h>
#include <IV-look/kit.h>
#include "ocglyph.h"
#include "classreg.h"
#include "oc2iv.h"
#include "apwindow.h"

class OcText : public Text {
public:
	OcText(unsigned rows = 24, unsigned cols = 80, TextBuffer* buf = NULL);
	virtual ~OcText();
	virtual void keystroke(const Event& event);
};

class OcMLineEditor : public OcGlyph {
public:
	OcMLineEditor(unsigned row, unsigned col, const char* buf = NULL);
	virtual ~OcMLineEditor();
public:
	OcText* txt_;
};

static double map(void* v) {
	OcMLineEditor* e = (OcMLineEditor*)v;
	PrintableWindow* w;
	if (ifarg(3)) {  
		w = e->make_window(float(*getarg(2)), float(*getarg(3)),
			float(*getarg(4)), float(*getarg(5)));
	}else{
		w = e->make_window();
	}       
	if (ifarg(1)) {
		char* name = gargstr(1);
		w->name(name);
	}
	w->map();
	return 0.;
}

static double readonly(void* v) {
	OcMLineEditor* e = (OcMLineEditor*)v;
	if (ifarg(1)) {
		e->txt_->readOnly(int(chkarg(1, 0, 1)));
	}
	return double(e->txt_->readOnly());
}

static const char** v_text(void* v) {
	OcMLineEditor* e = (OcMLineEditor*)v;
	TextBuffer* tb = e->txt_->editBuffer();
	if (ifarg(1)) {
		e->txt_->reset();
		const char* s = gargstr(1);
		tb->Insert(0, s, strlen(s));
	}
	char** p = hoc_temp_charptr();
	*p = (char*)tb->Text();
	return (const char**)p;
}


static Member_func members[] = {
	"readonly", readonly,
	"map", map,	
	0,0
};

static Member_ret_str_func retstr_members[] = {
	"text",	v_text,
	0,0
};

static void* cons(Object*) {
	const char* buf = "";
	unsigned row = 5;
	unsigned col = 30;
	if (ifarg(1)) {
		buf = gargstr(1);
	}
	if (ifarg(2)) {
		row = unsigned(chkarg(2, 1, 1000));
		col = unsigned(chkarg(3, 1, 1000));
	}
	OcMLineEditor* e = new OcMLineEditor(row, col, buf);
	e->ref();
	return (void*)e;
}

static void destruct(void* v) {
	OcMLineEditor* e = (OcMLineEditor*)v;
	if (e->has_window()) {
		e->window()->dismiss();
	}
	e->unref();
}

void TextEditor_reg() {
	class2oc("TextEditor", cons, destruct, members, NULL, NULL, retstr_members);
}

OcMLineEditor::OcMLineEditor(unsigned row, unsigned col, const char* buf) {
	txt_ = new OcText(row, col, new TextBuffer(buf,strlen(buf),1000));
	txt_->ref();
	body(
		new Background(
			txt_,
			WidgetKit::instance()->background()
		)
	);
}
OcMLineEditor::~OcMLineEditor() {
	txt_->unref();
}

OcText::OcText(unsigned rows, unsigned cols, TextBuffer* buf) :
	Text(rows, cols, buf)
{
}

OcText::~OcText() {}

void OcText::keystroke(const Event& e){
  if (readOnly_) {
    return;
  }
  char buffer[8]; // needs to be dynamically adjusted
  int count = e.mapkey(buffer, 8);
  if (count <= 0) {
    return;
  }
	Text::keystroke(e);
}

#endif

