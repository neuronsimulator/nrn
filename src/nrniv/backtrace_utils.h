#pragma once
#include <cstddef>

int parse_bt_symbol(char* backtrace_line, void** addr, char* symbol, char* offset);
int cxx_demangle(char* symbol, char** funcname, std::size_t* funcname_sz);
void backward_wrapper();
