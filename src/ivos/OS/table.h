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

#endif
