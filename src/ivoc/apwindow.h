#ifndef dismiswin_h
#define dismiswin_h

#include <string>

#include <InterViews/window.h>

#include <InterViews/action.h>
#include <InterViews/handler.h>
#include <InterViews/observe.h>
#include <OS/string.h>

class Menu;
class MenuItem;
class OcGlyph;
class DismissableWindow;
class TopLevelWindow;
class OcGlyphContainer;
class PolyGlyph;

// action for dismissing
class WinDismiss: public Handler {  // dismiss a Window
  public:
    WinDismiss(DismissableWindow*);
    virtual ~WinDismiss();
    virtual void execute();  // this can be replaced
    virtual bool event(Event&);
    static void dismiss_defer();

  protected:
    DismissableWindow* win_;

  private:
    static DismissableWindow* win_defer_;
    static DismissableWindow* win_defer_longer_;
};

// Can be dismissed by window manager without quitting.
// The style determines dynamically if this is transient or toplevel
class DismissableWindow: public TransientWindow {
  public:
    DismissableWindow(Glyph*, bool force_menubar = false);
    virtual ~DismissableWindow();
    virtual void dismiss();
    virtual const char* name() const;
    virtual void name(const char*);
    virtual void replace_dismiss_action(WinDismiss*);
    virtual Glyph* glyph() const {
        return glyph_;
    }
    virtual void configure();
    virtual void set_attributes();
    MenuItem* append_menubar(const char*);  // return NULL if no dismiss menubar

    static bool is_transient() {
        return is_transient_;
    }

  private:
    Glyph* glyph_;
    WinDismiss* wd_;
    Action* dbutton_;
    static bool is_transient_;
    Menu* menubar_;
};


// If you want to place a screen window onto a piece of paper
class PrintableWindow: public DismissableWindow, public Observable {
  public:
    PrintableWindow(OcGlyph*);
    virtual ~PrintableWindow();
    virtual void map();
    virtual void unmap();
    virtual void hide();
    virtual bool receive(const Event&);
    virtual void reconfigured();
    // The glyph the user actually wants printed
    virtual Glyph* print_glyph();
    // for size of what actually gets printed
    virtual Coord left_pw() const;
    virtual Coord bottom_pw() const;
    virtual Coord width_pw() const;
    virtual Coord height_pw() const;
    void type(const char*);
    const char* type() const;
    static OcGlyphContainer* intercept(OcGlyphContainer*);  // instead of window put in a box
    virtual void map_notify();
    virtual void unmap_notify(){};
    virtual Coord save_left() const;    // offset by window decoration
    virtual Coord save_bottom() const;  // see nrn.defaults
    int xleft() const;
    int xtop() const;
    void xplace(int left, int top);  // in x display pixel coords
    void xmove(int left, int top);
    void request_on_resize(bool);
    static PrintableWindow* leader() {
        return leader_;
    }
    static void leader(PrintableWindow* w) {
        leader_ = w;
    }

  protected:
    virtual void default_geometry();

  private:
    std::string type_;
    static OcGlyphContainer* intercept_;
    bool mappable_;
    bool xplace_;
    int xleft_;
    int xtop_;
    static PrintableWindow* leader_;
};

// canvas with hbox and menubar at top and left and right vboxes
// the main glyph area resizes according to the window size.
class StandardWindow: public PrintableWindow {
  public:
    StandardWindow(Glyph* main,
                   Glyph* info = NULL,
                   Menu* m = NULL,
                   Glyph* l = NULL,
                   Glyph* r = NULL);
    virtual ~StandardWindow();
    Menu* menubar();
    Glyph* canvas_glyph();
    Glyph* info();
    Glyph* lbox();
    Glyph* rbox();

  private:
    Menu* m_;
    Glyph* can_;
    Glyph *info_, *l_, *r_;
};

class PWMImpl;

class PrintableWindowManager: public Observer {
  public:
    PrintableWindowManager();
    virtual ~PrintableWindowManager();
    void psfilter(const char* filename);
    void xplace(int, int, bool map = true);
    static PrintableWindowManager* current();

    void append(PrintableWindow*);
    void remove(PrintableWindow*);
    void reconfigured(PrintableWindow*);
    void doprint();

    virtual void update(Observable*);
    virtual void disconnect(Observable*);

  public:
    PWMImpl* pwmi_;

  private:
    static PrintableWindowManager* current_;
};

#endif
