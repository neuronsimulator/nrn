#pragma once
#if __has_include(<cxxabi.h>)
#include <cxxabi.h>
#endif

#include <cstddef>
#include <memory>
#include <string>

int parse_bt_symbol(char* backtrace_line, void** addr, char* symbol, char* offset);
int cxx_demangle(const char* symbol, char** funcname, std::size_t* funcname_sz);
/** @brief Try and demangle a string, return the original string on failure.
 *
 *  MSVC type_info::name() is already readable but includes a keyword prefix
 *  (struct/class/enum/union). Itanium cxa_demangle does not; get_name_impl
 *  strips neuron::container:: only as a prefix.
 */
inline std::string cxx_demangle(const char* mangled) {
    std::string name;
#if __has_include(<cxxabi.h>)
    int status{};
    // Note that the third argument to abi::__cxa_demangle returns the length of
    // the allocated buffer, which may be larger than strlen(demangled) + 1.
    std::unique_ptr<char, decltype(free)*> demangled{
        abi::__cxa_demangle(mangled, nullptr, nullptr, &status), free};
    name = status ? mangled : demangled.get();
#else
    name = mangled ? mangled : "";
#endif
    constexpr char const* keywords[] = {"struct ", "class ", "enum ", "union "};
    for (auto keyword: keywords) {
        auto const n = std::char_traits<char>::length(keyword);
        if (name.compare(0, n, keyword) == 0) {
            name.erase(0, n);
            break;
        }
    }
    return name;
}

void backward_wrapper();
