#ifndef nrnhash_h
#define nrnhash_h
// hash table where buckets are binary search maps and key is castable to unsigned long

#include <OS/table.h>
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
#define __NrnHashLT(Table) nrnhash_lt_##Table
#define NrnHashLT(Table) __NrnHashLT(Table)

	// note that more recent STL versions (But not gcc2.95) supply 
	// at(long) to get the vector element. My version contains some
	// confusion with regard to how to handle issues involving const
	// in this classes implementation of the find method.
	// [] creates key value association if does not exist

#define declareNrnHash(Table,Key,Value) \
struct NrnHashLT(Table) { \
	int operator() (Key i, Key j) const { \
		return (key_to_hash(i) < key_to_hash(j)); \
	} \
}; \
\
class NrnHashEntry(Table) : public std::map <Key, Value, NrnHashLT(Table)>{}; \
\
class Table : public vector<NrnHashEntry(Table)> { \
public: \
	Table(long size); \
	virtual ~Table(); \
	bool find(Key, Value&)const; \
	NrnHashEntry(Table)& at(unsigned long bucket){ return *(begin() + bucket); } \
	Value& operator[](Key key) { return at(hash(key))[key]; } \
	void remove(Key); \
	unsigned long hash(Key key)const { return (key_to_hash(key))%size_; } \
	long size_; \
};

#define implementNrnHash(Table,Key,Value) \
Table::Table(long size) { \
	resize(size+1); \
	size_ = size; \
} \
\
Table::~Table() {} \
\
bool Table::find(Key key, Value& ps)const { \
	NrnHashEntry(Table)::const_iterator itr; \
	const NrnHashEntry(Table)& gm = ((Table*)this)->at(hash(key)); \
	if ((itr = gm.find(key)) == gm.end()) { \
		return false; \
	} \
	ps = itr->second; \
	return true; \
}\
\
void Table::remove(Key key) { \
	NrnHashEntry(Table)& gm = ((Table*)this)->at(hash(key)); \
	gm.erase(key); \
}

// for iteration, if you have 
// declareNrnHash(Table,Key,Object)
// Table* table;
// then you can iterate with
#define NrnHashIterate(Table,table,Value,value) \
	if (table) for (long i__ = table->size_ - 1; i__ >= 0; --i__) { \
		NrnHashEntry(Table)::const_iterator p__ = table->at(i__).begin(); \
		NrnHashEntry(Table)::const_iterator pe__ = table->at(i__).end(); \
		while(p__ != pe__) { \
			Value value = (*p__).second; \
			++p__; \
// need to close with two extra }}

#define NrnHashIterateKeyValue(Table,table, Key,key, Value,value) \
	if (table) for (long i__ = table->size_ - 1; i__ >= 0; --i__) { \
		NrnHashEntry(Table)::const_iterator p__ = table->at(i__).begin(); \
		NrnHashEntry(Table)::const_iterator pe__ = table->at(i__).end(); \
		while(p__ != pe__) { \
			Key key = (*p__).first; \
			Value value = (*p__).second; \
			++p__; \
// need to close with two extra }}

#endif // nrnhash_h
