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

/*
 * FieldSEditor -- simple editor for text fields
 */

#ifndef ivlook_sfield_h
#define ivlook_sfield_h

#include <InterViews/input.h>
#include <InterViews/resource.h>

class FieldSEditor;
class FieldSEditorImpl;
class String;
class Style;
class WidgetKit;

class FieldSEditorAction : public Resource {
protected:
    FieldSEditorAction();
    virtual ~FieldSEditorAction();
public:
    virtual void accept(FieldSEditor*);
    virtual void cancel(FieldSEditor*);
};

#if defined(__STDC__) || defined(__ANSI_CPP__)
#define __FieldSEditorCallback(T) T##_FieldSEditorCallback
#define FieldSEditorCallback(T) __FieldSEditorCallback(T)
#define __FieldSEditorMemberFunction(T) T##_FieldSEditorMemberFunction
#define FieldSEditorMemberFunction(T) __FieldSEditorMemberFunction(T)
#else
#define __FieldSEditorCallback(T) T/**/_FieldSEditorCallback
#define FieldSEditorCallback(T) __FieldSEditorCallback(T)
#define __FieldSEditorMemberFunction(T) T/**/_FieldSEditorMemberFunction
#define FieldSEditorMemberFunction(T) __FieldSEditorMemberFunction(T)
#endif

#define declareFieldSEditorCallback(T) \
typedef void (T::*FieldSEditorMemberFunction(T))(FieldSEditor*); \
class FieldSEditorCallback(T) : public FieldSEditorAction { \
public: \
    FieldSEditorCallback(T)( \
	T*, FieldSEditorMemberFunction(T) accept, \
	FieldSEditorMemberFunction(T) cancel \
    ); \
    virtual ~FieldSEditorCallback(T)(); \
\
    virtual void accept(FieldSEditor*); \
    virtual void cancel(FieldSEditor*); \
private: \
    T* obj_; \
    FieldSEditorMemberFunction(T) accept_; \
    FieldSEditorMemberFunction(T) cancel_; \
};

#define implementFieldSEditorCallback(T) \
FieldSEditorCallback(T)::FieldSEditorCallback(T)( \
    T* obj, FieldSEditorMemberFunction(T) accept, \
    FieldSEditorMemberFunction(T) cancel \
) { \
    obj_ = obj; \
    accept_ = accept; \
    cancel_ = cancel; \
} \
\
FieldSEditorCallback(T)::~FieldSEditorCallback(T)() { } \
\
void FieldSEditorCallback(T)::accept(FieldSEditor* f) { (obj_->*accept_)(f); } \
void FieldSEditorCallback(T)::cancel(FieldSEditor* f) { (obj_->*cancel_)(f); }

class FieldSEditor : public InputHandler {
public:
    FieldSEditor(
	const String& sample, WidgetKit*, Style*, FieldSEditorAction* = NULL
    );
    virtual ~FieldSEditor();

    virtual void undraw();

    virtual void press(const Event&);
    virtual void drag(const Event&);
    virtual void release(const Event&);
    virtual void keystroke(const Event&);
    virtual InputHandler* focus_in();
    virtual void focus_out();

    virtual void field(const char*);
    virtual void field(const String&);

    virtual void select(int pos);
    virtual void select(int left, int right);

    virtual void selection(int& start, int& index) const;

    virtual void edit();
    virtual void edit(const char*, int left, int right);
    virtual void edit(const String&, int left, int right);

    virtual const String* text() const;
private:
    FieldSEditorImpl* impl_;
};

#endif
