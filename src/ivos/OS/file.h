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

#ifndef os_file_h
#define os_file_h

#include <OS/enter-scope.h>

class FileInfo;
class String;

class File {
protected:
    File(FileInfo*);
public:
    virtual ~File();

    virtual const String* name() const;
    virtual long length() const;
    virtual void close();

    virtual void limit(unsigned int buffersize);
protected:
    FileInfo* rep() const;
private:
    FileInfo* rep_;
private:
    /* not allowed */
    void operator =(const File&);
};

class InputFile : public File {
protected:
    InputFile(FileInfo*);
public:
    virtual ~InputFile();

    static InputFile* open(const String& name);

    virtual int read(const char*& start);
};

class StdInput : public InputFile {
public:
    StdInput();
    virtual ~StdInput();

    virtual long length() const;
    virtual int read(const char*& start);
};

#endif
