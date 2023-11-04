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
#if defined(MINGW)
#define UseFieldEditor 1
#else
#define UseFieldEditor 0  // Use the FieldSEditor
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

class HocPanel;      // panel is a vbox with menus, buttons, value editors, etc.
class HocMenu;       // popup menu panel item
class HocAction;     // button/menuItem action
class HocValEditor;  // field editor with button
class HocValAction;  // knows what to do when value editor or associated button pressed.
class HocItem;       // for printing
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


class HocPanel: public OcGlyph {
  public:
    HocPanel(const char* name, bool horizontal = false);
    virtual ~HocPanel();
    virtual void map_window(int scroll = -1);  // -1 leave up to panel_scroll attribute

    void pushButton(const char* name, const char* action, bool activate = false, Object* pyact = 0);
    void stateButton(neuron::container::data_handle<double> pd,
                     const char* name,
                     const char* action,
                     int style,
                     Object* pyvar = NULL,
                     Object* pyact = NULL);
    HocMenu* menu(const char* name, bool add2menubar = false);
    MenuItem* menuItem(const char* name,
                       const char* action,
                       bool activate = false,
                       Object* pyact = 0);
    MenuItem* menuStateItem(neuron::container::data_handle<double> pd,
                            const char* name,
                            const char* action,
                            Object* pyvar = NULL,
                            Object* pyact = NULL);
    void valueEd(const char* prompt,
                 const char* variable,
                 const char* action = 0,
                 bool canrun = false,
                 neuron::container::data_handle<double> pd = {},
                 bool deflt = false,
                 bool keep_updated = false,
                 HocSymExtension* extra = NULL,
                 Object* pyvar = NULL,
                 Object* pyact = NULL);
    void valueEd(const char* prompt,
                 Object* pyvar,
                 Object* pyact = 0,
                 bool canrun = false,
                 bool deflt = false,
                 bool keep_updated = false);

    // ZFM added vert
    void slider(neuron::container::data_handle<double>,
                float low = 0,
                float high = 100,
                float resolution = 1,
                int steps = 10,
                const char* send_cmd = NULL,
                bool vert = false,
                bool slow = false,
                Object* pyvar = NULL,
                Object* pysend = NULL);
    virtual void write(std::ostream&);
    virtual void save(std::ostream&);
    virtual HocItem* hoc_item();
    void label(const char*);
    void var_label(char**, Object* pyvar = NULL);
    PolyGlyph* box();
    const char* getName();
    void itemAppend(const char*);
    void notifyHocValue();
    void check_valid_pointers(void*, int);
    Coord left_, bottom_;  // write by makeTray read by dissolve
    static void save_all(std::ostream&);
    void data_path(HocDataPaths*, bool);
    void item_append(HocItem*);
    static void keep_updated();
    static void keep_updated(HocUpdateItem*, bool);
    static void paneltool(const char* name,
                          const char* procname,
                          const char* action,
                          ScenePicker*,
                          Object* pycallback = NULL,
                          Object* pyselact = NULL);

  private:
    PolyGlyph* box_;
    std::vector<HocUpdateItem*> elist_;
    std::vector<HocItem*> ilist_;
    static std::vector<HocUpdateItem*>* update_list_;
    bool horizontal_;
    InputHandler* ih_;
};

class HocItem: public Resource {
  public:
    HocItem(const char*, HocItem* parent = NULL);
    virtual ~HocItem();
    virtual void write(std::ostream&);
    const char* getStr();
    virtual void help(const char* childpath = NULL);
    virtual void help_parent(HocItem*);

  private:
    CopyString str_;
    HocItem* help_parent_;
};

class HocPushButton: public HocItem {
  public:
    HocPushButton(const char*, HocAction*, HocItem* parent = NULL);
    virtual ~HocPushButton();
    virtual void write(std::ostream&);

  private:
    HocAction* a_;
};

class HocRadioButton: public HocItem {
  public:
    HocRadioButton(const char*, HocRadioAction*, HocItem* parent = NULL);
    virtual ~HocRadioButton();
    virtual void write(std::ostream&);

  private:
    HocRadioAction* a_;
};

