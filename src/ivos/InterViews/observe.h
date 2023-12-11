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

#ifndef iv_observe_h
#define iv_observe_h

#include <InterViews/enter-scope.h>

#include <InterViews/_enter.h>
#include <vector>

class Observer;

class Observable {
public:
    Observable() = default;
    virtual ~Observable();

    virtual void attach(Observer*);
    virtual void detach(Observer*);
    virtual void notify();
private:
    std::vector<Observer*> observers_;
};

class Observer {
protected:
    Observer() = default;
public:
    virtual ~Observer() = default;

    virtual void update(Observable*) {};
    virtual void disconnect(Observable*) {};
};

#include <InterViews/_leave.h>

#endif
