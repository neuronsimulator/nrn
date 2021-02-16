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
 * Object association table with 2 keys.
 */

#ifndef os_table2_h
#define os_table2_h

#include <OS/enter-scope.h>

#if defined(__STDC__) || defined(__ANSI_CPP__)
#define __Table2Entry(Table2) Table2##_Entry
#define Table2Entry(Table2) __Table2Entry(Table2)
#define __Table2Iterator(Table2) Table2##_Iterator
#define Table2Iterator(Table2) __Table2Iterator(Table2)
#else
#define __Table2Entry(Table2) Table2/**/_Entry
#define Table2Entry(Table2) __Table2Entry(Table2)
#define __Table2Iterator(Table2) Table2/**/_Iterator
#define Table2Iterator(Table2) __Table2Iterator(Table2)
#endif

#define declareTable2(Table2,Key1,Key2,Value) \
struct Table2Entry(Table2); \
\
class Table2 { \
public: \
    Table2(int); \
    ~Table2(); \
\
    void insert(Key1, Key2, Value); \
    bool find(Value&, Key1, Key2); \
    void remove(Key1, Key2); \
private: \
    friend class Table2Iterator(Table2); \
\
    int size_; \
    Table2Entry(Table2)** first_; \
    Table2Entry(Table2)** last_; \
\
    Table2Entry(Table2)*& probe(Key1, Key2); \
}; \
\
struct Table2Entry(Table2) { \
private: \
friend class Table2; \
friend class Table2Iterator(Table2); \
\
    Key1 key1_; \
    Key2 key2_; \
    Value value_; \
    Table2Entry(Table2)* chain_; \
}; \
\
class Table2Iterator(Table2) { \
public: \
    Table2Iterator(Table2)(Table2&); \
\
    Key1& cur_key1(); \
    Key2& cur_key2(); \
    Value& cur_value(); \
    bool more(); \
    bool next(); \
private: \
    Table2Entry(Table2)* cur_; \
    Table2Entry(Table2)** entry_; \
    Table2Entry(Table2)** last_; \
}; \
\
inline Key1& Table2Iterator(Table2)::cur_key1() { return cur_->key1_; } \
inline Key2& Table2Iterator(Table2)::cur_key2() { return cur_->key2_; } \
inline Value& Table2Iterator(Table2)::cur_value() { return cur_->value_; } \
inline bool Table2Iterator(Table2)::more() { return entry_ <= last_; }

/*
 * Predefined hash functions
 */

#ifndef os_table_h
inline unsigned long key_to_hash(long k) { return (unsigned long)k; }
inline unsigned long key_to_hash(const void* k) { return (unsigned long)k; }
#endif

/*
 * Table2 implementation
 */

#define implementTable2(Table2,Key1,Key2,Value) \
Table2::Table2(int n) { \
    for (size_ = 32; size_ < n; size_ <<= 1); \
    first_ = new Table2Entry(Table2)*[size_]; \
    --size_; \
    last_ = &first_[size_]; \
    for (Table2Entry(Table2)** e = first_; e <= last_; e++) { \
	*e = nil; \
    } \
} \
\
Table2::~Table2() { \
    delete [] first_; \
} \
\
inline Table2Entry(Table2)*& Table2::probe(Key1 k1, Key2 k2) { \
    return first_[(key_to_hash(k1) ^ key_to_hash(k2)) & size_]; \
} \
\
void Table2::insert(Key1 k1, Key2 k2, Value v) { \
    Table2Entry(Table2)* e = new Table2Entry(Table2); \
    e->key1_ = k1; \
    e->key2_ = k2; \
    e->value_ = v; \
    Table2Entry(Table2)** a = &probe(k1, k2); \
    e->chain_ = *a; \
    *a = e; \
} \
\
bool Table2::find(Value& v, Key1 k1, Key2 k2) { \
    for ( \
	Table2Entry(Table2)* e = probe(k1, k2); \
	e != nil; \
	e = e->chain_ \
    ) { \
	if (e->key1_ == k1 && e->key2_ == k2) { \
	    v = e->value_; \
	    return true; \
	} \
    } \
    return false; \
} \
\
void Table2::remove(Key1 k1, Key2 k2) { \
    Table2Entry(Table2)** a = &probe(k1, k2); \
    Table2Entry(Table2)* e = *a; \
    if (e != nil) { \
	if (e->key1_ == k1 && e->key2_ == k2) { \
	    *a = e->chain_; \
	    delete e; \
	} else { \
	    Table2Entry(Table2)* prev; \
	    do { \
		prev = e; \
		e = e->chain_; \
	    } while (e != nil && (e->key1_ != k1 || e->key2_ != k2)); \
	    if (e != nil) { \
		prev->chain_ = e->chain_; \
		delete e; \
	    } \
	} \
    } \
} \
\
Table2Iterator(Table2)::Table2Iterator(Table2)(Table2& t) { \
    last_ = t.last_; \
    for (entry_ = t.first_; entry_ <= last_; entry_++) { \
	cur_ = *entry_; \
	if (cur_ != nil) { \
	    break; \
	} \
    } \
} \
\
bool Table2Iterator(Table2)::next() { \
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