class HocMenu: public HocItem {
  public:
    HocMenu(const char*, Menu*, MenuItem*, HocItem* parent = NULL, bool add2menubar = false);
    virtual ~HocMenu();
    virtual void write(std::ostream&);
    virtual Menu* menu() {
        return menu_;
    }
    virtual MenuItem* item() {
        return mi_;
    }

  private:
    MenuItem* mi_;
    Menu* menu_;
    bool add2menubar_;
};

class HocUpdateItem: public HocItem {
  public:
    HocUpdateItem(const char*, HocItem* parent = NULL);
    virtual ~HocUpdateItem();
    virtual void update_hoc_item();
    virtual void check_pointer(void*, int vector_size);
    virtual void data_path(HocDataPaths*, bool);
};

class HocLabel: public HocItem {
  public:
    HocLabel(const char*);
    virtual ~HocLabel();
    virtual void write(std::ostream&);
};

class HocVarLabel: public HocUpdateItem {
  public:
    HocVarLabel(char**, PolyGlyph*, Object* pyvar = NULL);
    virtual ~HocVarLabel();
    virtual void write(std::ostream&);
    virtual void update_hoc_item();
    virtual void check_pointer(void*, int);
    virtual void data_path(HocDataPaths*, bool);

  private:
    Patch* p_;
    char** cpp_;
    char* cp_;
    std::string variable_{};
    Object* pyvar_;
};

class HocAction: public Action {
  public:
    HocAction(const char* action, Object* pyact = NULL);
    virtual ~HocAction();
    virtual void execute();
    const char* name() const;
    virtual void help();
    void hoc_item(HocItem*);

  private:
    HocCommand* action_;
    HocItem* hi_;
};

class HocMenuAction: public HocAction {
  public:
    HocMenuAction(const char* action, Object* pyact, HocMenu*);
    virtual ~HocMenuAction();
    virtual void execute();

  private:
    HocMenu* hm_;
    HocPanel* hp_;  // a temporary. hm_ is not part of this panel
};

class HocEditorForItem: public FieldSEditor {
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
    int index_;
};

class HocValStepper: public Stepper {
  public:
    static HocValStepper* instance(HocValEditor*);
    HocValStepper(HocValEditor*, Glyph*, Style*, TelltaleState*);
    virtual ~HocValStepper();

    virtual void press(const Event&);
    virtual void release(const Event&);
    virtual void menu_up(Event&);
    void default_inc(bool, double);
    double default_inc();
    static StepperMenu* menu() {
        return menu_;
    }

  protected:
    virtual void adjust();

  private:
    void left();
    void middle();
    void right();

  private:
    bool geometric_;
    int steps_;
    float default_inc_;
    float inc_;
    HocValEditor* hve_;
    static StepperMenu* menu_;
};

class HocValEditor: public HocUpdateItem {
  public:
    HocValEditor(const char* name,
                 const char* variable,
                 ValEdLabel*,
                 HocValAction*,
                 neuron::container::data_handle<double> pd = {},
                 bool canrun = false,
                 HocItem* parent = NULL,
                 Object* pvar = NULL);
    virtual ~HocValEditor();
    FieldSEditor* field_editor() {
        return fe_;
    }
    virtual Stepper* stepper() {
        return NULL;
    }
    virtual void update_hoc_item();
    void evalField();
    void audit();
    virtual void updateField();
    virtual void write(std::ostream&);
    virtual void data_path(HocDataPaths*, bool);
    virtual void check_pointer(void*, int);
    virtual void print(Printer*, const Allocation&) const;
    virtual int hoc_default_val_editor() {
        return 0;
    }
    void set_val(double);
    double get_val();
    virtual void exec_action();
    const char* variable() const;
    virtual void setlimits(float*);
    virtual double domain_limits(double);
    bool active() {
        return active_;
    }

  private:
    friend class HocEditorForItem;
    friend class HocValStepper;
    HocEditorForItem* fe_;
    bool active_;
    bool canrun_;
    HocAction* action_;
    std::string variable_{};
    neuron::container::data_handle<double> pval_;
    ValEdLabel* prompt_;
    float* domain_limits_;
    Object* pyvar_;
};

