#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// forward declarations (c++) and opaque c types
typedef struct Symbol Symbol;
typedef struct Object Object;
typedef struct Section Section;
typedef struct SectionListIterator SectionListIterator;
typedef struct NrnListItem NrnListItem;
typedef struct NrnListItem NrnList;
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

/// @brief Type for Arguments
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

/// @brief Type of method results
typedef enum {
    NRN_ERR = -1,
    NRN_VOID,
    NRN_DOUBLE,
    NRN_STRING,
    NRN_OBJECT,
} NrnResultType;

/** @brief A result holder.
 * Result type can be directly inspected but to obtain the value please use the respective method
 */
typedef struct {
    /// The type of the returned data
    NrnResultType type;
    /// The actual value. Refer to accessor functions `nrn_result_get_xxx`
    union {
        double d;
        Object* o;
        char** s;
    };
} NrnResult;

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
double nrn_section_length_get(Section* sec);
double nrn_section_Ra_get(Section* sec);
void nrn_section_Ra_set(Section* sec, double val);
char const* nrn_secname(Section* sec);
void nrn_section_push(Section* sec);
void nrn_section_pop(void);
void nrn_mechanism_insert(Section* sec, const Symbol* mechanism);
NrnList* nrn_allsec(void);
NrnList* nrn_sectionlist_data(Object* obj);

/****************************************
 * Segments
 ****************************************/
int nrn_nseg_get(const Section* sec);
void nrn_nseg_set(Section* sec, int nseg);
void nrn_segment_diam_set(Section* sec, double x, double diam);
RangeVar nrn_rangevar_new(Section*, double x, const char* var_name);
double nrn_rangevar_get(Section* sec, double x, const char* var_name);
void nrn_rangevar_set(Section* sec, double x, const char* var_name, double value);

/****************************************
 * Functions, objects, and the stack
 ****************************************/

/// @brief The class name of an object
char const* nrn_class_name(Object const* obj);

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
NrnResult nrn_function_call(const char* func_name, const char* format, ...);

/**
 * @brief Create a new object given the "Class" Symbol and its arguments
 * @param format The argument types, given as a format string. See NRN_ARG_XXX defines
 * @return The created object. On error returns NULL and `nrn_stack_err()` is set.
 */
Object* nrn_object_new(const char* cls_name, const char* format, ...);

/**
 * @brief Call a method, given the object, method name and arguments.
 * On error `nrn_stack_err()` is set.
 * @param format The argument types, given as a format string. See NRN_ARG_XXX defines
 */
NrnResult nrn_method_call(Object* obj, const char* method_name, const char* format, ...);

/// Expose a sort of default constructor, sligthly more efficient
Object* nrn_object_new_NoArgs(const char* cls_name);

// Methods may return void, double, string or objects
// The user should pop from the stack accordingly
int nrn_symbol_type(const Symbol* sym);

/// Increment the object reference count
void nrn_object_incref(Object* obj);
/// Decrement the object reference count
void nrn_object_decref(Object* obj);

/// @brief Gets the double from a result. Aborts if wrong type.
double nrn_result_get_double(NrnResult* r);
/// @brief Gets the object pointer from a result. Aborts if wrong type.
Object* nrn_result_get_object(NrnResult* r);
/// @brief Gets the string from a result. Aborts if wrong type.
const char* nrn_result_get_string(NrnResult* r);

/// @brief Decreses reference counts properly, according to inner type
/// In case of objects, similar to nrn_object_decref(nrn_result_get_object(Result*))
void nrn_result_drop(NrnResult* r);

/****************************************
 * Miscellaneous
 ****************************************/
int nrn_hoc_call(char const* command);
SectionListIterator* nrn_sectionlist_iterator_new(NrnListItem* my_sectionlist);
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
