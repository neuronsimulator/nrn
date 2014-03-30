/* a copy of OS/table.h hacked to be a replacement for a NrnHash */
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

#ifndef nrnhash_alt_h
#define nrnhash_alt_h

#if defined(HAVE_STDINT_H)
#include <stdint.h>
#endif

#include <OS/enter-scope.h>

#if 1 || defined(__STDC__) || defined(__ANSI_CPP__)
#define __NrnHashEntry(NrnHash) NrnHash##_Entry
#define NrnHashEntry(NrnHash) __NrnHashEntry(NrnHash)
#define __NrnHashIterator(NrnHash) NrnHash##_Iterator
#define NrnHashIterator(NrnHash) __NrnHashIterator(NrnHash)
#else
#define __NrnHashEntry(NrnHash) NrnHash/**/_Entry
#define NrnHashEntry(NrnHash) __NrnHashEntry(NrnHash)
#define __NrnHashIterator(NrnHash) NrnHash/**/_Iterator
#define NrnHashIterator(NrnHash) __NrnHashIterator(NrnHash)
#endif

#define declareNrnHash(NrnHash,Key,Value) \
struct NrnHashEntry(NrnHash); \
\
class NrnHash { \
public: \
    NrnHash(int); \
    ~NrnHash(); \
\
    void insert(Key, Value); \
    bool find(Key, Value&); \
    bool find_and_remove(Value&, Key); \
    void remove(Key); \
    void remove_all(); \
    int max_chain_length(); \
    int nclash() {return nclash_;} \
    int nfind() { return nfind_;} \
private: \
    friend class NrnHashIterator(NrnHash); \
\
    int size_; \
    NrnHashEntry(NrnHash)** first_; \
    NrnHashEntry(NrnHash)** last_; \
\
    NrnHashEntry(NrnHash)*& probe(Key); \
    int nclash_; \
    int nfind_; \
}; \
\
struct NrnHashEntry(NrnHash) { \
private: \
    friend class NrnHash; \
    friend class NrnHashIterator(NrnHash); \
\
    Key key_; \
    Value value_; \
    NrnHashEntry(NrnHash)* chain_; \
}; \
\
class NrnHashIterator(NrnHash) { \
public: \
    NrnHashIterator(NrnHash)(NrnHash&); \
\
    Key& cur_key(); \
    Value& cur_value(); \
    bool more(); \
    bool next(); \
private: \
    NrnHashEntry(NrnHash)* cur_; \
    NrnHashEntry(NrnHash)** entry_; \
    NrnHashEntry(NrnHash)** last_; \
}; \
\
inline Key& NrnHashIterator(NrnHash)::cur_key() { return cur_->key_; } \
inline Value& NrnHashIterator(NrnHash)::cur_value() { return cur_->value_; } \
inline bool NrnHashIterator(NrnHash)::more() { return entry_ <= last_; }

/*
 * Predefined hash functions
 */

// google integer hash function
// http://www.concentric.net/~Ttwang/tech/inthash.htm
static uint32_t nrn_uint32hash( uint32_t a) {
    return a;
    a = (a ^ 61) ^ (a >> 16);
    a = a + (a << 3);  
    a = a ^ (a >> 4);  
    a = a * 0x27d4eb2d;
    a = a ^ (a >> 15);
    return a;
}

//inline unsigned long key_to_hash(long k) { return (unsigned long)k; }
//inline unsigned long key_to_hash(const void* k) { return (unsigned long)k; }
inline uint32_t nrn_key_to_hash(long k) { return nrn_uint32hash((uint32_t)k); }
#if defined(__SIZEOF_POINTER__) && __SIZEOF_POINTER__ > __SIZEOF_LONG__
inline uint32_t nrn_key_to_hash(const void* k) { return nrn_uint32hash((uint32_t)((unsigned long long)k)); }
#else
inline uint32_t nrn_key_to_hash(const void* k) { return nrn_uint32hash((uint32_t)((unsigned long)k)); }
#endif

/*
 * NrnHash implementation
 */

