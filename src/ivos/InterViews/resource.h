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
 * Resources are shared objects.
 */

#ifndef iv_resource_h
#define iv_resource_h

#include <InterViews/enter-scope.h>

class Resource {
public:
    Resource();
    virtual ~Resource();

    virtual void ref() const;
    virtual void unref() const;
    virtual void unref_deferred() const;
    virtual void cleanup();

    /* nops for nil pointers */
    static void ref(const Resource*);
    static void unref(const Resource*);
    static void unref_deferred(const Resource*);

    /* postpone unref deletes */
    static bool defer(bool);
    static void flush();

    /* for backward compatibility */
    virtual void Reference() const { ref(); }
    virtual void Unreference() const { unref(); }
private:
    unsigned refcount_;
private:
    /* prohibit default assignment */
    Resource& operator =(const Resource&);
};

/*
 * For backward compatibility
 */

static inline void Unref(const Resource* r) { Resource::unref(r); }

#endif
