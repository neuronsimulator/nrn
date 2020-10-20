#ifndef backtrace_utils_h
#define backtrace_utils_h

#if defined(__cplusplus)
extern "C" {
#endif

extern void backward_wrapper();

extern int parse_bt_symbol(char* backtrace_line, void** addr, char* symbol, char* offset);

extern int cxx_demangle(char* symbol, char** funcname, size_t* funcname_sz);


#if defined(__cplusplus)
}
#endif

#endif // backtrace_utils_h
