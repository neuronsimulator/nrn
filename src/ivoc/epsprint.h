#ifndef epsprinter_h
#define epsprinter_h

#include <InterViews/printer.h>

class EPSPrinter: public Printer {
  public:
    EPSPrinter(std::ostream*);
    virtual ~EPSPrinter();

    virtual void eps_prolog(std::ostream&,
                            Coord width,
                            Coord height,
                            const char* creator = "InterViews");
};

#endif
