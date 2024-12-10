#ifdef HAVE_CONFIG_H
#include <../../nrnconf.h>
#endif
/*
Based on Unidraw UList but UList changed to HTList (head tail list)
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
 * HTList implementation.
 */

#include <stdio.h>
#include <OS/enter-scope.h>
#include <htlist.h>

/*****************************************************************************/

HTList::HTList() {
    _next = this;
    _prev = this;
}

HTList::~HTList() {
    HTList* next = _next;
    if (next != this && next != NULL) {
        this->Remove();
        delete next;
    }
}

void HTList::Append(HTList* e) {
    _prev->_next = e;
    e->_prev = _prev;
    e->_next = this;
    _prev = e;
}

void HTList::Remove() {
    if (_prev) {
        _prev->_next = _next;
    }
    if (_next) {
        _next->_prev = _prev;
    }
    _prev = _next = NULL;
}

void HTList::RemoveAll() {
    while (!IsEmpty()) {
        First()->Remove();
    }
}
