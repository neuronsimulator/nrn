#ifdef HAVE_CONFIG_H
#include <../../nrnconf.h>
#endif
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

#include <OS/table.h>
#include <OS/ustring.h>
#include <string.h>

/*
 * UniqueString uses a table for matching strings and a string pool
 * for storing the string data.  A pool is presumably more efficient
 * than malloc/new, but individual strings cannot be deallocated.
 */

inline unsigned long key_to_hash(String& s) { return s.hash(); }

declareTable(UniqueStringTable,String,String)
implementTable(UniqueStringTable,String,String)

static const unsigned strpoolsize = 800;

class UniqueStringPool {
public:
    UniqueStringPool(unsigned poolsize = strpoolsize);
    ~UniqueStringPool();

    char* add(const char*, unsigned);
private:
    char* data;
    unsigned size;
    unsigned cur;
    UniqueStringPool* prev;
};

UniqueStringTable* UniqueString::table_;
UniqueStringPool* UniqueString::pool_;

UniqueString::UniqueString() : String() { }
UniqueString::UniqueString(const char* s) : String() { init(String(s)); }
UniqueString::UniqueString(const char* s, int n) : String() {
    init(String(s, n));
}
UniqueString::UniqueString(const String& s) : String() { init(s); }
UniqueString::UniqueString(const UniqueString& s) : String(s) { }
UniqueString::~UniqueString() { }

void UniqueString::init(const String& s) {
    if (table_ == nil) {
        table_ = new UniqueStringTable(256);
    }
    if (!table_->find(*this, s)) {
        if (pool_ == nil) {
            pool_ = new UniqueStringPool;
        }
        int n = s.length();
        set_value(pool_->add(s.string(), n), n);
        table_->insert(*this, *this);
    }
}

/*
 * UniqueString's have a unique data pointer, so we can just use
 * that for a hash value.
 */

unsigned long UniqueString::hash() const { return key_to_hash(string()); }

bool UniqueString::operator ==(const String& s) const {
    return string() == s.string() && length() == s.length();
}

bool UniqueString::operator ==(const char* s) const {
    return String::operator ==(s);
}

bool UniqueString::null_terminated() const { return false; }

/*
 * UniqueStringPool implementation.
 */

UniqueStringPool::UniqueStringPool(unsigned poolsize) {
    data = new char[poolsize];
    size = poolsize;
    cur = 0;
    prev = nil;
}

/*
 * Tail-recursive deletion to walk the list back to the head
 * of the pool.
 */

UniqueStringPool::~UniqueStringPool() {
    delete [] data;
    delete prev;
}

/*
 * Add a string of a given length to the pool.  If it won't fit,
 * create a copy of the current pool and allocate space for a new one.
 *
 * No null-padding is implied, so if you want that you must include
 * the null in the length.
 */

char* UniqueStringPool::add(const char* str, unsigned len) {
    if (len > strpoolsize) {
	UniqueStringPool* s = new UniqueStringPool(len);
	strncpy(s->data, str, len);
	s->cur = len;
	s->prev = prev;
	prev = s;
	return s->data;
    }
    unsigned index = cur;
    unsigned newcur = index + len;
    if (newcur > size) {
	UniqueStringPool* s = new UniqueStringPool;
	char* newdata = s->data;
	s->data = data;
	s->size = size;
	s->cur = cur;
	s->prev = prev;
	data = newdata;
	prev = s;
	index = 0;
	newcur = len;
    }
    char* r = &data[index];
    strncpy(r, str, len);
    cur = newcur;
    return r;
}
