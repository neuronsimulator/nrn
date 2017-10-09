#ifndef _MODL_TOKEN_H_
#define _MODL_TOKEN_H_

#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>

/* store the location of every token in the file */
class Position {
  public:
    /* this is corresponding to the YYLTYPE structure
     * of the Bison. It stores the location info of
     * every token. This information is filled by Lex.
     */
    int fline;  // first_line
    int lline;  // last_line
    int fcol;   // first_column
    int lcol;   // last_column

    /* some symbols are externals. while showing them
     * in symbol table, instead of showing unknown
     * locations, we can show as an external
     */

    bool external;

    /* @todo: not sure how/when lcol != fcol for lexer (make sense for parser */

    Position() : fline(-1), lline(-1), fcol(-1), lcol(-1), external(false) {
    }

    Position(int fl, int ll, int fc, int lc) : fline(fl), lline(ll), fcol(fc), lcol(lc), external(false) {
    }

    Position(bool ext) : external(ext) {
    }

    int get_first_line() const {
        return fline;
    }

    int get_first_column() const {
        return fcol;
    }

    /* for token on same line, print it as line.firstcol.lastcol => [12.3-5]
     * for token spanning different line, print as first_line-fitst_col.last_line-first_col =>
     * [12-3.13.5]
     */
    friend std::ostream& operator<<(std::ostream& stream, const Position& p) {
        if (p.external) {
            return stream << "[EXTERNAL]";
        } else if (p.fline < 0 || p.lline < 0) {
            return stream << "[UNKNOWN]";
        } else if (p.fline == p.lline) {
            return stream << "[" << p.fline << "." << p.fcol << "-" << p.lcol << "]";
        } else {
            return stream << "[" << p.fline << "-" << p.fcol << "." << p.lline << "-" << p.lcol << "]";
        }
    }

    /** todo : duplicate logic for ostream overload as well */
    std::string string() {
        std::stringstream ss;

        if (external) {
            ss << "[EXTERNAL]";
        } else if (fline < 0 || lline < 0) {
            ss << "[UNKNOWN]";
        } else if (fline == lline) {
            ss << "[" << fline << "." << fcol << "-" << lcol << "]";
        } else {
            ss << "[" << fline << "-" << fcol << "." << lline << "-" << lcol << "]";
        }

        return ss.str();
    }
};

/* every token returned by lexer, mainly value yytext,
 * position information and type of the token.
 *
 * Do we need Position in token? Lexer can be  built as a
 * standalone component and can be run independently with
 * it's test.
 */
class ModToken {
  private:
    std::string value;
    int ktype;
    Position _position;

  public:
    ModToken() : ktype(-1) {
    }

    /** todo: change this */
    ModToken(bool ext) : _position(ext) {
    }

    ModToken(std::string v, int t, Position p) : value(v), ktype(t), _position(p) {
    }

    ModToken(const ModToken& m) {
        value = m.text();
        ktype = m.type();
        _position = m.position();
    }

    /* may not be useful */
    ModToken(std::string v, int t, int fline, int lline, int fcol, int lcol)
        : value(v), ktype(t), _position(fline, lline, fcol, lcol) {
    }

    ModToken* clone() const {
        return new ModToken(*this);
    }

    /* print token as "Token Name at Position with type Type"
     * e.g. Token FUNCTION at [12.3-5] with type 258
     */
    friend std::ostream& operator<<(std::ostream& stream, const ModToken& mt) {
        return stream << std::setw(15) << mt.value << " at " << mt._position << " type " << mt.ktype;
    }

    std::string text() const {
        return value;
    }

    int itext() const {
        // atoi(value.str());
        return 1;
    }

    int type() const {
        return ktype;
    }

    Position position() const {
        return _position;
    }
};

#endif