#define implementNrnHash(NrnHash,Key,Value) \
NrnHash::NrnHash(int n) { \
    for (size_ = 32; size_ < n; size_ <<= 1); \
    first_ = new NrnHashEntry(NrnHash)*[size_]; \
    --size_; \
    last_ = &first_[size_]; \
    for (register NrnHashEntry(NrnHash)** e = first_; e <= last_; e++) { \
	*e = NULL; \
    } \
    nclash_ = nfind_ = 0; \
} \
\
NrnHash::~NrnHash() { \
    remove_all(); \
    delete [] first_; \
} \
\
void NrnHash::remove_all() { \
    for (register NrnHashEntry(NrnHash)** e = first_; e <= last_; e++) { \
	NrnHashEntry(NrnHash)* t = *e; \
        for (register NrnHashEntry(NrnHash)* i = t; i; i = t) { \
	    t = i->chain_; \
	    delete i; \
	} \
	*e = NULL; \
    } \
} \
\
inline NrnHashEntry(NrnHash)*& NrnHash::probe(Key i) { \
    return first_[nrn_key_to_hash(i) & size_]; \
} \
\
void NrnHash::insert(Key k, Value v) { \
    register NrnHashEntry(NrnHash)* e; \
    for (e = probe(k); e != NULL; e = e->chain_) { \
	if (e->key_ == k) { \
	    e->value_ = v; \
	    return; \
	} \
    } \
    e = new NrnHashEntry(NrnHash); \
    e->key_ = k; \
    e->value_ = v; \
    register NrnHashEntry(NrnHash)** a = &probe(k); \
    e->chain_ = *a; \
    *a = e; \
} \
\
bool NrnHash::find(Key k, Value& v) { \
    ++nfind_; \
    for (register NrnHashEntry(NrnHash)* e = probe(k); e != NULL; e = e->chain_) { \
	if (e->key_ == k) { \
	    v = e->value_; \
	    return true; \
	} \
	if (e->chain_) { ++nclash_;} \
    } \
    return false; \
} \
\
bool NrnHash::find_and_remove(Value& v, Key k) { \
    NrnHashEntry(NrnHash)** a = &probe(k); \
    register NrnHashEntry(NrnHash)* e = *a; \
    if (e != NULL) { \
	if (e->key_ == k) { \
	    v = e->value_; \
	    *a = e->chain_; \
	    delete e; \
	    return true; \
	} else { \
	    register NrnHashEntry(NrnHash)* prev; \
	    do { \
		prev = e; \
		e = e->chain_; \
	    } while (e != NULL && e->key_ != k); \
	    if (e != NULL) { \
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
void NrnHash::remove(Key k) { \
    NrnHashEntry(NrnHash)** a = &probe(k); \
    register NrnHashEntry(NrnHash)* e = *a; \
    if (e != NULL) { \
	if (e->key_ == k) { \
	    *a = e->chain_; \
	    delete e; \
	} else { \
	    register NrnHashEntry(NrnHash)* prev; \
	    do { \
		prev = e; \
		e = e->chain_; \
	    } while (e != NULL && e->key_ != k); \
	    if (e != NULL) { \
		prev->chain_ = e->chain_; \
		delete e; \
	    } \
	} \
    } \
} \
\
int NrnHash::max_chain_length() { \
    int m = 0; \
    for (NrnHashEntry(NrnHash)** e = first_; e <= last_; e++) { \
        int i = 0; \
        for (NrnHashEntry(NrnHash)* ec = *e; ec; ec = ec->chain_) { \
            ++i; \
        } \
        if (i > m) { m = i; } \
    } \
    return m; \
} \
\
NrnHashIterator(NrnHash)::NrnHashIterator(NrnHash)(NrnHash& t) { \
    last_ = t.last_; \
    for (entry_ = t.first_; entry_ <= last_; entry_++) { \
	cur_ = *entry_; \
	if (cur_ != NULL) { \
	    break; \
	} \
    } \
} \
\
bool NrnHashIterator(NrnHash)::next() { \
    cur_ = cur_->chain_; \
    if (cur_ != NULL) { \
	return true; \
    } \
    for (++entry_; entry_ <= last_; entry_++) { \
	cur_ = *entry_; \
	if (cur_ != NULL) { \
	    return true; \
	} \
    } \
    return false; \
}


// for iteration, if you have 
// declareNrnHash(Table,int,Object)
// Table* table;
// then you can iterate with
#define NrnHashIterate(Table,table,Value,value) \
	if (table) for (NrnHashIterator(Table) i__(*table); i__.more(); i__.next()) {{ \
		Value value = i__.cur_value(); \
// need to close with two extra }}

#define NrnHashIterateKeyValue(Table,table, Key,key, Value,value) \
	if (table) for (NrnHashIterator(Table) i__(*table); i__.more(); i__.next()) {{ \
		Key key = i__.cur_key(); \
		Value value = i__.cur_value(); \
// need to close with two extra }}

#endif
