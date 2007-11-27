#ifndef symdir_h
#define symdir_h

#include <InterViews/resource.h>
#include <OS/table.h>
#include <OS/string.h>

struct Object;
class SymDirectoryImpl;
struct Symbol;

declareTable(SymbolTable, String, Symbol*)

class IvocAliases {
public:
	IvocAliases(Object*);
	virtual ~IvocAliases();
	Symbol* lookup(const char*);
	Symbol* install(const char*);
	void remove(Symbol*);
	int count();
	Symbol* symbol(int);

	Object* ob_; // not referenced
	SymbolTable* symtab_;
};

/* List of Symbols considered as a directory */

class SymDirectory : public Resource {
public:
	SymDirectory(const String& parent_path, Object* parent_object,
		Symbol*, int array_index=nil, int node_index=nil);
	SymDirectory(Object*);
	SymDirectory(int type);
	virtual ~SymDirectory();

	virtual const String& path() const;
	virtual int count() const;
	virtual const String& name(int index) const;
	virtual int index(const String&) const;
	virtual void whole_name(int index, CopyString&) const;
	virtual boolean is_directory(int index) const;
	virtual double* variable(int index);
	virtual int whole_vector(int index);

	static boolean match(const String& name, const String& pattern);
	Symbol* symbol(int index) const;
	int array_index(int index) const;
	Object* object() const; // the parent_object
	Object* obj(int index); // non-nil if SymbolItem is an object
private:
	SymDirectoryImpl* impl_;
};

#endif
