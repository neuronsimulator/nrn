#ifndef oc_ansi_h
#define oc_ansi_h


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


#if defined(__cplusplus)
class IvocVect;
#else
#define IvocVect void
#endif

#if defined(__cplusplus)
extern "C" {
#endif

// nocpout.cpp
extern void hoc_register_var(DoubScal*, DoubVec*, VoidFunc*);
extern void ivoc_help(const char*);

extern Symbol* hoc_lookup(const char*);

extern void* hoc_Ecalloc(size_t nmemb, size_t size);
extern void* hoc_Emalloc(size_t size);
extern void hoc_malchk(void);

extern void hoc_execerror(const char*, const char*);
extern void hoc_execerr_ext(const char* fmt, ...);
extern char* hoc_object_name(Object*);
extern void hoc_retpushx(double);

extern double* getarg(int);
extern int ifarg(int);

extern int vector_instance_px(void*, double**);
extern void install_vector_method(const char*, double (*)(void*));

extern IvocVect* vector_arg(int);
extern int vector_arg_px(int i, double** p);
extern double* vector_vec(IvocVect*);
extern int vector_capacity(IvocVect*);
extern void vector_resize(IvocVect*, int);

#if defined(__cplusplus)
}
#endif

extern int nrnignore;

#if defined(__cplusplus)
extern "C" {
#endif


/**
 * \brief Brief explanation of hoc_obj_run
 *
 * Detailed explanation of hoc_obj_run goes here.
 */
extern int hoc_obj_run(const char*, Object*);

extern int hoc_argtype(int);
extern int hoc_is_double_arg(int);
extern int hoc_is_pdouble_arg(int);
extern int hoc_is_str_arg(int);
extern int hoc_is_object_arg(int);
extern char* gargstr(int);
extern char** hoc_pgargstr(int);

extern double* hoc_pgetarg(int);
extern Object** hoc_objgetarg(int);
extern Object* hoc_name2obj(const char* name, int index);

extern char** hoc_temp_charptr(void);
extern int hoc_is_temp_charptr(char** cpp);
extern void hoc_assign_str(char** pstr, const char* buf);
extern double chkarg(int, double low, double high);
extern double hoc_call_func(Symbol*,
                            int narg); /* push first arg first. Warning: if the function is inside
                                          an object make sure you know what you are doing.*/
extern double hoc_call_objfunc(Symbol*, int narg, Object*); /* call a fuction within the context of
                                                               an object.*/
extern double hoc_ac_;
extern double hoc_epsilon;
extern int nrn_inpython_;

extern int hoc_color;
extern int hoc_set_color(int);
extern void hoc_plt(int, double, double);
extern void hoc_plprint(const char*);
extern void hoc_ret(void); /* but need to push before returning */

extern void hoc_pushx(double);
extern void hoc_pushstr(char**);
extern void hoc_pushobj(Object**);
extern void hoc_push_object(Object*);
extern void hoc_pushpx(double*);
extern void hoc_pushs(Symbol*);
extern void hoc_pushi(int);
extern double hoc_xpop(void);
extern Symbol* hoc_spop(void);
extern double* hoc_pxpop(void);
extern Object** hoc_objpop(void);
extern Object* hoc_pop_object(void);
extern char** hoc_strpop(void);
extern int hoc_ipop(void);
extern void hoc_nopop(void);

extern void hoc_execerror_mes(const char*, const char*, int);
extern void hoc_warning(const char*, const char*);
extern double* hoc_val_pointer(const char*);
extern Symbol* hoc_table_lookup(const char*, Symlist*);
extern Symbol* hoc_install(const char*, int, double, Symlist**);
extern Objectdata* hoc_objectdata;
extern Datum* hoc_look_inside_stack(int, int);
extern Object* hoc_obj_look_inside_stack(int);
extern int hoc_obj_look_inside_stack_index(int);
extern void hoc_stkobj_unref(Object*, int stkindex);
extern size_t hoc_total_array_data(Symbol*, Objectdata*);
extern char* hoc_araystr(Symbol*, int, Objectdata*);

extern char* hoc_object_pathname(Object*);
extern const char* expand_env_var(const char*);
extern void check_obj_type(Object*, const char*);
extern int is_obj_type(Object*, const char*);
extern void hoc_obj_ref(Object*);   /* NULL allowed */
extern void hoc_obj_unref(Object*); /* NULL allowed */
extern void hoc_dec_refcount(Object**);
extern Object** hoc_temp_objvar(Symbol* template_symbol, void* cpp_object);
extern Object** hoc_temp_objptr(Object*);
extern void hoc_new_object_asgn(Object** obp, Symbol* template_symbol, void* cpp_object);
extern HocSymExtension* hoc_var_extra(const char*);
extern double check_domain_limits(float*, double);
extern Object* hoc_obj_get(int i);
extern void hoc_obj_set(int i, Object*);
extern void nrn_hoc_lock(void);
extern void nrn_hoc_unlock(void);

extern void* hoc_Erealloc(void* ptr, size_t size);

extern void* nrn_cacheline_alloc(void** memptr, size_t size);
extern void* nrn_cacheline_calloc(void** memptr, size_t nmemb, size_t size);
extern void nrn_exit(int);
extern void hoc_free_list(Symlist**);
extern int hoc_errno_check(void);
extern Symbol* hoc_parse_stmt(const char*, Symlist**);
extern void hoc_run_stmt(Symbol*);
extern Symbol* hoc_parse_expr(const char*, Symlist**);
extern double hoc_run_expr(Symbol*);
extern void hoc_free_string(char*);
extern int hoc_xopen1(const char*, const char*);
extern int hoc_xopen_run(Symbol*, const char*);
extern void hoc_symbol_limits(Symbol*, float, float);
extern void sym_extra_alloc(Symbol*);
extern int hoc_chdir(const char* path);

extern void hoc_final_exit();
extern void hoc_sprint1(char**, int);
extern double hoc_scan(FILE*);
extern char* hoc_symbol_units(Symbol* sym, const char* units);
extern void hoc_fake_call(Symbol*);
extern void hoc_last_init(void);
extern void hoc_obj_notify(Object*);
extern int ivoc_list_count(Object*);
extern double hoc_func_table(void* functable, int n, double* args);
extern void hoc_spec_table(void** pfunctable, int n);
extern void* hoc_sec_internal_name2ptr(const char* s, int eflag);
extern void* hoc_pysec_name2ptr(const char* s, int eflag);
extern void* nrn_parsing_pysec_;

extern void vector_append(IvocVect*, double);
extern IvocVect* vector_new(int, Object*); /*use this if possible*/
extern IvocVect* vector_new0();
extern IvocVect* vector_new1(int);
extern IvocVect* vector_new2(IvocVect*);
extern void vector_delete(IvocVect*);
extern int vector_buffer_size(IvocVect*);

extern Object** vector_temp_objvar(IvocVect*);
extern Object** vector_pobj(IvocVect*);

extern int is_vector_arg(int);

extern char* vector_get_label(IvocVect*);
extern void vector_set_label(IvocVect*, char*);

extern void hoc_regexp_compile(const char*);
extern int hoc_regexp_search(const char*);
extern Symbol* hoc_install_var(const char*, double*);
extern void hoc_class_registration(void);
extern void hoc_spinit(void);
extern void hoc_freearay(Symbol*);
extern int hoc_arayinfo_install(Symbol*, int);
extern void hoc_free_arrayinfo(Arrayinfo*);
extern void hoc_free_val_array(double*, size_t);
extern size_t hoc_total_array(Symbol*);
extern void hoc_menu_cleanup(void);
extern void frame_debug(void);
extern void hoc_oop_initaftererror(void);
extern void save_parallel_envp(void);
extern void save_parallel_argv(int, const char**);
extern void hoc_init(void);
extern void initplot(void);
extern void hoc_audit_command(const char*);
extern void hoc_audit_from_hoc_main1(int, const char**, const char**);
extern void hoc_audit_from_final_exit(void);
extern void hoc_audit_from_xopen1(const char*, const char*);
extern void hoc_xopen_from_audit(const char* fname);
extern int hoc_retrieving_audit(void);
extern int hoc_retrieve_audit(int id);
extern int hoc_saveaudit(void);

extern void hoc_close_plot(void);
extern void ivoc_cleanup(void);
extern void ivoc_final_exit(void);
extern int hoc_oc(const char*);
extern void hoc_initcode(void);
extern int hoc_ParseExec(int);
extern int hoc_get_line(void);
extern int hoc_araypt(Symbol*, int);
extern double hoc_opasgn(int op, double dest, double src);
extern void hoc_install_object_data_index(Symbol*);
extern void hoc_template_notify(Object*, int);
extern void hoc_construct_point(Object*, int);
extern void hoc_call_ob_proc(Object* ob, Symbol* sym, int narg);
extern void hoc_push_frame(Symbol*, int);
extern void hoc_pop_frame(void);
extern int hoc_argindex(void);
extern void hoc_pop_defer(void);
extern void hoc_tobj_unref(Object**);
extern int hoc_stacktype(void);
extern int hoc_inside_stacktype(int);
extern void hoc_link_symbol(Symbol*, Symlist*);
extern void hoc_unlink_symbol(Symbol*, Symlist*);
extern void notify_freed(void*);
extern void notify_freed_val_array(double*, size_t);
extern void notify_pointer_freed(void*);
extern int ivoc_list_look(Object*, Object*, char*, int);
extern void ivoc_free_alias(Object*);
extern Symbol* ivoc_alias_lookup(const char*, Object*);
extern void hoc_obj_disconnect(Object*);
extern void hoc_free_object(Object*);
extern void hoc_free_pstring(char**);
extern int hoc_returning;
extern void hoc_on_init_register(Pfrv);
extern int hoc_pid(void);
extern int hoc_ired(const char*, int, int, int);
extern double hoc_xred(const char*, double, double, double);
extern int hoc_sred(const char*, char*, char*);
extern int nrnpy_pr(const char* fmt, ...);
extern int Fprintf(FILE*, const char* fmt, ...);
extern void nrnpy_pass();
extern void hoc_free_allobjects(cTemplate*, Symlist*, Objectdata*);
extern int nrn_is_cable(void);
extern int nrn_isdouble(double*, double, double);
extern void* nrn_opaque_obj2pyobj(Object*);  // PyObject reference not incremented

extern double hoc_Exp(double);
extern int hoc_is_tempobj_arg(int narg);
extern FILE* hoc_obj_file_arg(int i);
extern void hoc_reg_nmodl_text(int type, const char* txt);
extern void hoc_reg_nmodl_filename(int type, const char* filename);
extern size_t nrn_mallinfo(int item);
extern int nrn_mlh_gsort(double* vec, int* base_ptr, int total_elems, int (*cmp)(double, double));
extern void state_discontinuity(int i, double* pd, double d);

extern double nrn_event_queue_stats(double* stats);
extern double* nrn_recalc_ptr(double*);
extern void nrn_register_recalc_ptr_callback(Pfrv f);

// BlueBrain
extern void* bbss_buffer_counts(int*, int**, int**, int*);
extern void bbss_save_global(void*, char*, int);
extern void bbss_restore_global(void*, char*, int);
extern void bbss_save(void*, int, char*, int);
extern void bbss_restore(void*, int, int, char*, int);
extern void bbss_save_done(void*);
extern void bbss_restore_done(void*);

#if defined(__cplusplus)
}
#endif

extern int _nrnunit_use_legacy_; /* 1:legacy, 0:modern (default) */
extern void bbs_done(void);
extern int hoc_main1(int, const char**, const char**);
extern char* cxx_char_alloc(size_t size);
extern int stoprun;
extern int nrn_mpiabort_on_error_;

#endif

/** @} */  // end of hoc_functions
