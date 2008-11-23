#ifndef xmenu_h
#define xmenu_h

#include <InterViews/window.h>
#include <InterViews/box.h>
#include <InterViews/event.h>
#include <OS/list.h>
#include <OS/string.h>
#include <IV-look/kit.h>
#include <IV-look/stepper.h>
#include <IV-look/dialogs.h>
#if defined(WIN32) || defined(MAC) || defined(CYGWIN)
#define UseFieldEditor 1
#else
#define UseFieldEditor 0 // Use the FieldSEditor
#endif
#if UseFieldEditor
#include <IV-look/field.h>
#define FieldSEditor FieldEditor
#else
#include "field.h"
#endif
#include "ocglyph.h"
#include "apwindow.h"
#include "ivocconf.h"

class HocPanel; // panel is a vbox with menus, buttons, value editors, etc.
class HocMenu; // popup menu panel item
class HocAction; // button/menuItem action
class HocValEditor; // field editor with button
class HocValAction; // knows what to do when value editor or associated button pressed.
class HocItem; // for printing
class HocDataPaths;
class HocCommand;
class HocVarLabel;
class HocUpdateItem;
class Patch;
class BoundedValue;
class HocRadioAction;
class StepperMenu;
class ValEdLabel;
class ScenePicker;
struct HocSymExtension;

declarePtrList(HocUpdateItemList, HocUpdateItem)
declarePtrList(HocItemList, HocItem)
declarePtrList(HocPanelList, HocPanel)

class HocPanel : public OcGlyph {
public:
	HocPanel(const char* name, boolean horizontal=false);
	virtual ~HocPanel();
	virtual void map_window(int scroll = -1); // -1 leave up to panel_scroll attribute

	void pushButton(const char* name, const char* action, boolean activate = false, Object* pyact = 0);
	void stateButton(double *pd, const char* name, const char* action, int style, Object* pyvar = nil, Object* pyact = nil);
	HocMenu* menu(const char* name, boolean add2menubar = false);
	MenuItem* menuItem(const char* name, const char* action, boolean activate = false, Object* pyact = 0);
	MenuItem* menuStateItem(double *pd, const char* name, const char* action, Object* pyvar = nil, Object* pyact = nil);
	void valueEd(const char* prompt, const char* variable, const char* action=0,
		boolean canrun=false, double* pd=nil, boolean deflt=false,
		boolean keep_updated=false, HocSymExtension* extra=nil,
		Object* pyvar=nil, Object* pyact=nil);
	void valueEd(const char* prompt, Object* pyvar, Object* pyact=0,
		boolean canrun=false, boolean deflt=false,
		boolean keep_updated=false);

// ZFM added vert
	void slider(double*, float low = 0, float high = 100,
		float resolution = 1, int steps = 10,
		const char* send_cmd = nil, boolean vert = false,
		boolean slow = false, Object* pyvar=nil, Object* pysend=nil);
	virtual void write(ostream&);
	virtual void save(ostream&);
	virtual HocItem* hoc_item();
	void label(const char*);
	void var_label(char**, Object* pyvar = nil);
	PolyGlyph* box();
	const char* getName();
	void itemAppend(const char*);
	void notifyHocValue();
	void check_valid_pointers(void*, int);
	Coord left_, bottom_; // write by makeTray read by dissolve
	static void save_all(ostream&);
	void data_path(HocDataPaths*, boolean);
	void item_append(HocItem*);
#if MAC
	void mac_menubar();
	void mac_menubar(int&, int&, int); // recurse per menu through list
#endif
	static void keep_updated();
	static void keep_updated(HocUpdateItem*, boolean);
	static void paneltool(const char* name, const char* procname, const char* action, ScenePicker*, Object* pycallback = nil, Object* pyselact = nil);
	static void update_ptrs();
private:
	PolyGlyph* box_;
	HocUpdateItemList elist_;
	HocItemList ilist_;
	static HocUpdateItemList* update_list_;
	boolean horizontal_;
	InputHandler* ih_;
};

class HocItem : public Resource {
public:
	HocItem(const char*, HocItem* parent = nil);
	virtual ~HocItem();
	virtual void write(ostream&);
	const char* getStr();
	virtual void help(const char* childpath = nil);
	virtual void help_parent(HocItem*);
#if MAC
	virtual int mac_menubar(int&, int, int);
#endif
private:
	CopyString str_;
	HocItem* help_parent_;
};

