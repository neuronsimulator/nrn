#pragma once
#include "neuron/container/data_handle.hpp"
#include "neuron/container/generic_data_handle.hpp"

#include <cstdio>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
/**
 * \dir
 * \brief HOC Interpreter
 *
 * \file
 * \brief HOC interpreter function declarations (included by hocdec.h)
 */

/**
 * @defgroup HOC HOC Interpreter
 * @brief All HOC interpreter related implementation details
 *
 * @defgroup hoc_functions HOC Functions
 * @ingroup HOC
 * @brief All hoc functions used in the NEURON codebase
 * @{
 */

struct Arrayinfo;
struct cTemplate;
struct DoubScal;
struct DoubVec;
struct HocSymExtension;
class IvocVect;
struct Object;
union Objectdata;
struct Symbol;
struct Symlist;
struct VoidFunc;

namespace neuron {
struct model_sorted_token;
}

// nocpout.cpp
Symbol* hoc_get_symbol(const char* var);
void hoc_register_var(DoubScal*, DoubVec*, VoidFunc*);
void ivoc_help(const char*);

Symbol* hoc_lookup(const char*);

void* hoc_Ecalloc(std::size_t nmemb, std::size_t size);
void* hoc_Emalloc(size_t size);
void hoc_malchk();
[[noreturn]] void hoc_execerror(const char*, const char*);
[[noreturn]] void hoc_execerr_ext(const char* fmt, ...);
char* hoc_object_name(Object*);
void hoc_retpushx(double);

namespace neuron::oc {
struct runtime_error: ::std::runtime_error {
    using ::std::runtime_error::runtime_error;
};
/**
 * @brief Execute C++ code that may throw and propagate HOC information.
 *
 * Low level C++ code called from HOC/Python may throw exceptions that do not carry any information
 * about the HOC/Python expression that generated the exception. This can lead to unhelpful error
 * messages if the exception is caught high up the stack. This wrapper is designed to be used at the
 * points where we leave the HOC world and call lower level code. If that lower level code throws an
 * exception, the message will be passed to hoc_execerror. This saves additional information so that
 * the final error message provides additional context. If the "lower level" code called
 * hoc_execerror itself then we pass on the exception without re-calling hoc_execerror.
 */
template <typename Callable, typename... Args>
decltype(auto) invoke_method_that_may_throw(Callable message_prefix, Args&&... args) {
    try {
        return std::invoke(std::forward<Args>(args)...);
    } catch (runtime_error const&) {
        // This message was thrown by hoc_execerror; just pass it on
        throw;
    } catch (std::exception const& e) {
        std::string message{message_prefix()};
        std::string_view what{e.what()};
        if (!what.empty()) {
            message.append(": ");
            message.append(what);
        }
        hoc_execerror(message.c_str(), nullptr);
    }
}
}  // namespace neuron::oc

double* hoc_getarg(int);
int ifarg(int);

int vector_instance_px(void*, double**);
void install_vector_method(const char*, double (*)(void*));

int vector_arg_px(int i, double** p);

double hoc_Exp(double);
int hoc_is_tempobj_arg(int narg);
std::FILE* hoc_obj_file_arg(int i);
void hoc_reg_nmodl_text(int type, const char* txt);
void hoc_reg_nmodl_filename(int type, const char* filename);
std::size_t nrn_mallinfo(int item);
int nrn_mlh_gsort(double* vec, int* base_ptr, int total_elems, int (*cmp)(double, double));
void state_discontinuity(int i, double* pd, double d);

IvocVect* vector_arg(int);
int vector_buffer_size(IvocVect*);
int vector_capacity(IvocVect*);
IvocVect* vector_new(int, Object* = nullptr);
IvocVect* vector_new0();
IvocVect* vector_new1(int);
IvocVect* vector_new2(IvocVect*);
Object** vector_pobj(IvocVect*);
void vector_resize(IvocVect*, int);
double* vector_vec(IvocVect*);

