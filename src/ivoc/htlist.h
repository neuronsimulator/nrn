/*
from Unidraw but UList changed to HTList (head tail list)
for fast insertion, deletion, iteration
*/

/*
 * Copyright (c) 1990, 1991 Stanford University
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Stanford not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  Stanford makes no representations about
 * the suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * STANFORD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
 * IN NO EVENT SHALL STANFORD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * UList - list object.
 */

#ifndef htlist_h
#define htlist_h

class HTList {
public:
    HTList(void* = NULL);
    virtual ~HTList();

    bool IsEmpty();
    void Append(HTList*);
    void Prepend(HTList*);
    void Remove(HTList*);
    void Remove();
    void RemoveAll();
    void Delete(void*);
    HTList* Find(void*);
    HTList* First();
    HTList* Last();
    HTList* End();
    HTList* Next();
    HTList* Prev();

    void* vptr();
    void* operator()();
    HTList* operator[](int count);
protected:
    void* _object;
    HTList* _next;
    HTList* _prev;
};

inline bool HTList::IsEmpty () { return _next == this; }
inline HTList* HTList::First () { return _next; }
inline HTList* HTList::Last () { return _prev; }
inline HTList* HTList::End () { return this; }
inline HTList* HTList::Next () { return _next; }
inline HTList* HTList::Prev () { return _prev; }
inline void* HTList::operator() () { return _object; }
inline void* HTList::vptr() { return _object; }
#endif
