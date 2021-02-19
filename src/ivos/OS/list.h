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
 * Generic list implemented as dynamic array
 */

#ifndef os_list_h
#define os_list_h

#include <OS/enter-scope.h>

extern void ListImpl_range_error(long index);
extern long ListImpl_best_new_count(long count, unsigned int size, unsigned int m = 1);

#if 1 || defined(__STDC__) || defined(__ANSI_CPP__)
#define __ListItr(List) List##_Iterator
#define ListItr(List) __ListItr(List)
#define __ListUpdater(List) List##_Updater
#define ListUpdater(List) __ListUpdater(List)
#else
#define __ListItr(List) List/**/_Iterator
#define ListItr(List) __ListItr(List)
#define __ListUpdater(List) List/**/_Updater
#define ListUpdater(List) __ListUpdater(List)
#endif

#define declareList(List,T) \
class List { \
public: \
    List(long size = 0); \
    ~List(); \
\
    long count() const; \
    T item(long index) const; \
    T& item_ref(long index) const; \
\
    void prepend(const T&); \
    void append(const T&); \
    void insert(long index, const T&); \
    void remove(long index); \
    void remove_all(); \
private: \
    T* items_; \
    long size_; \
    long count_; \
    long free_; \
}; \
\
inline long List::count() const { return count_; } \
\
inline T List::item(long index) const { \
    if (index < 0 || index >= count_) { \
	ListImpl_range_error(index); \
    } \
    long i = index < free_ ? index : index + size_ - count_; \
    return items_[i]; \
} \
inline T& List::item_ref(long index) const { \
    if (index < 0 || index >= count_) { \
	ListImpl_range_error(index); \
    } \
    long i = index < free_ ? index : index + size_ - count_; \
    return items_[i]; \
} \
\
inline void List::append(const T& item) { insert(count_, item); } \
inline void List::prepend(const T& item) { insert(0, item); } \
\
class ListItr(List) { \
public: \
    ListItr(List)(const List&); \
\
    bool more() const; \
    T cur() const; \
    T& cur_ref() const; \
    void next(); \
private: \
    const List* list_; \
    long cur_; \
}; \
\
inline bool ListItr(List)::more() const { return cur_ < list_->count(); } \
inline T ListItr(List)::cur() const { return list_->item(cur_); } \
inline T& ListItr(List)::cur_ref() const { \
    return list_->item_ref(cur_); \
} \
inline void ListItr(List)::next() { ++cur_; } \
\
class ListUpdater(List) { \
public: \
    ListUpdater(List)(List&); \
\
    bool more() const; \
    T cur() const; \
    T& cur_ref() const; \
    void remove_cur(); \
    void next(); \
private: \
    List* list_; \
    long cur_; \
}; \
\
inline bool ListUpdater(List)::more() const { \
    return cur_ < list_->count(); \
} \
inline T ListUpdater(List)::cur() const { return list_->item(cur_); } \
inline T& ListUpdater(List)::cur_ref() const { \
    return list_->item_ref(cur_); \
} \
inline void ListUpdater(List)::remove_cur() { list_->remove(cur_); } \
inline void ListUpdater(List)::next() { ++cur_; }

/*
 * Lists of pointers
 *
 * Don't ask me to explain the AnyPtr nonsense.  C++ compilers
 * have a hard time deciding between (const void*)& and const (void*&).
 * Typedefs help, though still keep me guessing.
 */

typedef void* __AnyPtr;

declareList(__AnyPtrList,__AnyPtr)

