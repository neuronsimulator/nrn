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
 */
inline std::string cxx_demangle(const char* mangled) {
#if __has_include(<cxxabi.h>)
    int status{};
    // Note that the third argument to abi::__cxa_demangle returns the length of
    // the allocated buffer, which may be larger than strlen(demangled) + 1.
    std::unique_ptr<char, decltype(free)*> demangled{
        abi::__cxa_demangle(mangled, nullptr, nullptr, &status), free};
    return status ? mangled : demangled.get();
#else
    return mangled;
#endif
}

void backward_wrapper();