class HocPushButton : public HocItem {
public:
	HocPushButton(const char*, HocAction*, HocItem* parent = nil);
	virtual ~HocPushButton();
	virtual void write(ostream&);
#if MAC
	virtual int mac_menubar(int&, int, int);
#endif
private:
	HocAction* a_;	
};

class HocRadioButton : public HocItem {
public:
	HocRadioButton(const char*, HocRadioAction*, HocItem* parent = nil);
	virtual ~HocRadioButton();
	virtual void write(ostream&);
#if MAC
	virtual int mac_menubar(int&, int, int);
#endif
private:
	HocRadioAction* a_;	
};

class HocMenu : public HocItem {
public:
	HocMenu(const char*, Menu*, MenuItem*, HocItem* parent = nil, boolean add2menubar = false);
	virtual ~HocMenu();
	virtual void write(ostream&);
	virtual Menu* menu() { return menu_;}
	virtual MenuItem* item() { return mi_; }
#if MAC
	virtual int mac_menubar(int&, int, int);
#endif
private:
	MenuItem* mi_;
	Menu* menu_;
	boolean add2menubar_;
};

class HocUpdateItem : public HocItem {
public:
	HocUpdateItem(const char*, HocItem* parent = nil);
	virtual ~HocUpdateItem();
	virtual void update_hoc_item();
	virtual void check_pointer(void*, int vector_size);
	virtual void data_path(HocDataPaths*, boolean);
	virtual void update_ptrs(){}
	void update_ptrs_helper(double**);
};

class HocLabel : public HocItem {
public:
	HocLabel(const char*);
	virtual ~HocLabel();
	virtual void write(ostream&);
};

class HocVarLabel : public HocUpdateItem {
public:
	HocVarLabel(char**, PolyGlyph*, Object* pyvar = nil);
	virtual ~HocVarLabel();
	virtual void write(ostream&);
	virtual void update_hoc_item();
	virtual void check_pointer(void*, int);
	virtual void data_path(HocDataPaths*, boolean);
private:
	Patch* p_;
	char** cpp_;
	char* cp_;
	CopyString* variable_;
	Object* pyvar_;
};

class HocAction : public Action {
public:
	HocAction(const char* action, Object* pyact = nil);
	virtual ~HocAction();
	virtual void execute();
	const char* name() const;
	virtual void help();
	void hoc_item(HocItem*);
private:
	HocCommand* action_;
	HocItem* hi_;
};

class HocMenuAction : public HocAction {
public:
	HocMenuAction(const char* action, HocMenu*);
	virtual ~HocMenuAction();
	virtual void execute();
private:
	HocMenu* hm_;
	HocPanel* hp_; // a temporary. hm_ is not part of this panel
};	

class HocEditorForItem : public FieldSEditor {
public:
	HocEditorForItem(HocValEditor*, HocValAction*);
	virtual ~HocEditorForItem();

	virtual void keystroke(const Event&);
	virtual void press(const Event&);
	virtual void drag(const Event&);
	virtual void release(const Event&);
	virtual void val_inc(const Event&);
	
	virtual InputHandler* focus_in();
	virtual void focus_out();
private:
	HocValEditor* hve_;
	Coord y_;
	int index_;
	EventButton b_;
};

class HocValStepper : public Stepper {
public:
	static HocValStepper* instance(HocValEditor*);
	HocValStepper(HocValEditor*, Glyph*, Style*, TelltaleState*);
	virtual ~HocValStepper();
	
	virtual void press(const Event&);
	virtual void release(const Event&);
	virtual void menu_up(Event&);
	void default_inc(boolean, double);
	double default_inc();
	static StepperMenu* menu() { return menu_;}
protected:
	virtual void adjust();
private:
	void left();
	void middle();
	void right();
private:
	boolean geometric_;
	int steps_;
	float default_inc_;
	float inc_;
	HocValEditor* hve_;
	static StepperMenu* menu_;
};

class HocValEditor : public HocUpdateItem {
public:
	HocValEditor(const char* name, const char* variable, ValEdLabel*,
		HocValAction*, double* pd=0, boolean canrun=false,
		HocItem* parent = nil, Object* pvar = nil);
	virtual ~HocValEditor();
	FieldSEditor* field_editor() { return fe_; }
	virtual Stepper* stepper() { return nil; }
	virtual void update_hoc_item();
	void evalField();
	void audit();
	virtual void updateField();
	virtual void write(ostream&);
	virtual void data_path(HocDataPaths*, boolean);
	virtual void check_pointer(void*, int);
	virtual void print(Printer*, const Allocation&)const;
	virtual int hoc_default_val_editor() {return 0;}
	void set_val(double);
	double get_val();
	virtual void exec_action();
	const char* variable() const;
	virtual void setlimits(float*);
	virtual double domain_limits(double);
	boolean active() { return active_;}
	virtual void update_ptrs();
private:
	friend class HocEditorForItem;
	friend class HocValStepper;
	HocEditorForItem* fe_;
	boolean active_;
	boolean canrun_;
	HocAction* action_;
	CopyString* variable_;
	double* pval_;
	ValEdLabel* prompt_;
	float* domain_limits_;
	Object* pyvar_;
};

