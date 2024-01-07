#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// forward declarations (c++) and opaque c types
typedef struct Symbol Symbol;
typedef struct Object Object;
typedef struct Section Section;
typedef struct SectionListIterator SectionListIterator;
typedef struct nrn_Item nrn_Item;
typedef struct SymbolTableIterator SymbolTableIterator;
typedef struct Symlist Symlist;
typedef struct Segment Segment;

typedef struct {
    Segment* node;
    double x;
    Symbol* sym;
} RangeVar;

/// Track errors. NULL -> No error, otherwise a description of it
const char* nrn_stack_err();

typedef enum {
    STACK_IS_STR = 1,
    STACK_IS_VAR = 2,
    STACK_IS_NUM = 3,
    STACK_IS_OBJVAR = 4,
    STACK_IS_OBJTMP = 5,
    STACK_IS_USERINT = 6,
    STACK_IS_SYM = 7,
    STACK_IS_OBJUNREF = 8,
    STACK_UNKNOWN = -1
} nrn_stack_types_t;

// char* is really handy as literals have the \0 sentinel
// and automatically join when mentioned together, e.g. NRN_ARG_DOUBLE NRN_ARG_STR
#define NRN_NO_ARGS        ""
#define NRN_ARG_DOUBLE     "d"
#define NRN_ARG_DOUBLE_PTR "D"
#define NRN_ARG_STR_PTR    "S"
#define NRN_ARG_SYMBOL     "y"
#define NRN_ARG_OBJECT     "o"
#define NRN_ARG_INT        "i"
#define NRN_ARG_RANGEVAR   "r"

/****************************************
 * Initialization
 ****************************************/
int nrn_init(int argc, const char** argv);
void nrn_stdout_redirect(int (*myprint)(int, char*));

/****************************************
 * Sections
 ****************************************/
Section* nrn_section_new(char const* name);
void nrn_section_connect(Section* child_sec, double child_x, Section* parent_sec, double parent_x);
void nrn_section_length_set(Section* sec, double length);
double nrn_section_length_get(Section const* sec);
void nrn_section_Ra_set(Section* sec, double val);
char const* nrn_secname(Section* sec);
void nrn_section_push(Section* sec);
void nrn_section_pop(void);
void nrn_mechanism_insert(Section* sec, Symbol* mechanism);
nrn_Item* nrn_allsec(void);
nrn_Item* nrn_sectionlist_data(Object* obj);

/****************************************
 * Segments
 ****************************************/
int nrn_nseg_get(Section const* sec);
void nrn_nseg_set(Section* sec, int nseg);
void nrn_segment_diam_set(Section* sec, double x, double diam);
RangeVar nrn_rangevar_new(Section*, double x, const char* var_name);
double nrn_rangevar_get(Symbol* sym, Section* sec, double x);
void nrn_rangevar_set(Symbol* sym, Section* sec, double x, double value);

/****************************************
 * Functions, objects, and the stack
 ****************************************/

/// @brief Looks up a top-level symbol by name. Returns NULL if it doesn't exist
Symbol* nrn_symbol(char const* name);
/// @brief Looks up an object's method symbol by name. Returns NULL if it doesn't exist
Symbol* nrn_method_symbol(Object* obj, char const* name);

/**
 * @brief Invokes the execution of an interpreter function, given its name and arguments
 * @param func_name The name of the function to be called
 * @param format The argument types, given as a format string. See NRN_ARG_XXX defines
 * @param ... The arguments themselves
 * @return The returned value (double). On error returns -1 and `nrn_stack_err()` is set.
 */
double nrn_function_call(const char* func_name, const char* format, ...);

/**
 * @brief Create a new object given the "Class" Symbol and its arguments
 * @param format The argument types, given as a format string. See NRN_ARG_XXX defines
 * @return The created object. On error returns NULL and `nrn_stack_err()` is set.
 */
Object* nrn_object_new(const char* cls_name, const char* format, ...);

/**
 * @brief Call a method, given the object, method name and arguments
 * @param format The argument types, given as a format string. See NRN_ARG_XXX defines
 * @return 1 on suceess. On error returns 0 and `nrn_stack_err()` is set.
 */
int nrn_method_call(Object* obj, const char* method_name, const char* format, ...);

/// @brief Calls a function which expects a single double value
double nrn_function_call_d(const char* func_name, double v);
/// @brief Calls a function which expects a single string (char*) value
double nrn_function_call_s(const char* func_name, const char* v);
/// @brief Calls a constructor which requires no arguments. Avoids any argument handling
Object* nrn_object_new_NoArgs(const char* method_name);
/// @brief Calls a constructor which expects a single double value
Object* nrn_object_new_d(const char* cls_name, double v);
/// @brief Calls a constructor which expects a single string (char*) value
Object* nrn_object_new_s(const char* cls_name, const char* v);
/// @brief Calls a method which expects a single double value
int nrn_method_call_d(Object* obj, const char* method_name, double v);
/// @brief Calls a method which expects a single string (char*) value
int nrn_method_call_s(Object* obj, const char* method_name, const char* v);

// Methods may return void, double, string or objects
// The user should pop from the stack accordingly
nrn_stack_types_t nrn_stack_type(void);
char const* const nrn_stack_type_name(nrn_stack_types_t id);
int nrn_symbol_type(const Symbol* sym);
double* (*nrn_get_symbol_ptr)(Symbol* sym);
double nrn_double_pop(void);
double* nrn_double_ptr_pop(void);
char** nrn_pop_str(void);
int nrn_int_pop(void);
Object* nrn_object_pop(void);

void nrn_object_ref(Object* obj);
void nrn_object_unref(Object* obj);
char const* nrn_class_name(Object const* obj);

/****************************************
 * Miscellaneous
 ****************************************/
int nrn_hoc_call(char const* command);
SectionListIterator* nrn_sectionlist_iterator_new(nrn_Item* my_sectionlist);
void nrn_sectionlist_iterator_free(SectionListIterator* sl);
Section* nrn_sectionlist_iterator_next(SectionListIterator* sl);
int nrn_sectionlist_iterator_done(SectionListIterator* sl);
SymbolTableIterator* nrn_symbol_table_iterator_new(Symlist* my_symbol_table);
void nrn_symbol_table_iterator_free(SymbolTableIterator* st);
char const* nrn_symbol_table_iterator_next(SymbolTableIterator* st);
int nrn_symbol_table_iterator_done(SymbolTableIterator* st);
int nrn_vector_size(const Object* vec);
double* nrn_vector_data(Object* vec);
double nrn_property_get(const Object* obj, const char* name);
double nrn_property_array_get(const Object* obj, const char* name, int i);
void nrn_property_set(Object* obj, const char* name, double value);
void nrn_property_array_set(Object* obj, const char* name, int i, double value);
void nrn_property_push(Object* obj, const char* name);
void nrn_property_array_push(Object* obj, const char* name, int i);
char const* nrn_symbol_name(const Symbol* sym);
Symlist* nrn_symbol_table(Symbol* sym);
Symlist* nrn_global_symbol_table(void);
// TODO: need shapeplot information extraction

#ifdef __cplusplus
}
#endif
