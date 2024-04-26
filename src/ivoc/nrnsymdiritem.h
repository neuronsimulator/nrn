#pragma once

// allow communication between src/ivoc/symdir.cpp and src/nrniv/pysecname.cpp

class SymbolItem {
  public:
    SymbolItem(std::string&&, int whole_array = 0);
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
    const std::string& name() const {
        return name_;
    }
    bool is_directory() const;
    int array_index() const {
        return index_;
    }
    int whole_vector();
    int pysec_type_ = 0; /* PYSECOBJ (cell prefix) or PYSECNAME (Section) */
    void* pysec_ = nullptr;    /* Name2Section* or Section* */
  private:
    std::string name_{};
    Symbol* symbol_ = nullptr;
    int index_ = 0;
    Object* ob_ = nullptr;
    int whole_array_ = 0;
};

void nrn_symdir_load_pysec(std::vector<SymbolItem*>& sl, void*);