// olupton 2022-01-21: These overloads are added for backwards compatibility
//                     with pre-C++ mechanisms.
[[deprecated("non-void* overloads are preferred")]] int vector_buffer_size(void*);
[[deprecated("non-void* overloads are preferred")]] int vector_capacity(void*);
[[deprecated("non-void* overloads are preferred")]] Object** vector_pobj(void*);
[[deprecated("non-void* overloads are preferred")]] void vector_resize(void*, int);
[[deprecated("non-void* overloads are preferred")]] double* vector_vec(void*);

extern int nrnignore;

/**
 * \brief Brief explanation of hoc_obj_run
 *
 * Detailed explanation of hoc_obj_run goes here.
 */
int hoc_obj_run(const char*, Object*);

void hoc_prstack();
int hoc_argtype(int);
int hoc_is_double_arg(int);
int hoc_is_pdouble_arg(int);
int hoc_is_str_arg(int);
int hoc_is_object_arg(int);
char* hoc_gargstr(int);
char** hoc_pgargstr(int);

Object** hoc_objgetarg(int);
Object* hoc_name2obj(const char* name, int index);

char** hoc_temp_charptr();
int hoc_is_temp_charptr(char** cpp);
void hoc_assign_str(char** pstr, const char* buf);
double chkarg(int, double low, double high);
// push first arg first. Warning: if the function is inside an object make sure
// you know what you are doing.
double hoc_call_func(Symbol*, int narg);
// call a fuction within the context of an object.
double hoc_call_objfunc(Symbol*, int narg, Object*);
extern double hoc_ac_;
extern double hoc_epsilon;
extern int nrn_inpython_;

extern int hoc_color;
int hoc_set_color(int);
void hoc_plt(int, double, double);
void hoc_plprint(const char*);
void hoc_ret(); /* but need to push before returning */

void hoc_push(neuron::container::generic_data_handle handle);
template <typename T>
void hoc_push(neuron::container::data_handle<T> const& handle) {
    hoc_push(neuron::container::generic_data_handle{handle});
}
void hoc_pushx(double);
void hoc_pushstr(char**);
void hoc_pushobj(Object**);
void hoc_push_object(Object*);
void hoc_pushpx(double*);
void hoc_pushs(Symbol*);
void hoc_pushi(int);
void hoc_push_ndim(int);
int hoc_pop_ndim();
int hoc_stack_type();
bool hoc_stack_type_is_ndim();

namespace neuron::oc::detail {
template <typename>
struct hoc_get_arg_helper;
template <typename>
struct hoc_pop_helper;
}  // namespace neuron::oc::detail

/** @brief Pop an object of type T from the HOC stack.
 *
 *  The helper type neuron::oc::detail::hoc_pop must be specialised for all
 *  supported (families of) T.
 */
template <typename T>
T hoc_pop() {
    return neuron::oc::detail::hoc_pop_helper<T>::impl();
}

/** @brief Get the nth (1..N) argument on the stack.
 *
 *  This is a templated version of hoc_getarg, hoc_pgetarg and friends.
 *
 *  @todo Should the stack be modified such that this can return const
 *  references, even for things like data_handle<T> that at the moment are not
 *  exactly stored (we store generic_data_handle, which can produce a
 *  data_handle<T> on demand but which does not, at present, actually *contain*
 *  a data_handle<T>)?
 */
template <typename T>
[[nodiscard]] T hoc_get_arg(std::size_t narg) {
    return neuron::oc::detail::hoc_get_arg_helper<T>::impl(narg);
}

namespace neuron::oc::detail {
template <>
struct hoc_get_arg_helper<neuron::container::generic_data_handle> {
    static neuron::container::generic_data_handle impl(std::size_t);
};
template <typename T>
struct hoc_get_arg_helper<neuron::container::data_handle<T>> {
    static neuron::container::data_handle<T> impl(std::size_t narg) {
        return neuron::container::data_handle<T>{
            hoc_get_arg<neuron::container::generic_data_handle>(narg)};
    }
};
template <>
struct hoc_pop_helper<neuron::container::generic_data_handle> {
    /** @brief Pop a generic data handle from the HOC stack.
     *
     *  This function is not used from translated MOD files, so we can assume
     *  that the generic_data_handle is not in permissive mode.
     */
    static neuron::container::generic_data_handle impl();
};

template <typename T>
struct hoc_pop_helper<neuron::container::data_handle<T>> {
    /** @brief Pop a data_handle<T> from the HOC stack.
     */
    static neuron::container::data_handle<T> impl() {
        return neuron::container::data_handle<T>{hoc_pop<neuron::container::generic_data_handle>()};
    }
};
}  // namespace neuron::oc::detail

