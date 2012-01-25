/*
 * Copyright (c) 1991 Stanford University
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

#ifndef os_ustring_h
#define os_ustring_h

/*
 * UniqueString - unique string using hash table
 */

#include <OS/string.h>

class UniqueStringPool;
class UniqueStringTable;

class UniqueString : public String {
public:
#ifdef _DELTA_EXTENSIONS
#pragma __static_class
#endif
    UniqueString();
    UniqueString(const char*);
    UniqueString(const char*, int length);
    UniqueString(const String&);
    UniqueString(const UniqueString&);
    virtual ~UniqueString();

    virtual unsigned long hash() const;
    virtual bool operator ==(const String&) const;
    virtual bool operator ==(const char*) const;

    virtual bool null_terminated() const;
private:
    static UniqueStringTable* table_;
    static UniqueStringPool* pool_;

    void init(const String&);
};

#endif
