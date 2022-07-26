#include "backtrace_utils.h"

#ifdef USE_BACKWARD
#include "backward.hpp"
#endif

#if __has_include(<cxxabi.h>)
#define USE_CXXABI
#include <cxxabi.h>
#endif

#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>
#include <regex>
#include <string>

/*
 * Small utility to bring C++ symbol demangling to C
 */
int parse_bt_symbol(char* backtrace_line, void** addr, char* symbol, char* offset) {
#ifndef _WIN32
#ifdef __APPLE__
    std::regex btline("(\\d+)\\s+([\\w\\.]+)\\s+(0x[\\da-f]+)\\s+(\\w+)\\s+\\+\\s+(\\d+)");
#define ADDR   3
#define SYMBOL 4
#define OFFSET 5
#define OBJPOS 4
#elif __linux__
    std::regex btline("([\\w\\.\\/]+)\\((\\w*)\\+(0x[\\da-f]+)\\)\\s+\\[(0x[\\da-f]+)\\]");
#define ADDR   4
#define SYMBOL 2
#define OFFSET 3
#define OBJPOS 2
#endif
    std::cmatch backtrace_match;
    if (std::regex_search(backtrace_line, backtrace_match, btline)) {
        *addr = reinterpret_cast<void*>(std::stoul(backtrace_match[ADDR].str(), nullptr, 16));
        std::strcpy(symbol, backtrace_match[SYMBOL].str().c_str());
        std::strcpy(offset, backtrace_match[OFFSET].str().c_str());
        backtrace_line[backtrace_match.position(OBJPOS) - 1] = '\0';
        return 1;
    }
#endif
    return 0;
}

int cxx_demangle(const char* symbol, char** funcname, size_t* funcname_sz) {
#if __has_include(<cxxabi.h>)
    int status = 0;
    char* ret = abi::__cxa_demangle(symbol, *funcname, funcname_sz, &status);
    *funcname = ret;
    return status;
#else
    // return something non-zero to indicate failure
    // cxa_demangle can return 3 error codes: -1, -2, -3, we extend by one more error code
    return -4;
#endif
}

namespace {
struct free_deleter {
    void operator()(char* p) {
        free(p);
    }
};
}  // namespace
std::string cxx_demangle(const char* mangled) {
    char* demangled{nullptr};
    std::size_t demangled_size{};
    auto const status = cxx_demangle(mangled, &demangled, &demangled_size);
    std::unique_ptr<char, free_deleter> p{demangled};
    if (status != 0) {
        // There was an error
        return mangled;
    }
    // demangled_size gives the buffer length, which is 1 more than the
    // string length
    assert(demangled_size);
    return {demangled, demangled + demangled_size - 1};
}

void backward_wrapper() {
#ifdef USE_BACKWARD
    backward::StackTrace st;
    st.load_here(12);
    backward::Printer p;
    p.print(st);
#endif
}
