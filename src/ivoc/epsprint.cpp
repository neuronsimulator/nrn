#include <../../nrnconf.h>
#if HAVE_IV  // to end of file

#include "epsprint.h"

// ps_prolog copied from InterViews's printer.cpp
static const char* ps_prolog =
    "\
save 20 dict begin\n\
\n\
/sf {   % scale /fontName => -  (set current font)\n\
    {findfont} stopped {pop /Courier findfont} if\n\
    exch scalefont setfont\n\
} def\n\
\n\
/ws {\n\
    4 index 6 4 roll moveto sub\n\
    2 index stringwidth pop sub\n\
    exch div 0 8#40 4 3 roll\n\
    widthshow\n\
} def\n\
\n\
/as {\n\
    4 index 6 4 roll moveto sub\n\
    2 index stringwidth pop sub\n\
    exch div 0 3 2 roll\n\
    ashow\n\
} def\n\
\n\
";

EPSPrinter::EPSPrinter(std::ostream* o)
    : Printer(o) {}

EPSPrinter::~EPSPrinter() {}

void EPSPrinter::eps_prolog(std::ostream& out, Coord width, Coord height, const char* creator) {
    int bbw = int(width);
    int bbh = int(height);
    // need to describe it as EPSF = "encapsulated postscript"
    out << "%!PS-Adobe-2.0 EPSF-1.2\n";

    out << "%%Creator: " << creator << "\n";
    out << "%%Pages: atend\n";

    // adding a bounding box makes this "encapsulated"
    // bbw and bbh are the width and height of the bounding box 1/72 of an inch
    out << "%%BoundingBox: 0 0 " << bbw << " " << bbh << "\n";

    out << "%%EndComments\n";
    out << ps_prolog;
    out << "%%EndProlog\n";
}

#endif
