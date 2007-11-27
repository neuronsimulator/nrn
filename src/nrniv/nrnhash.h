#ifndef nrnhash_h
#define nrnhash_h
// hash table where buckets are binary search maps and key is an int

#include <ivstream.h>

#if defined(HAVE_SSTREAM) // the standard...
#include <vector>
#include <map>
#else
#include <vector.h>
#include <map.h>
#endif

#define __NrnHashEntry(Table) Table##_Entry
#define NrnHashEntry(Table) __NrnHashEntry(Table)

struct nrnhash_ltint {
	int operator() (int i, int j) const {
		return i < j;
	}
};

	// note that more recent STL versions (But not gcc2.95) supply 
	// at(int) to get the vector element. My version contains some
	// confusion with regard to how to handle issues involving const
	// in this classes implementation of the find method.
	// [] creates key value association if does not exist

#define declareNrnHash(Table,Key,Value) \
class NrnHashEntry(Table) : public map <int, Value, nrnhash_ltint>{}; \
\
class Table : public vector<NrnHashEntry(Table)> { \
public: \
	Table(int size); \
	virtual ~Table(); \
	boolean find(int, Value&)const; \
	NrnHashEntry(Table)& at(Key key){ return *(begin() + key); } \
	Value& operator[](Key key) { return at(hash(key))[key]; } \
	int hash(Key key)const { return key%size_; } \
	int size_; \
};

#define implementNrnHash(Table,Key,Value) \
Table::Table(int size) { \
	resize(size+1); \
	size_ = size; \
} \
\
Table::~Table() {} \
\
boolean Table::find(int key, Value& ps)const { \
	NrnHashEntry(Table)::const_iterator itr; \
	const NrnHashEntry(Table)& gm = ((Table*)this)->at(hash(key)); \
	if ((itr = gm.find(key)) == gm.end()) { \
		return false; \
	} \
	ps = itr->second; \
	return true; \
}

// for iteration, if you have 
// declareNrnHash(Table,int,Object)
// Table* table;
// then you can iterate with
#define NrnHashIterate(Table,table,Value,value) \
	if (table) for (int i__ = table->size_ - 1; i__ >= 0; --i__) { \
		NrnHashEntry(Table)::const_iterator p__ = table->at(i__).begin(); \
		NrnHashEntry(Table)::const_iterator pe__ = table->at(i__).end(); \
		while(p__ != pe__) { \
			Value value = (*p__).second; \
			++p__; \
// need to close with two extra }}

#endif // nrnhash_h
