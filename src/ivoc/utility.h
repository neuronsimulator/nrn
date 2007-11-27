#ifndef utility_h
#define utility_h

#include <InterViews/dialog.h>
#include <IV-look/field.h>
#include <OS/string.h>
#include <InterViews/handler.h>

#undef PopupMenu

class Window;
class FieldEditor;
class Menu;
class Event;
class MenuItem;
class PopupWindow;
class TelltaleGroup;

class FieldDialog : public Dialog {
public:
	static FieldDialog* field_dialog_instance(const char*, Style*,
		Glyph* extra = nil);
	virtual ~FieldDialog();
	virtual void dismiss(boolean accept);
	const String* text() const {return fe_->text();}
	virtual void keystroke(const Event& e) {fe_->keystroke(e);}
	virtual void accept(FieldEditor*);
	virtual void cancel(FieldEditor*);
	virtual boolean run();
private:
	FieldDialog(Glyph*, Style*);
	FieldEditor* fe_;
	CopyString s_;
};

boolean ok_to_write(const String&, Window* w=nil);
boolean ok_to_write(const char*, Window* w=nil);
boolean ok_to_read(const String&, Window* w=nil);
boolean ok_to_read(const char*, Window* w=nil);
boolean boolean_dialog(const char* label, const char* accept, const char* cancel,
	Window* w=nil, Coord x=400., Coord y=400.);
void continue_dialog(const char* label, Window* w=nil, Coord x=400., Coord y=400.);

boolean str_chooser(const char*, char*, Window* w=nil, Coord x = 400., Coord y = 400.);
boolean var_pair_chooser(const char*, float& x, float& y, Window* w=nil,
	Coord x1=400., Coord y1=400.);

class PopupMenu : public Handler {
public:
	PopupMenu();
	virtual ~PopupMenu();
	virtual boolean event(Event&);
	void append_item(MenuItem*);
	Menu* menu() { return menu_; }
private:
	Menu* menu_;
	PopupWindow* w_;
	boolean grabbed_;
};
	
// makes sure menuitem width is size of menu width
class K {
public:
	static MenuItem* menu_item(const char*);
	static MenuItem* radio_menu_item(TelltaleGroup*, const char*);
	static MenuItem* check_menu_item(const char*);
};

void handle_old_focus();

#endif
