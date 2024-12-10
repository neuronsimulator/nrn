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

#pragma once

class HTList {
  public:
    HTList();
    virtual ~HTList();

    bool IsEmpty();
    void Append(HTList*);
    void Remove();
    void RemoveAll();
    HTList* First();
    HTList* End();
    HTList* Next();

  protected:
    void Remove(HTList*);

    HTList* _next;
    HTList* _prev;
};

inline bool HTList::IsEmpty() {
    return _next == this;
}
inline HTList* HTList::First() {
    return _next;
}
inline HTList* HTList::End() {
    return this;
}
inline HTList* HTList::Next() {
    return _next;
}
