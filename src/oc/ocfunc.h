#pragma once
#include "nrnfilewrap.h"

#include <cstddef>

extern double hoc_Log(double), hoc_Log10(double), hoc1_Exp(double), hoc_Sqrt(double),
    hoc_integer(double);
extern double hoc_Pow(double, double);
extern void hoc_System(void), hoc_Prmat(void), hoc_solve(void), hoc_eqinit(void), hoc_Plt(void),
    hoc_atan2(void);
extern void hoc_symbols(void), hoc_PRintf(void), hoc_Xred(void), hoc_Sred(void);
extern void hoc_ropen(void), hoc_wopen(void), hoc_xopen(void), hoc_Fscan(void), hoc_Fprint(void);
extern void hoc_Graph(void), hoc_Graphmode(void), hoc_Plot(void), hoc_axis(void), hoc_Sprint(void);
extern void hoc_Getstr(void), hoc_Strcmp(void);
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
extern void hoc_nrn_load_dll(void), hoc_nrnversion(void), hoc_object_pushed(void);
extern void hoc_mallinfo(void);
extern void hoc_Setcolor(void);
extern void hoc_init_space(void);
extern void hoc_install_hoc_obj(void);
extern void nrn_feenableexcept(void);
void hoc_coreneuron_handle();
void hoc_get_config_key();
void hoc_get_config_val();
void hoc_num_config_keys();
extern int nrn_feenableexcept_;
#if defined(WIN32)
extern void hoc_win_exec();
#endif

namespace nrn::oc {
// Avoid `Frame` because InterViews likes #define-ing that as something else
struct frame;
}  // namespace nrn::oc
union Inst;
struct Object;
union Objectdata;
struct Symlist;
void oc_restore_code(Inst** a1,
                     Inst** a2,
                     std::size_t& a3,
                     nrn::oc::frame** a4,
                     int* a5,
                     int* a6,
                     Inst** a7,
                     nrn::oc::frame** a8,
                     std::size_t& a9,
                     Symlist** a10,
                     Inst** a11,
                     int* a12);
void oc_restore_hoc_oop(Object** a1, Objectdata** a2, int* a4, Symlist** a5);
void oc_restore_input_info(const char* i1, int i2, int i3, NrnFILEWrap* i4);
void oc_save_code(Inst** a1,
                  Inst** a2,
                  std::size_t& a3,
                  nrn::oc::frame** a4,
                  int* a5,
                  int* a6,
                  Inst** a7,
                  nrn::oc::frame** a8,
                  std::size_t& a9,
                  Symlist** a10,
                  Inst** a11,
                  int* a12);
void oc_save_hoc_oop(Object** a1, Objectdata** a2, int* a4, Symlist** a5);
void oc_save_input_info(const char**, int*, int*, NrnFILEWrap**);