#define declarePtrList(PtrList,T) \
class PtrList { \
public: \
    PtrList(long size = 0); \
\
    long count() const; \
    T* item(long index) const; \
\
    void prepend(T*); \
    void append(T*); \
    void insert(long index, T*); \
    void remove(long index); \
    void remove_all(); \
private: \
    __AnyPtrList impl_; \
}; \
\
inline PtrList::PtrList(long size) : impl_(size) { } \
inline long PtrList::count() const { return impl_.count(); } \
inline T* PtrList::item(long index) const { return (T*)impl_.item(index); } \
inline void PtrList::append(T* item) { insert(impl_.count(), item); } \
inline void PtrList::prepend(T* item) { insert(0, item); } \
inline void PtrList::remove(long index) { impl_.remove(index); } \
inline void PtrList::remove_all() { impl_.remove_all(); } \
\
class ListItr(PtrList) { \
public: \
    ListItr(PtrList)(const PtrList&); \
\
    bool more() const; \
    T* cur() const; \
    void next(); \
private: \
    const PtrList* list_; \
    long cur_; \
}; \
\
inline bool ListItr(PtrList)::more() const { \
    return cur_ < list_->count(); \
} \
inline T* ListItr(PtrList)::cur() const { return list_->item(cur_); } \
inline void ListItr(PtrList)::next() { ++cur_; } \
\
class ListUpdater(PtrList) { \
public: \
    ListUpdater(PtrList)(PtrList&); \
\
    bool more() const; \
    T* cur() const; \
    void remove_cur(); \
    void next(); \
private: \
    PtrList* list_; \
    long cur_; \
}; \
\
inline bool ListUpdater(PtrList)::more() const { \
    return cur_ < list_->count(); \
} \
inline T* ListUpdater(PtrList)::cur() const { return list_->item(cur_); } \
inline void ListUpdater(PtrList)::remove_cur() { list_->remove(cur_); } \
inline void ListUpdater(PtrList)::next() { ++cur_; }

/*
 * List implementation
 */

#define implementList(List,T) \
List::List(long size) { \
    if (size > 0) { \
        size_ = ListImpl_best_new_count(size, sizeof(T)); \
        items_ = new T[size_]; \
    } else { \
        size_ = 0; \
        items_ = 0; \
    } \
    count_ = 0; \
    free_ = 0; \
} \
\
List::~List() { \
    delete [] items_; \
} \
\
void List::insert(long index, const T& item) { \
    if (count_ == size_) { \
        long size = ListImpl_best_new_count(size_ + 1, sizeof(T), 2); \
        T* items = new T[size]; \
        if (items_ != 0) { \
            long i; \
            for (i = 0; i < free_; ++i) { \
                items[i] = items_[i]; \
            } \
            for (i = 0; i < count_ - free_; ++i) { \
                items[free_ + size - count_ + i] = \
                    items_[free_ + size_ - count_ + i]; \
            } \
	    delete [] items_; \
        } \
        items_ = items; \
        size_ = size; \
    } \
    if (index >= 0 && index <= count_) { \
	if (index < free_) { \
            for (long i = free_ - index - 1; i >= 0; --i) { \
                items_[index + size_ - count_ + i] = items_[index + i]; \
            } \
        } else if (index > free_) { \
            for (long i = 0; i < index - free_; ++i) { \
                items_[free_ + i] = items_[free_ + size_ - count_ + i]; \
            } \
        } \
        free_ = index + 1; \
        count_ += 1; \
        items_[index] = item; \
    } \
} \
\
void List::remove(long index) { \
    if (index >= 0 && index <= count_) { \
        if (index < free_) { \
            for (long i = free_ - index - 2; i >= 0; --i) { \
                items_[size_ - count_ + index + 1 + i] = \
		    items_[index + 1 + i]; \
            } \
        } else if (index > free_) { \
            for (long i = 0; i < index - free_; ++i) { \
                items_[free_ + i] = items_[free_ + size_ - count_ + i]; \
            } \
        } \
        free_ = index; \
        count_ -= 1; \
    } \
} \
\
void List::remove_all() { \
    count_ = 0; \
    free_ = 0; \
} \
\
ListItr(List)::ListItr(List)(const List& list) { \
    list_ = &list; \
    cur_ = 0; \
} \
\
ListUpdater(List)::ListUpdater(List)(List& list) { \
    list_ = &list; \
    cur_ = 0; \
}

#define implementPtrList(PtrList,T) \
void PtrList::insert(long index, T* item) { \
    const __AnyPtr p = item; \
    impl_.insert(index, p); \
} \
ListItr(PtrList)::ListItr(PtrList)(const PtrList& list) { \
    list_ = &list; \
    cur_ = 0; \
} \
\
ListUpdater(PtrList)::ListUpdater(PtrList)(PtrList& list) { \
    list_ = &list; \
    cur_ = 0; \
}

#endif
