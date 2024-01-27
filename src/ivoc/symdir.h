#ifndef symdir_h
#define symdir_h

#include <InterViews/resource.h>
#include <map>

struct Object;
class SymDirectoryImpl;
struct Symbol;

class IvocAliases {
  public:
    IvocAliases(Object*);
    virtual ~IvocAliases();
    Symbol* lookup(const char*);
    Symbol* install(const char*);
    void remove(Symbol*);
    int count();
    Symbol* symbol(int);

    Object* ob_;  // not referenced
    std::map<std::string, Symbol*> symtab_;
};

/* List of Symbols considered as a directory */

class SymDirectory: public Resource {
  public:
    SymDirectory(const std::string& parent_path,
                 Object* parent_object,
                 Symbol*,
                 int array_index = 0,
                 int node_index = 0);
    SymDirectory(Object*);
    SymDirectory(int type);
    SymDirectory();
    virtual ~SymDirectory();

    virtual const std::string& path() const;
    virtual int count() const;
    virtual const std::string& name(int index) const;
    virtual int index(const std::string&) const;
    virtual void whole_name(int index, std::string&) const;
    virtual bool is_directory(int index) const;
    virtual double* variable(int index);
    virtual int whole_vector(int index);

    static bool match(const std::string& name, const std::string& pattern);
    Symbol* symbol(int index) const;
    int array_index(int index) const;
    Object* object() const;  // the parent_object
    Object* obj(int index);  // non-NULL if SymbolItem is an object
    virtual bool is_pysec(int index) const;
    SymDirectory* newsymdir(int index);

  private:
    SymDirectoryImpl* impl_;
};

#endif
