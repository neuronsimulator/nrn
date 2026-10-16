#pragma once

#include <vector>

#include <InterViews/resource.h>
#include <InterViews/observe.h>
struct Object;
class OcListBrowser;
struct cTemplate;

class OcList: public Resource, public Observer {
  public:
    OcList(long = 5);
    OcList(const char* template_name);
    virtual ~OcList();
    void append(Object*);
    void prepend(Object*);
    void insert(long, Object*);
    long count();
    void remove(long);
    long index(Object*);
    Object* object(long);
    void remove_all();
    bool refs_items() {
        return ct_ == NULL;
    }

    void create_browser(const char* name, const char* items = NULL, Object* pystract = NULL);
    void create_browser(const char* name, char** pstr, const char* action);
    OcListBrowser* browser();

    virtual void update(Observable*);

  private:
    void oref(Object*);
    void ounref(Object*);

  private:
    std::vector<Object*> oli_;
    OcListBrowser* b_;
    cTemplate* ct_;
};
