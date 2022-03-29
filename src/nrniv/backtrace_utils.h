#ifndef backtrace_utils_h
#define backtrace_utils_h

#if defined(__cplusplus)
extern "C" {
#endif

extern int parse_bt_symbol(char* backtrace_line, void** addr, char* symbol, char* offset);

extern int cxx_demangle(char* symbol, char** funcname, size_t* funcname_sz);

extern void backward_wrapper();


#if defined(__cplusplus)
}
#endif

#endif  // backtrace_utils_h
