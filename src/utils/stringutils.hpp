#ifndef _UTILS_H_
#define _UTILS_H_

#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>
#include <vector>
#include <sstream>

namespace StringUtils {

    /*trim implementations from
     * http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring */

    /* trim from start */
    static inline std::string& ltrim(std::string& s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
        return s;
    }

    /* trim from end */
    static inline std::string& rtrim(std::string& s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
        return s;
    }

    /* trim from both ends */
    static inline std::string& trim(std::string& s) {
        return ltrim(rtrim(s));
    }

    /* remove leading newline for the string read by grammar */
    static inline std::string& trimnewline(std::string& s) {
        s.erase(std::remove(s.begin(), s.end(), '\n'), s.end());
        return s;
    }

    /* for printing json, we have to escape double quotes */
    static inline std::string escape_quotes(const std::string& before) {
        std::string after;

        for (std::string::size_type i = 0; i < before.length(); ++i) {
            switch (before[i]) {
                case '"':
                case '\\':
                    after += '\\';

                default:
                    after += before[i];
            }
        }

        return after;
    }

    inline void split(const std::string& s, char delim, std::vector<std::string>& elems) {
        std::stringstream ss(s);
        std::string item;

        while (std::getline(ss, item, delim)) {
            elems.push_back(item);
        }
    }

    /* spilt string with given delimiter and returns vector */
    inline std::vector<std::string> split_string(const std::string& s, char delim) {
        std::vector<std::string> elems;
        split(s, delim, elems);
        return elems;
    }

    inline void remove_character(std::string& str, const char c) {
        str.erase(std::remove(str.begin(), str.end(), c), str.end());
    }
}  // namespace StringUtils

#endif