class HocDefaultValEditor : public HocValEditor {
public:
	HocDefaultValEditor(const char* name, const char* variable, ValEdLabel*,
		HocValAction*, double* pd=0, boolean canrun=false,
		HocItem* parent = nil, Object* pyvar=nil);
	virtual ~HocDefaultValEditor();
	virtual Stepper* stepper() { return vs_; }
	virtual void updateField();
	virtual int hoc_default_val_editor() {return 1;}
	void deflt(double);
	void def_action();
	void def_change(float, float);
	Button* checkbox() { return checkbox_; }
private:
	Button* checkbox_;	// not your normal checkbox. see xmenu.c
	double deflt_;
	double most_recent_;
	HocValStepper* vs_;
};

class HocValEditorKeepUpdated : public HocValEditor {
public:
	HocValEditorKeepUpdated(const char* name, const char* variable, ValEdLabel*,
		HocValAction*, double*, HocItem* parent = nil, Object* pyvar=nil);
	virtual ~HocValEditorKeepUpdated();
	virtual void write(ostream&);
};

class HocValAction : public HocAction {
public:
	HocValAction(const char* action, Object* pyact = 0);
	HocValAction(Object* pyaction);
	virtual ~HocValAction();
	void accept(FieldSEditor*);
	void execute();
	void setFieldSEditor(HocValEditor*);
#if UseFieldEditor
	FieldEditorAction* fea(){return fea_;}
#else
	FieldSEditorAction* fea(){return fea_;}
#endif
private:
	HocValEditor* fe_;
#if UseFieldEditor
	FieldEditorAction* fea_;
#else
	FieldSEditorAction* fea_;
#endif
};


// ZFM added vert_
class OcSlider : public HocUpdateItem, public Observer {
public:
	OcSlider(double*, float low, float high,
		float resolution, int nsteps,
		const char* send_cmd, boolean vert,
		boolean slow = false, Object* pyvar=nil, Object* pysend=nil);
	virtual ~OcSlider();
	virtual void write(ostream&);

	Adjustable* adjustable();

	virtual void update(Observable*);

	virtual void update_hoc_item();
	virtual void check_pointer(void*, int vector_size);
	virtual void data_path(HocDataPaths*, boolean);
	virtual double slider_val();
	virtual void update_ptrs();
private:
	void audit();
private:
	float resolution_;
	BoundedValue* bv_;
	HocCommand* send_;
	double *pval_;
	Object* pyvar_;
	CopyString* variable_;
	boolean scrolling_;
	boolean vert_;
	boolean slow_;
};



class HocStateButton : public HocUpdateItem, public Observer {
 public:
  HocStateButton(double*, const char*, Button*, HocAction*, int, HocItem* parent = nil, Object* pyvar = nil);
  virtual ~HocStateButton();
  virtual void write(ostream&);

  boolean chosen();
  void button_action();

  virtual void update_hoc_item();
  virtual void check_pointer(void*, int);
  virtual void data_path(HocDataPaths*, boolean);
  virtual void print(Printer*, const Allocation&) const;
  virtual void update_ptrs();
  enum { CHECKBOX,PALETTE };

 private:
  int style_;
  CopyString* variable_;
  CopyString* name_;
  double* pval_;
  Object* pyvar_;
  Button* b_;
  HocAction* action_;
};


class HocStateMenuItem : public HocUpdateItem, public Observer {
 public:
  HocStateMenuItem(double*, const char*, MenuItem*, HocAction*, HocItem* parent = nil, Object* pyvar = nil);
  virtual ~HocStateMenuItem();
  virtual void write(ostream&);

  boolean chosen();
  void button_action();

  virtual void update_hoc_item();
  virtual void check_pointer(void*, int);
  virtual void data_path(HocDataPaths*, boolean);
  virtual void print(Printer*, const Allocation&) const;
  virtual void update_ptrs();

 private:
  CopyString* variable_;
  CopyString* name_;
  double* pval_;
  Object* pyvar_;
  MenuItem* b_;
  HocAction* action_;
};


#endif



