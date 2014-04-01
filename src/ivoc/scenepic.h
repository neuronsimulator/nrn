#ifndef scenepicker_h
#define scenepicker_h

#include <InterViews/hit.h>
#include "ocpicker.h"

/*
 right button invokes a menu.
 selected rubberbands and handlers become associated with left button

 Default menu is new view, zoom, whole scene.
 Default adjust is translate.
 Default select is new view.
*/

class Menu;
class Button;
class MenuItem;
class Action;
class ScenePickerImpl;
class Scene;
class TelltaleGroup;
class DismissableWindow;

class ScenePicker : public StandardPicker {
public:
	ScenePicker(Scene*);
	virtual ~ScenePicker();
	
	MenuItem* add_menu(MenuItem*, Menu* = NULL); // not executable from hoc

	MenuItem* add_radio_menu(const char*, Action*, Menu* = NULL);
	MenuItem* add_radio_menu(const char*, Rubberband*, Action*, int tool=0, Menu* = NULL);
	MenuItem* add_radio_menu(const char*, OcHandler*, int tool=0, Menu* = NULL);
	Button* radio_button(const char*, Action*);
	Button* radio_button(const char*, Rubberband*, Action*, int tool=0);

	MenuItem* add_menu(const char*, Action*, Menu* = NULL);
	MenuItem* add_menu(const char*, MenuItem*, Menu* = NULL);

	void remove_item(const char*);	
	void insert_item(const char*, const char*, MenuItem*);

	virtual void pick_menu(Glyph*, int, Hit&);
	virtual void set_scene_tool(int);
	TelltaleGroup* telltale_group();
	virtual const char* select_name();
	virtual void select_name(const char*);
	virtual void help();
	virtual void exec_item(const char*);
	static DismissableWindow* last_window();
private:
	ScenePickerImpl* spi_;
};

#endif
