#ifndef nrnmenu_h
#define nrnmenu_h

#include "ndatclas.h"
class MechTypeImpl;

class MechanismStandard: public Resource {
  public:
    MechanismStandard(const char*, int vartype);
    virtual ~MechanismStandard();

    void panel(const char* label = NULL);
    void action(const char*, Object* pyact);

    int count();
    const char* name();
    const char* name(int, int&);  // returns array dimension and name

    // from arg (section.node(x) (0 if x < 0) to this
    void in(Section*, double x = -1.);
    void in(Point_process*);
    void in(MechanismStandard*);
    void set(const char*, double val, int arrayindex = 0);

    // from this to segement containing x (uniformly if x < 0)
    void out(Section*, double x = -1.);
    void out(Point_process*);
    void out(MechanismStandard*);
    double get(const char*, int arrayindex = 0);

    void save(const char*, std::ostream*);  // for session files
    NrnProperty* np() {
        return np_;
    }
    Object* msobj_;  // wraps 'this' and used as first arg for pyact_
  private:
    NrnProperty* np_;
    int name_cnt_;
    int offset_;
    int vartype_;
    std::string action_;
    Object* pyact_;
    Symbol** glosym_;
    void mschk(const char*);
};

class MechanismType: public Resource {
  public:
    MechanismType(bool point_process);
    virtual ~MechanismType();
    bool is_point();
    bool is_netcon_target(int);
    bool has_net_event(int);
    bool is_artificial(int);
    bool is_ion();
    void select(const char*);
    const char* selected();
    void insert(Section*);
    void remove(Section*);
    void point_process(Object**);
    void action(const char*, Object* pyact);
    void menu();

    int count();
    int selected_item();
    int internal_type();
    void select(int);

    Point_process* pp_begin();
    Point_process* pp_next();

    Object* mtobj_;  // wraps 'this' and used as first arg for pyact_
  private:
    MechTypeImpl* mti_;
};

#endif
