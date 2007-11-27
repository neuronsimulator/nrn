#ifdef HAVE_CONFIG_H
#include <../../nrnconf.h>
#endif
#if carbon
#undef MAC
#endif

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

// =======================================================================
//
// 1.5
// 1999/07/05 15:34:58
//
// Windows 3.1/NT InterViews Port 
// Copyright (c) 1993 Tim Prinzing
//
// Permission to use, copy, modify, distribute, and sell this software and 
// its documentation for any purpose is hereby granted without fee, provided
// that (i) the above copyright notice and this permission notice appear in
// all copies of the software and related documentation, and (ii) the name of
// Tim Prinzing may not be used in any advertising or publicity relating to 
// the software without the specific, prior written permission of Tim Prinzing.
// 
// THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
// EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
// WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
//
// IN NO EVENT SHALL Tim Prinzing BE LIABLE FOR ANY SPECIAL, INCIDENTAL, 
// INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND, OR ANY DAMAGES WHATSOEVER 
// RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER OR NOT ADVISED OF THE 
// POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF LIABILITY, ARISING OUT OF OR 
// IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//
// =======================================================================

#include <OS/file.h>
#include <OS/string.h>
#include <OS/types.h>
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>

#ifndef MAC
	#include <sys/stat.h>
#endif

#ifdef HAVE_SYS_MMAN_H
extern "C" {
#include <sys/mman.h>
}
#endif

#ifdef WIN16
#define WIN32
#endif

#ifdef WIN32
#include <io.h>
#endif

#ifdef CYGWIN
extern "C" {
// These are the POSIX definitions.  Hopefully they won't conflict.
    extern int _close(int);
    extern int _read(int, void*, unsigned int);
}
#endif

#if !defined(__GNUC__) || !defined (WIN32) && !defined (MAC)

/* no standard place for these */
// Yes there is.  Posix says read and close are in unistd.h.
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#else
extern "C" {
// These are the POSIX definitions.  Hopefully they won't conflict.
    extern int close(int);
    extern int read(int, void*, unsigned int);
}
#endif
//#if defined(SGI)
//#endif
//#if defined(sun) && !defined(__SYSENT_H)
//    extern int read(int, void*, unsigned int);
//#endif
//#if defined(AIXV3)
//    extern int read(int, char*, unsigned int);
//#endif
//#if defined(apollo)
//    extern long read(int, void*, unsigned int);
//#endif
//#if defined(__DECCXX)
//    extern int read(int, void*, unsigned int);
//#endif
//}
#endif /* WIN32 */

class FileInfo {
#ifndef MAC
public:
    CopyString* name_;
    int fd_;
    char* map_;
    struct stat info_;
    off_t pos_;
    char* buf_;
    unsigned int limit_;

    FileInfo(CopyString*, int fd);
#endif
};

#ifndef MAC
FileInfo::FileInfo(CopyString* s, int fd) {
    name_ = s;
    fd_ = fd;
    pos_ = 0;
    limit_ = 0;
    map_ = nil;
    buf_ = nil;
}
#endif


File::File(FileInfo* i) {
    assert(i != nil);
    rep_ = i;
}

File::~File() {
    close();
#ifndef MAC
    delete rep_->name_;
    delete rep_;
#endif
}

const String* File::name() const {
#ifndef MAC    
    return rep_->name_;
#else
	return nil;
#endif
}

long File::length() const {
#ifndef MAC
    return rep_->info_.st_size;
#else
	return 0;
#endif
}

void File::close() {
#ifndef MAC
    FileInfo* i = rep_;
    if (i->fd_ >= 0) {
	if (i->map_ != nil) {
#ifdef HAVE_SYS_MMAN_H // #if defined(SGI) || defined(__alpha)
	    munmap(i->map_, int(i->info_.st_size));
#endif
	}
	if (i->buf_ != nil) {
	    delete [] i->buf_;
	}
#ifdef WIN32
	_close(i->fd_);
#else
	::close(i->fd_);
#endif
	i->fd_ = -1;
    }
#endif
}

void File::limit(unsigned int buffersize) {
#ifndef MAC
    rep_->limit_ = buffersize;
#endif
}

FileInfo* File::rep() const { 
#ifndef MAC
	return rep_; 
#else
	return nil;
#endif
}

/* class InputFile */

InputFile::InputFile(FileInfo* i) : File(i) { }
InputFile::~InputFile() { }

InputFile* InputFile::open(const String& name) {
	 CopyString* s = new CopyString(name);
#ifndef MAC

#if defined(WIN32) && !defined(__MWERKS__) && !defined(CYGWIN)
	 int fd = _open((char*)s->string(), O_RDONLY);
#else
    /* cast to workaround DEC C++ prototype bug */
    int fd = ::open((char*)s->string(), O_RDONLY);
#endif
    if (fd < 0) {
	delete s;
	return nil;
    }
    FileInfo* i = new FileInfo(s, fd);
    if (fstat(fd, &i->info_) < 0) {
	delete s;
	delete i;
	return nil;
    }
    return new InputFile(i);
#else
	return nil;
#endif
}

int InputFile::read(const char*& start) {
#ifndef MAC
    FileInfo* i = rep();
    int len = (int)(i->info_.st_size);
    if (i->pos_ >= len) {
	return 0;
    }
    if (i->limit_ != 0 && len > i->limit_) {
	len = (int)(i->limit_);
    }
#if HAVE_SYS_MMAN_H // #if defined(SGI) || defined(__alpha)
    i->map_ = (char*)mmap(0, len, PROT_READ, MAP_PRIVATE, i->fd_, i->pos_);
    if ((long)(i->map_) == -1) {
	return -1;
    }
    start = i->map_;
#else
    if (i->buf_ == nil) {
	i->buf_ = new char[len];
    }
    start = i->buf_;
#ifdef WIN32
    len = _read(i->fd_, i->buf_, len);
#else
    len = ::read(i->fd_, i->buf_, len);
#endif /* WIN32 */

#endif
    i->pos_ += len;
    return len;
#else 
	return 0;
#endif
}

/* class StdInput */

#if !MAC
StdInput::StdInput() : InputFile(new FileInfo(new CopyString("-stdin"), 0)) { }
#endif
StdInput::~StdInput() { }

long StdInput::length() const { return -1; }

int StdInput::read(const char*& start) {
#ifndef MAC
    FileInfo* i = rep();
    if (i->buf_ == nil) {
	if (i->limit_ == 0) {
	    i->limit_ = BUFSIZ;
	}
	i->buf_ = new char[i->limit_];
    }

#ifdef WIN32
    int nbytes = _read(i->fd_, (char*)i->buf_, i->limit_);
#else
    int nbytes = ::read(i->fd_, (char*)i->buf_, i->limit_);
#endif /* WIN32 */

    if (nbytes > 0) {
	start = (const char*)(i->buf_);
    }
    return nbytes;
#else
	return 0;
#endif
}
