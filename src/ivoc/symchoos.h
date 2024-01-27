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

/* hacked by Michael Hines from fchooser.h */
/*
 * SymChooser -- select a Symbol
 */

#ifndef symchooser_h
#define symchooser_h

#include <InterViews/dialog.h>
#include <InterViews/resource.h>

#include <InterViews/_enter.h>

class SymChooser;
class SymChooserImpl;
class String;
class WidgetKit;
class SymDirectory;

class SymChooserAction: public Resource {
  protected:
    SymChooserAction();
    virtual ~SymChooserAction();

  public:
    virtual void execute(SymChooser*, bool accept);
};

class SymChooser: public Dialog {
  public:
    SymChooser(SymDirectory*, WidgetKit*, Style*, SymChooserAction* = NULL, int nbrowser = 3);
    virtual ~SymChooser();

    virtual const std::string& selected() const;
    virtual double* selected_var();
    virtual int selected_vector_count();
    virtual void reread();
    virtual void dismiss(bool);

  private:
    SymChooserImpl* impl_;
};

#include <InterViews/_leave.h>

#endif
