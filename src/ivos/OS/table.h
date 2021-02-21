/*
 * Copyright (c) 1987, 1988, 1989, 1990, 1991 Stanford University
 * Copyright (c) 1991 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and 
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Stanford and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Stanford and Silicon Graphics.
 * 
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
 *
 * IN NO EVENT SHALL STANFORD OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
 * OF THIS SOFTWARE.
 */

/*
 * Generic object association table.
 */

#ifndef os_table_h
#define os_table_h

#include <OS/enter-scope.h>

#if 1 || defined(__STDC__) || defined(__ANSI_CPP__)
#define __TableEntry(Table) Table##_Entry
#define TableEntry(Table) __TableEntry(Table)
#define __TableIterator(Table) Table##_Iterator
#define TableIterator(Table) __TableIterator(Table)
#else
#define __TableEntry(Table) Table/**/_Entry
#define TableEntry(Table) __TableEntry(Table)
#define __TableIterator(Table) Table/**/_Iterator
#define TableIterator(Table) __TableIterator(Table)
#endif

#define declareTable(Table,Key,Value) \
struct TableEntry(Table); \
\
class Table { \
public: \
    Table(int); \
    ~Table(); \
\
    void insert(Key, Value); \
    bool find(Value&, Key); \
    bool find_and_remove(Value&, Key); \
    void remove(Key); \
private: \
    friend class TableIterator(Table); \
\
    int size_; \
    TableEntry(Table)** first_; \
    TableEntry(Table)** last_; \
\
    TableEntry(Table)*& probe(Key); \
}; \
\
struct TableEntry(Table) { \
private: \
    friend class Table; \
    friend class TableIterator(Table); \
\
    Key key_; \
    Value value_; \
    TableEntry(Table)* chain_; \
}; \
\
class TableIterator(Table) { \
public: \
    TableIterator(Table)(Table&); \
\
    Key& cur_key(); \
    Value& cur_value(); \
    bool more(); \
    bool next(); \
private: \
    TableEntry(Table)* cur_; \
    TableEntry(Table)** entry_; \
    TableEntry(Table)** last_; \
}; \
\
inline Key& TableIterator(Table)::cur_key() { return cur_->key_; } \
inline Value& TableIterator(Table)::cur_value() { return cur_->value_; } \
inline bool TableIterator(Table)::more() { return entry_ <= last_; }

/*
 * Predefined hash functions
 */

#ifndef os_table2_h
inline unsigned long key_to_hash(long k) { return (unsigned long)k; }
#if defined(__SIZEOF_POINTER__) && __SIZEOF_POINTER__ > __SIZEOF_LONG__
inline unsigned long key_to_hash(const void* k) { return (unsigned long)((unsigned long long)k); }
#else
inline unsigned long key_to_hash(const void* k) { return (unsigned long)k; }
#endif
#endif

/*
 * Table implementation
 */

#define implementTable(Table,Key,Value) \
Table::Table(int n) { \
    for (size_ = 32; size_ < n; size_ <<= 1); \
    first_ = new TableEntry(Table)*[size_]; \
    --size_; \
    last_ = &first_[size_]; \
    for (TableEntry(Table)** e = first_; e <= last_; e++) { \
	*e = nil; \
    } \
} \
\
Table::~Table() { \
    for (TableEntry(Table)** e = first_; e <= last_; e++) { \
	TableEntry(Table)* t = *e; \
        for (TableEntry(Table)* i = t; i; i = t) { \
	    t = i->chain_; \
	    delete i; \
	} \
    } \
    delete [] first_; \
} \
\
inline TableEntry(Table)*& Table::probe(Key i) { \
    return first_[key_to_hash(i) & size_]; \
} \
\
void Table::insert(Key k, Value v) { \
    TableEntry(Table)* e = new TableEntry(Table); \
    e->key_ = k; \
    e->value_ = v; \
    TableEntry(Table)** a = &probe(k); \
    e->chain_ = *a; \
    *a = e; \
} \
\
bool Table::find(Value& v, Key k) { \
    for (TableEntry(Table)* e = probe(k); e != nil; e = e->chain_) { \
	if (e->key_ == k) { \
	    v = e->value_; \
	    return true; \
	} \
    } \
    return false; \
} \
\
bool Table::find_and_remove(Value& v, Key k) { \
    TableEntry(Table)** a = &probe(k); \
    TableEntry(Table)* e = *a; \
    if (e != nil) { \
	if (e->key_ == k) { \
	    v = e->value_; \
	    *a = e->chain_; \
	    delete e; \
	    return true; \
	} else { \
	    TableEntry(Table)* prev; \
	    do { \
		prev = e; \
		e = e->chain_; \
	    } while (e != nil && e->key_ != k); \
	    if (e != nil) { \
		v = e->value_; \
		prev->chain_ = e->chain_; \
		delete e; \
		return true; \
	    } \
	} \
    } \
    return false; \
} \
\
void Table::remove(Key k) { \
    TableEntry(Table)** a = &probe(k); \
    TableEntry(Table)* e = *a; \
    if (e != nil) { \
	if (e->key_ == k) { \
	    *a = e->chain_; \
	    delete e; \
	} else { \
	    TableEntry(Table)* prev; \
	    do { \
		prev = e; \
		e = e->chain_; \
	    } while (e != nil && e->key_ != k); \
	    if (e != nil) { \
		prev->chain_ = e->chain_; \
		delete e; \
	    } \
	} \
    } \
} \
\
TableIterator(Table)::TableIterator(Table)(Table& t) { \
    last_ = t.last_; \
    for (entry_ = t.first_; entry_ <= last_; entry_++) { \
	cur_ = *entry_; \
	if (cur_ != nil) { \
	    break; \
	} \
    } \
} \
\
bool TableIterator(Table)::next() { \
    cur_ = cur_->chain_; \
    if (cur_ != nil) { \
	return true; \
    } \
    for (++entry_; entry_ <= last_; entry_++) { \
	cur_ = *entry_; \
	if (cur_ != nil) { \
	    return true; \
	} \
    } \
    return false; \
}

#endif
