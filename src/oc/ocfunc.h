#ifndef ocfunc_h
#define ocfunc_h


extern double hoc_Log(double), hoc_Log10(double), hoc1_Exp(double), hoc_Sqrt(double),
    hoc_integer(double);
extern double hoc_Pow(double, double);
extern void hoc_System(void), hoc_Prmat(void), hoc_solve(void), hoc_eqinit(void), hoc_Plt(void),
    hoc_atan2(void);
extern void hoc_symbols(void), hoc_PRintf(void), hoc_Xred(void), hoc_Sred(void);
extern void hoc_ropen(void), hoc_wopen(void), hoc_xopen(void), hoc_Fscan(void), hoc_Fprint(void);
extern void hoc_Graph(void), hoc_Graphmode(void), hoc_Plot(void), hoc_axis(void), hoc_Sprint(void);
extern void hoc_fmenu(void), hoc_Getstr(void), hoc_Strcmp(void);
extern void hoc_Lw(void), hoc_machine_name(void), hoc_Saveaudit(void), hoc_Retrieveaudit(void);
extern void hoc_plotx(void), hoc_ploty(void), hoc_regraph(void);
extern void hoc_startsw(void), hoc_stopsw(void), hoc_object_id(void);
extern void hoc_allobjects(void), hoc_allobjectvars(void);
extern void hoc_xpanel(void), hoc_xbutton(void), hoc_xmenu(void), hoc_xslider(void);
extern void hoc_xfixedvalue(void), hoc_xvarlabel(void), hoc_xradiobutton(void);
extern void hoc_xvalue(void), hoc_xpvalue(void), hoc_xlabel(void), ivoc_style(void);
extern void hoc_boolean_dialog(void), hoc_string_dialog(void), hoc_continue_dialog(void);
extern void nrn_err_dialog(const char*);
extern void hoc_single_event_run(void), hoc_notify_iv(void), nrniv_bind_thread(void);
extern void hoc_pointer(void), hoc_Numarg(void), hoc_Argtype(void), hoc_exec_cmd(void);
extern void hoc_load_proc(void), hoc_load_func(void), hoc_load_template(void), hoc_load_file(void);
extern void hoc_xcheckbox(void), hoc_xstatebutton(void), hoc_Symbol_limits(void);
extern void hoc_coredump_on_error(void), hoc_checkpoint(void), hoc_quit(void);
extern void hoc_object_push(void), hoc_object_pop(void), hoc_pwman_place(void);
extern void hoc_show_errmess_always(void), hoc_execute1(void), hoc_secname(void);
extern void hoc_neuronhome(void), hoc_Execerror(void);
extern void hoc_sscanf(void), hoc_save_session(void), hoc_print_session(void);
extern void hoc_Chdir(void), hoc_getcwd(void), hoc_Symbol_units(void), hoc_stdout(void);
extern void hoc_name_declared(void), hoc_unix_mac_pc(void), hoc_show_winio(void);
extern void hoc_usemcran4(void), hoc_mcran4(void), hoc_mcran4init(void);
extern void hoc_nrn_load_dll(void), hoc_nrnversion(void), hoc_object_pushed(void);
extern void hoc_mallinfo(void), hoc_load_java(void);
extern void hoc_Setcolor(void);
extern void hoc_init_space(void);
extern void hoc_install_hoc_obj(void);
extern void nrn_feenableexcept(void);
extern int nrn_feenableexcept_;
#if DOS
extern void hoc_settext(void);
#endif
#if defined(WIN32)
extern void hoc_win_exec();
#endif


#endif
