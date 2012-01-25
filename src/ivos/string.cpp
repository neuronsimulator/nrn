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

#include <OS/string.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/*
 * Just to be sure ...
 */

/* fails on mac osx
extern "C" {
#if !MAC
#ifndef tolower
    extern int tolower(int);
#endif
#ifndef toupper
    extern int toupper(int);
#endif
#endif
    extern long int strtol(const char*, char**, int);
    extern double strtod(const char*, char**);
}
*/

String::String() {
    data_ = nil;
    length_ = 0;
}

String::String(const char* s) {
    data_ = s;
    length_ = strlen(s);
}

String::String(const char* s, int n) {
    data_ = s;
    length_ = n;
}

String::String(const String& s) {
    data_ = s.data_;
    length_ = s.length_;
}

String::~String() { }

unsigned long String::hash() const {
    const char* p;
    unsigned long v = 0;
    if (length_ == -1) {
	for (p = data_; *p != '\0'; p++) {
	    v = (v << 1) ^ (*p);
	}
	String* s = (String*)this;
	s->length_ = p - data_;
    } else {
	const char* q = &data_[length_];
	for (p = data_; p < q; p++) {
	    v = (v << 1) ^ (*p);
	}
    }
    unsigned long t = v >> 10;
    t ^= (t >> 10);
    return v ^ t;
}

String& String::operator =(const String& s) {
    data_ = s.data_;
    length_ = s.length_;
    return *this;
}

String& String::operator =(const char* s) {
    data_ = s;
    length_ = strlen(s);
    return *this;
}

bool String::operator ==(const String& s) const {
    return (length_ == s.length_) && (strncmp(data_, s.data_, length_) == 0);
}

bool String::operator ==(const char* s) const {
    return (strncmp(data_, s, length_) == 0) && (s[length_] == '\0');
}

bool String::operator !=(const String& s) const {
    return (length_ != s.length_) || (strncmp(data_, s.data_, length_) != 0);
}

bool String::operator !=(const char* s) const {
    return (strncmp(data_, s, length_) != 0) || (s[length_] != '\0');
}

bool String::operator >(const String& s) const {
    return strncmp(data_, s.data_, length_) > 0;
}

bool String::operator >(const char* s) const {
    return strncmp(data_, s, length_) > 0;
}

bool String::operator >=(const String& s) const {
    return strncmp(data_, s.data_, length_) >= 0;
}

bool String::operator >=(const char* s) const {
    return strncmp(data_, s, length_) >= 0;
}

bool String::operator <(const String& s) const {
    return strncmp(data_, s.data_, length_) < 0;
}

bool String::operator <(const char* s) const {
    return strncmp(data_, s, length_) < 0;
}

bool String::operator <=(const String& s) const {
    return strncmp(data_, s.data_, length_) <= 0;
}

bool String::operator <=(const char* s) const {
    return strncmp(data_, s, length_) <= 0;
}

bool String::case_insensitive_equal(const String& s) const {
    if (length() != s.length()) {
	return false;
    }
    const char* p = string();
    const char* p2 = s.string();
    const char* q = p + length();
    for (; p < q; p++, p2++) {
	int c1 = *p;
	int c2 = *p2;
	if (c1 != c2 && tolower(c1) != tolower(c2)) {
	    return false;
	}
    }
    return true;
}

bool String::case_insensitive_equal(const char* s) const {
    return case_insensitive_equal(String(s));
}

/*
 * A negative value for start initializes the position at the end
 * of the string before indexing.  Any negative length makes
 * the substring extend to the end of the string.
 */

String String::substr(int start, int length) const {
    if (start >= length_ || start < -length_) {
	/* should raise exception */
	return String("");
    }
    int pos = (start >= 0) ? start : (length_ + start);
    if (pos + length > length_) {
	/* should raise exception */
	return String("");
    }
    int len = (length >= 0) ? length : (length_ - pos);
    return String(data_ + pos, len);
}

void String::set_to_substr(int start, int length) {
    if (start > length_ || start < -length_) {
	/* should raise exception */
	return;
    }
    int pos = (start >= 0) ? start : (length_ + start);
    if (pos + length > length_) {
	/* should raise exception */
	return;
    }
    int len = (length >= 0) ? length : (length_ - pos);
    data_ += pos;
    length_ = len;
}

