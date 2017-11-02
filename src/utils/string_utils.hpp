#pragma once

#include <algorithm>
#include <sstream>
#include <vector>

/**
 * \brief String manipulation functions
 *
 * String trimming and manipulation functions based on
 * stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
 */

namespace stringutils {

    /// Trim from start
    static inline std::string& ltrim(std::string& s) {
        s.erase(s.begin(),
                std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
        return s;
    }

    /// Trim from end
    static inline std::string& rtrim(std::string& s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace)))
                    .base(),
                s.end());
        return s;
    }

    /// Trim from both ends
    static inline std::string& trim(std::string& s) {
        return ltrim(rtrim(s));
    }

    inline void remove_character(std::string& str, const char c) {
        str.erase(std::remove(str.begin(), str.end(), c), str.end());
    }

    /// Remove leading newline for the string read by grammar
    static inline std::string& trimnewline(std::string& s) {
        remove_character(s, '\n');
        return s;
    }

    /// for printing json, we have to escape double quotes
    static inline std::string escape_quotes(const std::string& before) {
        std::string after;

        for (auto c : before) {
            switch (c) {
                case '"':
                case '\\':
                    after += '\\';
                    /// don't break here as we want to append actual character

                default:
                    after += c;
            }
        }

        return after;
    }

    /// Spilt string with given delimiter and returns vector
    inline std::vector<std::string> split_string(const std::string& s, char delim) {
        std::vector<std::string> elems;
        std::stringstream ss(s);
        std::string item;

        while (std::getline(ss, item, delim)) {
            elems.push_back(item);
        }

        return elems;
    }

}    // namespace stringutils
