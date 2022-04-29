/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "parser/nmodl/location.hh"

namespace nmodl {

/**
 * @defgroup token Token Implementation
 * @brief All token related implementation
 *
 * @defgroup token_modtoken Token Classes
 * @ingroup token
 * @brief Represent token classes for various scanners
 * @{
 */

/**
 * \class ModToken
 * \brief Represent token returned by scanner
 *
 * Every token returned by lexer is represented by ModToken.  Some tokens are
 * also externally defined names like \c dt, \c t.  These names are defined in
 * NEURON and hence we set external property to true. Also, location class
 * represent the position of the token in nmodl file. By default location is
 * initialized to <c>line,column</c> as <c>1,1</c>. Some tokens are explicitly
 * added during compiler passes. Hence we set the position to <c>0,0</c> so that we can
 * distinguish them from other tokens produced by lexer.
 *
 * \todo
 *  - \ref LocationType object is copyable except if we specify the stream name.
 *     It would be good to track filename when we go for multi-channel optimization
 *     and code generation.
 *
 * \see
 *  - @ref token_test for how ModToken can be used
 */

class ModToken {
    using LocationType = nmodl::parser::location;

  private:
    /// name of the token
    std::string name;

    /// token value returned by lexer
    int token = -1;

    /// position of token in the mod file
    LocationType pos;

    /// true if token is externally defined variable (e.g. \c t, \c dt in NEURON)
    bool external = false;

  public:
    /// \name Ctor & dtor
    /// \{

    ModToken()
        : pos(nullptr, 0){};

    explicit ModToken(bool ext)
        : pos(nullptr, 0)
        , external(ext) {}

    ModToken(std::string name, int token, LocationType& pos)
        : name(std::move(name))
        , token(token)
        , pos(pos) {}

    /// \}

    /// Return a new instance of token
    ModToken* clone() const {
        return new ModToken(*this);
    }

    /// Return token text from mod file
    std::string text() const {
        return name;
    }

    /// Return token type from lexer
    int type() const {
        return token;
    }

    /// Return line number where token started in the mod file
    int start_line() const {
        return pos.begin.line;
    }

    /// Return start of column number where token appear in the mod file
    int start_column() const {
        return pos.begin.column;
    }

    /**
     * Return position of the token as string
     *
     * This is used used by other passes like scope checker to print the location
     * of token in mod files :
     * \li external token position : `[EXTERNAL]`
     * \li token with unknown position : `[UNKNOWN]`
     * \li token with known position : `[line_num.start_column-end_column]`
     */
    std::string position() const;

    /**
     * Overload \c << operator to print object
     *
     * Overload ostream operator to print token in the form :
     *
     * \code
     *      token at [line.start_column-end_column] type token
     * \endcode
     *
     * For example:
     *
     * \code
     *      v at [118.9-14] type 376
     * \endcode
     */
    friend std::ostream& operator<<(std::ostream& stream, const ModToken& mt);

    /**
     * Overload \c + operator to create an object whose position starts
     * from the start line and column of the first adder and finishes
     * at the end line and column of the second adder
     *
     *
     * For example:
     *
     * \code
     *      a at [118.9-14]
     *      b at [121.4-5]
     * \endcode
     *
     * Output:
     *
     * \code
     *      (a + b) at [118.9-121.5]
     * \endcode
     */
    friend ModToken operator+(ModToken adder1, ModToken adder2);
};

/** @} */  // end of token_modtoken

}  // namespace nmodl