bool String::null_terminated() const { return false; }

void String::set_value(const char* s) {
    data_ = s;
    length_ = strlen(s);
}

void String::set_value(const char* s, int len) {
    data_ = s;
    length_ = len;
}

/*
 * A negative value for start initializes the position to the end
 * of the string before indexing and searches right-to-left.
 */

int String::search(int start, u_char c) const {
    if (start >= length_ || start < -length_) {
	/* should raise exception */
	return -1;
    }
    if (start >= 0) {
	const char* end = data_ + length_;
	for (const char* p = data_ + start; p < end; p++) {
	    if (*p == c) {
		return p - data_;
	    }
	}
    } else {
	for (const char* p = data_ + length_ + start; p >= data_; p--) {
	    if (*p == c) {
		return p - data_;
	    }
	}
    }
    return -1;
}

/*
 * Convert a string to binary value.
 */

bool String::convert(int& value) const {
    NullTerminatedString s(*this);
    const char* str = s.string();
    char* ptr;
    value = (int)strtol(str, &ptr, 0);
    return ptr != str;
}

bool String::convert(long& value) const {
    NullTerminatedString s(*this);
    const char* str = s.string();
    char* ptr;
    value = strtol(str, &ptr, 0);
    return ptr != str;
}

bool String::convert(float& value) const {
    NullTerminatedString s(*this);
    const char* str = s.string();
    char* ptr;
    value = (float)strtod(str, &ptr);
    return ptr != str;
}

bool String::convert(double& value) const {
    NullTerminatedString s(*this);
    const char* str = s.string();
    char* ptr;
    value = strtod(str, &ptr);
    return ptr != str;
}

/* class CopyString */

CopyString::CopyString() : String() { }

CopyString::CopyString(const char* s) : String() {
    set_value(s);
}

CopyString::CopyString(const char* s, int length) : String() {
    set_value(s, length);
}

CopyString::CopyString(const String& s) : String() {
    set_value(s.string(), s.length());
}

CopyString::CopyString(const CopyString& s) : String() {
    set_value(s.string(), s.length());
}

CopyString::~CopyString() {
    strfree();
}

String& CopyString::operator =(const CopyString& s) {
    strfree();
    set_value(s.string(), s.length());
    return *this;
}

String& CopyString::operator =(const String& s) {
    strfree();
    set_value(s.string(), s.length());
    return *this;
}

String& CopyString::operator =(const char* s) {
    strfree();
    set_value(s);
    return *this;
}

bool CopyString::null_terminated() const { return true; }

void CopyString::set_value(const char* s) {
    set_value(s, strlen(s));
}

/*
 * Guarantee null-terminated string for compatibility with printf et al.
 */

void CopyString::set_value(const char* s, int len) {
    char* ns = new char[len + 1];
    ns[len] = '\0';
    String::set_value(strncpy(ns, s, len), len);
}

void CopyString::strfree() {
    char* s = (char*)(string());
    delete [] s;
}

/*
 * class NullTerminatedString
 */

NullTerminatedString::NullTerminatedString() : String() {
    allocated_ = false;
}

NullTerminatedString::NullTerminatedString(const String& s) : String() {
    assign(s);
}

NullTerminatedString::NullTerminatedString(
    const NullTerminatedString& s
) : String() {
    allocated_ = false;
    String::set_value(s.string(), s.length());
}

NullTerminatedString::~NullTerminatedString() {
    strfree();
}

String& NullTerminatedString::operator =(const String& s) {
    strfree();
    assign(s);
    return *this;
}

String& NullTerminatedString::operator =(const char* s) {
    strfree();
    allocated_ = false;
    String::set_value(s, strlen(s));
    return *this;
}

bool NullTerminatedString::null_terminated() const { return true; }

void NullTerminatedString::assign(const String& s) {
    if (s.null_terminated()) {
	allocated_ = false;
	String::set_value(s.string(), s.length());
    } else {
	allocated_ = true;
	int len = s.length();
	char* ns = new char[len + 1];
	ns[len] = '\0';
	String::set_value(strncpy(ns, s.string(), len), len);
    }
}

void NullTerminatedString::strfree() {
    if (allocated_) {
	char* s = (char*)(string());
	delete [] s;
	allocated_ = false;
    }
}
