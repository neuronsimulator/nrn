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

#include <vector>

#include <InterViews/resource.h>

class ResourceImpl {
    friend class Resource;

    static bool deferred_;
    static std::vector<Resource*> deletes_;
};

bool ResourceImpl::deferred_ = false;
std::vector<Resource*> ResourceImpl::deletes_;

void Resource::ref() const {
    Resource* r = (Resource*)this;
    r->refcount_ += 1;
}

void Resource::unref() const {
    Resource* r = (Resource*)this;
    if (r->refcount_ != 0) {
        r->refcount_ -= 1;
    }
    if (r->refcount_ == 0) {
        r->cleanup();
        delete r;
    }
}

void Resource::unref_deferred() const {
    Resource* r = (Resource*)this;
    if (r->refcount_ != 0) {
        r->refcount_ -= 1;
    }
    if (r->refcount_ == 0) {
        r->cleanup();
        if (ResourceImpl::deferred_) {
            ResourceImpl::deletes_.push_back(r);
        } else {
            delete r;
        }
    }
}

void Resource::cleanup() { }

void Resource::ref(const Resource* r) {
    if (r != nil) {
        r->ref();
    }
}

void Resource::unref(const Resource* r) {
    if (r != nil) {
        r->unref();
    }
}

void Resource::unref_deferred(const Resource* r) {
    if (r != nil) {
        r->unref_deferred();
    }
}

bool Resource::defer(bool b) {
    bool previous = ResourceImpl::deferred_;
    if (b != previous) {
        flush();
        ResourceImpl::deferred_ = b;
    }
    return previous;
}

void Resource::flush() {
	bool previous = ResourceImpl::deferred_;
	ResourceImpl::deferred_ = false;
	for (auto& r: ResourceImpl::deletes_) {
	    delete r;
	}
    ResourceImpl::deletes_.clear();
    ResourceImpl::deletes_.shrink_to_fit();
	ResourceImpl::deferred_ = previous;
}
