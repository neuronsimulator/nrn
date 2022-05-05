#ifndef epsprinter_h
#define epsprinter_h

#include <InterViews/printer.h>

class EPSPrinter: public Printer {
  public:
    EPSPrinter(ostream*);
    virtual ~EPSPrinter();

    virtual void eps_prolog(ostream&,
                            Coord width,
                            Coord height,
                            const char* creator = "InterViews");
};

#endif
