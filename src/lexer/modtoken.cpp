#include "lexer/modtoken.hpp"

/** Return position of the token as string. This is used used by other
 * passes like scope checker to print the location in mod files.
 * External token's position : [EXTERNAL]
 * Token with unknown position : [UNKNOWN]
 * Token from lexer with known position : [line.start_column-end_column] */
std::string ModToken::position() const {
    std::stringstream ss;
    if (external) {
        ss << "[EXTERNAL]";
    } else if (start_line() == 0) {
        ss << "[UNKNOWN]";
    } else {
        ss << "[" << pos << "]";
    }
    return ss.str();
}

/** Print token as : token at [line.start_column-end_column] type token
 * Example: v at [118.9-14] type 376 */
std::ostream& operator<<(std::ostream& stream, const ModToken& mt) {
    stream << std::setw(15) << mt.name << " at " << mt.position();
    return stream << " type " << mt.token;
}

std::string ModToken::to_string() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}