[[nodiscard]] inline double* hoc_pgetarg(int narg) {
    return static_cast<double*>(hoc_get_arg<neuron::container::data_handle<double>>(narg));
}

double hoc_xpop();
Symbol* hoc_spop();
double* hoc_pxpop();
Object** hoc_objpop();
struct TmpObjectDeleter {
    void operator()(Object*) const;
};
using TmpObject = std::unique_ptr<Object, TmpObjectDeleter>;
TmpObject hoc_pop_object();
char** hoc_strpop();
int hoc_ipop();
void hoc_nopop();

/** @brief Shorthand for hoc_pop<data_handle<T>>().
 */
template <typename T>
neuron::container::data_handle<T> hoc_pop_handle() {
    return hoc_pop<neuron::container::data_handle<T>>();
}

/**
 * @brief Shorthand for hoc_get_arg<data_handle<T>>(narg).
 *
 * Migrating code to be data_handle-aware typically involves replacing hoc_pgetarg with
 * hoc_hgetarg<double>.
 */
template <typename T>
[[nodiscard]] neuron::container::data_handle<T> hoc_hgetarg(int narg) {
    return hoc_get_arg<neuron::container::data_handle<T>>(narg);
}

[[noreturn]] void hoc_execerror_mes(const char*, const char*, int);
void hoc_warning(const char*, const char*);
double* hoc_val_pointer(const char*);
neuron::container::data_handle<double> hoc_val_handle(std::string_view);
Symbol* hoc_table_lookup(const char*, Symlist*);
Symbol* hoc_install(const char*, int, double, Symlist**);
extern Objectdata* hoc_objectdata;
/** @brief Get the stack entry at depth i.
 *
 *  i=0 is the most recently pushed entry. This will raise an error if the stack
 *  is empty, or if the given entry does not have type T.
 */
template <typename T>
T const& hoc_look_inside_stack(int i);
Object* hoc_obj_look_inside_stack(int);
void hoc_stkobj_unref(Object*, int stkindex);
size_t hoc_total_array_data(Symbol*, Objectdata*);
char* hoc_araystr(Symbol*, int, Objectdata*);

char* hoc_object_pathname(Object*);
const char* expand_env_var(const char*);
void check_obj_type(Object*, const char*);
int is_obj_type(Object*, const char*);
void hoc_obj_ref(Object*);   /* NULL allowed */
void hoc_obj_unref(Object*); /* NULL allowed */
void hoc_dec_refcount(Object**);
Object** hoc_temp_objvar(Symbol* template_symbol, void* cpp_object);
Object** hoc_temp_objptr(Object*);
Object* hoc_new_object(Symbol* symtemp, void* v);
void hoc_new_object_asgn(Object** obp, Symbol* template_symbol, void* cpp_object);
HocSymExtension* hoc_var_extra(const char*);
double check_domain_limits(float*, double);
Object* hoc_obj_get(int i);
void hoc_obj_set(int i, Object*);
void nrn_hoc_lock();
void nrn_hoc_unlock();

void* hoc_Erealloc(void* ptr, std::size_t size);

void* nrn_cacheline_alloc(void** memptr, std::size_t size);
void* nrn_cacheline_calloc(void** memptr, std::size_t nmemb, std::size_t size);
[[noreturn]] void nrn_exit(int);
void hoc_free_list(Symlist**);
int hoc_errno_check();
Symbol* hoc_parse_stmt(const char*, Symlist**);
void hoc_run_stmt(Symbol*);
Symbol* hoc_parse_expr(const char*, Symlist**);
double hoc_run_expr(Symbol*);
void hoc_free_string(char*);
int hoc_xopen1(const char*, const char*);
int hoc_xopen_run(Symbol*, const char*);
void hoc_symbol_limits(Symbol*, float, float);
void sym_extra_alloc(Symbol*);
int hoc_chdir(const char* path);

