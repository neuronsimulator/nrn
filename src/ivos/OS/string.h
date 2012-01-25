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

#ifndef os_string_h
#define os_string_h

/*
 * String - simple (non-copying) string class
 */

#include <OS/enter-scope.h>

class String {
public:
#ifdef _DELTA_EXTENSIONS
#pragma __static_class
#endif
    String();
    String(const char*);
    String(const char*, int length);
    String(const String&);
    virtual ~String();

    const char* string() const;
    int length() const;

    virtual unsigned long hash() const;
    virtual String& operator =(const String&);
    virtual String& operator =(const char*);
    virtual bool operator ==(const String&) const;
    virtual bool operator ==(const char*) const;
    virtual bool operator !=(const String&) const;
    virtual bool operator !=(const char*) const;
    virtual bool operator >(const String&) const;
    virtual bool operator >(const char*) const;
    virtual bool operator >=(const String&) const;
    virtual bool operator >=(const char*) const;
    virtual bool operator <(const String&) const;
    virtual bool operator <(const char*) const;
    virtual bool operator <=(const String&) const;
    virtual bool operator <=(const char*) const;

    virtual bool case_insensitive_equal(const String&) const;
    virtual bool case_insensitive_equal(const char*) const;

    u_char operator [](int index) const;
    virtual String substr(int start, int length) const;
    String left(int length) const;
    String right(int start) const;

    virtual void set_to_substr(int start, int length);
    void set_to_left(int length);
    void set_to_right(int start);

    virtual int search(int start, u_char) const;
    int index(u_char) const;
    int rindex(u_char) const;

    virtual bool convert(int&) const;
    virtual bool convert(long&) const;
    virtual bool convert(float&) const;
    virtual bool convert(double&) const;

    virtual bool null_terminated() const;
protected:
    virtual void set_value(const char*);
    virtual void set_value(const char*, int);
private:
    const char* data_;
    int length_;
};

class CopyString : public String {
public:
#ifdef _DELTA_EXTENSIONS
#pragma __static_class
#endif
    CopyString();
    CopyString(const char*);
    CopyString(const char*, int length);
    CopyString(const String&);
    CopyString(const CopyString&);
    virtual ~CopyString();

    virtual String& operator =(const CopyString&);
    virtual String& operator =(const String&);
    virtual String& operator =(const char*);

    virtual bool null_terminated() const;
protected:
    virtual void set_value(const char*);
    virtual void set_value(const char*, int);
private:
    void strfree();
};

class NullTerminatedString : public String {
public:
#ifdef _DELTA_EXTENSIONS
#pragma __static_class
#endif
    NullTerminatedString();
    NullTerminatedString(const String&);
    NullTerminatedString(const NullTerminatedString&);
    virtual ~NullTerminatedString();

    virtual String& operator =(const String&);
    virtual String& operator =(const char*);

    virtual bool null_terminated() const;
private:
    bool allocated_;

    void assign(const String&);
    void strfree();
};

inline const char* String::string() const { return data_; }
inline int String::length() const { return length_; }
inline u_char String::operator [](int index) const {
    return ((u_char*)data_)[index];
}

inline String String::left(int length) const { return substr(0, length); }
inline String String::right(int start) const { return substr(start, -1); }

inline void String::set_to_left(int length) { set_to_substr(0, length); }
inline void String::set_to_right(int start) { set_to_substr(start, -1); }

inline int String::index(u_char c) const { return search(0, c); }
inline int String::rindex(u_char c) const { return search(-1, c); }

#endif
