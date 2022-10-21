#ifndef nrnsymdiritem_h
#define nrnsymdiritem_h

// allow communication between src/ivoc/symdir.cpp and src/nrniv/pysecname.cpp

#include <OS/list.h>
#include <OS/string.h>

class SymbolItem {
  public:
    SymbolItem(const char*, int whole_array = 0);
    SymbolItem(Symbol*, Objectdata*, int index = 0, int whole_array = 0);
    SymbolItem(Object*);
    ~SymbolItem();
    Symbol* symbol() const {
        return symbol_;
    }
    Object* object() const {
        return ob_;
    }
    void no_object();
    const String& name() const {
        return name_;
    }
    bool is_directory() const;
    int array_index() const {
        return index_;
    }
    int whole_vector();
    int pysec_type_; /* PYSECOBJ (cell prefix) or PYSECNAME (Section) */
    void* pysec_;    /* Name2Section* or Section* */
  private:
    CopyString name_;
    Symbol* symbol_;
    int index_;
    Object* ob_;
    int whole_array_;
};

declarePtrList(SymbolList, SymbolItem);

void nrn_symdir_load_pysec(SymbolList& sl, void*);

#endif