class HocDefaultValEditor: public HocValEditor {
  public:
    HocDefaultValEditor(const char* name,
                        const char* variable,
                        ValEdLabel*,
                        HocValAction*,
                        neuron::container::data_handle<double> pd = {},
                        bool canrun = false,
                        HocItem* parent = NULL,
                        Object* pyvar = NULL);
    virtual ~HocDefaultValEditor();
    virtual Stepper* stepper() {
        return vs_;
    }
    virtual void updateField();
    virtual int hoc_default_val_editor() {
        return 1;
    }
    void deflt(double);
    void def_action();
    void def_change(float, float);
    Button* checkbox() {
        return checkbox_;
    }

  private:
    Button* checkbox_;  // not your normal checkbox. see xmenu.cpp
    double deflt_;
    double most_recent_;
    HocValStepper* vs_;
};

class HocValEditorKeepUpdated: public HocValEditor {
  public:
    HocValEditorKeepUpdated(const char* name,
                            const char* variable,
                            ValEdLabel*,
                            HocValAction*,
                            neuron::container::data_handle<double>,
                            HocItem* parent = NULL,
                            Object* pyvar = NULL);
    virtual ~HocValEditorKeepUpdated();
    virtual void write(std::ostream&);
};

class HocValAction: public HocAction {
  public:
    HocValAction(const char* action, Object* pyact = 0);
    HocValAction(Object* pyaction);
    virtual ~HocValAction();
    void accept(FieldSEditor*);
    void execute();
    void setFieldSEditor(HocValEditor*);
#if UseFieldEditor
    FieldEditorAction* fea() {
        return fea_;
    }
#else
    FieldSEditorAction* fea() {
        return fea_;
    }
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
class OcSlider: public HocUpdateItem, public Observer {
  public:
    OcSlider(neuron::container::data_handle<double>,
             float low,
             float high,
             float resolution,
             int nsteps,
             const char* send_cmd,
             bool vert,
             bool slow = false,
             Object* pyvar = NULL,
             Object* pysend = NULL);
    virtual ~OcSlider();
    virtual void write(std::ostream&);

    Adjustable* adjustable();

    virtual void update(Observable*);

    virtual void update_hoc_item();
    virtual void check_pointer(void*, int vector_size);
    virtual void data_path(HocDataPaths*, bool);
    virtual double slider_val();

  private:
    void audit();

  private:
    float resolution_;
    BoundedValue* bv_;
    HocCommand* send_;
    neuron::container::data_handle<double> pval_;
    Object* pyvar_;
    std::string variable_{};
    bool scrolling_;
    bool vert_;
    bool slow_;
};


class HocStateButton: public HocUpdateItem, public Observer {
  public:
    HocStateButton(neuron::container::data_handle<double>,
                   const char*,
                   Button*,
                   HocAction*,
                   int,
                   HocItem* parent = NULL,
                   Object* pyvar = NULL);
    virtual ~HocStateButton();
    virtual void write(std::ostream&);

    bool chosen();
    void button_action();

    virtual void update_hoc_item();
    virtual void check_pointer(void*, int);
    virtual void data_path(HocDataPaths*, bool);
    virtual void print(Printer*, const Allocation&) const;
    enum { CHECKBOX, PALETTE };

  private:
    int style_;
    std::string variable_{};
    CopyString* name_;
    neuron::container::data_handle<double> pval_;
    Object* pyvar_;
    Button* b_;
    HocAction* action_;
};


class HocStateMenuItem: public HocUpdateItem, public Observer {
  public:
    HocStateMenuItem(neuron::container::data_handle<double>,
                     const char*,
                     MenuItem*,
                     HocAction*,
                     HocItem* parent = NULL,
                     Object* pyvar = NULL);
    virtual ~HocStateMenuItem();
    virtual void write(std::ostream&);

    bool chosen();
    void button_action();

    virtual void update_hoc_item();
    virtual void check_pointer(void*, int);
    virtual void data_path(HocDataPaths*, bool);
    virtual void print(Printer*, const Allocation&) const;

  private:
    std::string variable_{};
    CopyString* name_;
    neuron::container::data_handle<double> pval_;
    Object* pyvar_;
    MenuItem* b_;
    HocAction* action_;
};


#endif
