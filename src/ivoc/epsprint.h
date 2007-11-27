#ifndef epsprinter_h
#define epsprinter_h

#ifdef CYGWIN
#ifndef WIN32
#define WIN32 1
#endif
#endif

#include <InterViews/printer.h>

class EPSPrinter : public Printer {
public:
	EPSPrinter(ostream*);
	virtual ~EPSPrinter();
	
	virtual void eps_prolog(ostream&, Coord width, Coord height,
		const char* creator = "InterViews");
};

#endif
