#ifdef HAVE_CONFIG_H
#include <../../nrnconf.h>
#endif
/*
 * Copyright (c) 1992 Stanford University
 * Copyright (c) 1992 Silicon Graphics, Inc.
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
 * Observable - object to observe
 */

#include <InterViews/observe.h>
#include <OS/list.h>

declarePtrList(ObserverList,Observer)
implementPtrList(ObserverList,Observer)

Observable::Observable() {
    observers_ = nil;
}

Observable::~Observable() {
    ObserverList* list = observers_;
    if (list != nil) {
	for (ListItr(ObserverList) i(*list); i.more(); i.next()) {
	    i.cur()->disconnect(this);
	}
	delete list;
    }
}

void Observable::attach(Observer* o) {
    ObserverList* list = observers_;
    if (list == nil) {
	list = new ObserverList(5);
	observers_ = list;
    }
    list->append(o);
}

void Observable::detach(Observer* o) {
    ObserverList* list = observers_;
    if (list != nil) {
	for (ListUpdater(ObserverList) i(*list); i.more(); i.next()) {
	    if (i.cur() == o) {
		i.remove_cur();
		break;
	    }
	}
    }
}

void Observable::notify() {
    ObserverList* list = observers_;
    if (list != nil) {
	for (ListItr(ObserverList) i(*list); i.more(); i.next()) {
	    i.cur()->update(this);
	}
    }
}

Observer::Observer() { }
Observer::~Observer() { }
void Observer::update(Observable*) { }
void Observer::disconnect(Observable*) { }
