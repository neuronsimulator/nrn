#ifndef _LEXER_MODTOKEN_HPP_
#define _LEXER_MODTOKEN_HPP_

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "parser/location.hh"

/**
 * \class ModToken
 * \brief Represent token return by scanner
 *
 * Every token returned by lexer is represented by ModToken.
 * Some tokens are also externally defined names like dt, t.
 * These names are defined in NEURON and hence we set external
 * property to true. Also, location class represent the position
 * of the token in nmodl file. By default location is initialized
 * to line,column as 1,1. Some tokens are explicitly added during
 * compiler passes. Hence we set the position to 0,0 so that we
 * can distinguish them from other tokens produced by lexer.
 *
 * \todo location object is copyable except if we specify the
 * stream name. It would be good to track filename when we go
 * for multi-channel optimization and code generation.
 */

class ModToken {
  private:
    /// name of the token
    std::string name;

    /// token value returned by lexer
    int token = -1;

    /// if token is externally defined symbol
    bool external = false;

    /// position of the token in mod file
    nmodl::location pos;

  public:
    ModToken() : pos(nullptr, 0){};

    explicit ModToken(bool ext) : pos(nullptr, 0), external(ext) {
    }

    ModToken(std::string str, int tok, nmodl::location& loc) : name(str), token(tok), pos(loc) {
    }

    ModToken* clone() const {
        return new ModToken(*this);
    }

    std::string text() const {
        return name;
    }

    int type() const {
        return token;
    }

    int start_line() const {
        return pos.begin.line;
    }

    int start_column() const {
        return pos.begin.column;
    }

    std::string position() const;

    friend std::ostream& operator<<(std::ostream& stream, const ModToken& mt);

    std::string to_string() const;
};

#endif