void hoc_final_exit();
void hoc_sprint1(char**, int);
double hoc_scan(std::FILE*);
char* hoc_symbol_units(Symbol* sym, const char* units);
void hoc_fake_call(Symbol*);
void hoc_last_init();
void hoc_obj_notify(Object*);
int ivoc_list_count(Object*);
Object* ivoc_list_item(Object*, int);
double hoc_func_table(void* functable, int n, double* args);
void hoc_spec_table(void** pfunctable, int n);
void* hoc_sec_internal_name2ptr(const char* s, int eflag);
void* hoc_pysec_name2ptr(const char* s, int eflag);
extern void* nrn_parsing_pysec_;

void vector_append(IvocVect*, double);
void vector_delete(IvocVect*);

Object** vector_temp_objvar(IvocVect*);

int is_vector_arg(int);

char* vector_get_label(IvocVect*);
void vector_set_label(IvocVect*, char*);

void hoc_regexp_compile(const char*);
int hoc_regexp_search(const char*);
Symbol* hoc_install_var(const char*, double*);
void hoc_class_registration();
void hoc_spinit();
void hoc_freearay(Symbol*);
int hoc_arayinfo_install(Symbol*, int);
void hoc_free_arrayinfo(Arrayinfo*);
void hoc_free_val_array(double*, std::size_t);
std::size_t hoc_total_array(Symbol*);
void frame_debug();
void hoc_oop_initaftererror();
void hoc_init();
void initplot();
void hoc_audit_command(const char*);
void hoc_audit_from_hoc_main1(int, const char**, const char**);
void hoc_audit_from_final_exit();
void hoc_audit_from_xopen1(const char*, const char*);
void hoc_xopen_from_audit(const char* fname);
int hoc_retrieving_audit();
int hoc_retrieve_audit(int id);
int hoc_saveaudit();

void hoc_close_plot();
void ivoc_cleanup();
void ivoc_final_exit();
[[nodiscard]] int hoc_oc(const char*);
[[nodiscard]] int hoc_oc(const char*, std::ostream& os);
void hoc_initcode();
int hoc_ParseExec(int);
int hoc_get_line();
int hoc_araypt(Symbol*, int);
double hoc_opasgn(int op, double dest, double src);
void hoc_install_object_data_index(Symbol*);
void hoc_template_notify(Object*, int);
void hoc_construct_point(Object*, int);
void hoc_call_ob_proc(Object* ob, Symbol* sym, int narg);
void hoc_push_frame(Symbol*, int);
void hoc_pop_frame();
int hoc_argindex();
void hoc_pop_defer();
void hoc_tobj_unref(Object**);
int hoc_stacktype();
int hoc_inside_stacktype(int);
void hoc_link_symbol(Symbol*, Symlist*);
void hoc_unlink_symbol(Symbol*, Symlist*);
void notify_freed(void*);
void notify_pointer_freed(void*);
int ivoc_list_look(Object*, Object*, char*, int);
void ivoc_free_alias(Object*);
Symbol* ivoc_alias_lookup(const char*, Object*);
void hoc_obj_disconnect(Object*);
void hoc_free_object(Object*);
void hoc_free_pstring(char**);
extern int hoc_returning;
void hoc_on_init_register(void (*)());
int hoc_pid();
int hoc_ired(const char*, int, int, int);
double hoc_xred(const char*, double, double, double);
int hoc_sred(const char*, char*, char*);
int nrnpy_pr(const char* fmt, ...);
int Fprintf(std::FILE*, const char* fmt, ...);
void nrnpy_pass();
void hoc_free_allobjects(cTemplate*, Symlist*, Objectdata*);
int nrn_is_cable();
void* nrn_opaque_obj2pyobj(Object*);  // PyObject reference not incremented
Symbol* hoc_get_symbol(const char* var);

void bbs_done(void);
int hoc_main1(int, const char**, const char**);
char* cxx_char_alloc(std::size_t size);

extern int stoprun;
extern int nrn_mpiabort_on_error_;

/** @} */  // end of hoc_functions
