#include "options.h"

#if METHOD3
extern int spatial_method();
#endif

#if NEMO
extern int neuron2nemo(), nemo2neuron();
#endif
extern void node_data(), disconnect();
extern void batch_run(), batch_save();
extern void pt3dclear(), pt3dadd(), n3d(), x3d(), y3d(), z3d(), arc3d(), diam3d();
extern void pt3dinsert(), pt3dremove(), pt3dchange();
extern void define_shape(), pt3dconst(), pt3dstyle();
extern void spine3d(), setSpineArea(), getSpineArea();
extern void area(), ri();
extern void initnrn(), nrnhoc_topology(), fadvance(), distance(), finitialize();
extern void fstim(), fstimi();
extern void ion_style(), ion_register(), ion_charge(), nernst(), ghk();
extern void section_owner(); /* returns object that created section */
extern void make_mechanism(), make_pointprocess();
extern void nrnpython();
#if !SEJNOWSKI
extern void fsyn(), fsyng(), fsyni();
#endif
extern void fclamp(), fclampi(), fclampv(), prstim();
extern void fcurrent(), fmatrix(), frecord_init();
extern void issection(), ismembrane(), sectionname(), psection();
extern void pop_section(), push_section(), section_exists();
extern void delete_section();
extern int secondorder, diam_changed, nrn_shape_changed_, nrn_netrec_state_adjust;
extern double clamp_resist;
extern double celsius;
extern int stoprun;

extern void fit_praxis(), attr_praxis(), pval_praxis(), stop_praxis();
#if KEEP_NSEG_PARM
extern void keep_nseg_parm();
#endif

extern void nrnallsectionmenu(), nrnallpointmenu(), nrnsecmenu();
extern void nrnglobalmechmenu(), nrnmechmenu(), nrnpointmenu();

extern void this_section(), this_node(), parent_section(), parent_node();
extern void parent_connection(), section_orientation();

#if SEJNOWSKI
extern void fdefault();			 		      /* for sej_menu.c     */
extern void dump_vars();                                       /* sej_default.c      */
extern void update_id_info(), params(), private_menu(); 	      /* sej_menu.c    	*/
extern void save_run(), save_params(), flush(), file_exist();  /* sej_menu.c    	*/
extern void sassign(), dassign(), setup_id_info(), clean_dir();/* sej_menu.c    	*/
extern void ftime(), fseed(), fran(), rand(), norm(), pois();  /* sej_ransyn.c  	*/
extern void syn_reset(), fsyn(), fsyn_set(), fsyng(), fsyni(); /* sej_synapse.c 	*/
extern void con_reset(), fcon(), fcon_set(), fcong(), fconi(); /* sej_connect.c 	*/
extern void top2(), dump(), dump_all();                        /* sej_dump.c         */

extern void ptest(); 

/* non-initialized variables */
double ic;
double id_number, param_number; 			      /* for sej_menu.c     */

/* initialized variables */
double stop_time, nsteps, vrest;
double init_seed, run_seed, cainit, kinit, nainit;
double dump_flag, print_flag, graph_flag, gray_flag;
double fig_flag, timer_flag, stim_flag, view_flag;

#endif

#if FISHER
extern void rcsdiff_file(), rcs_version(), rcs_co_file();   	/* rcs.c */
extern void rcs_ci_file(), rcs_view_file();               	/* rcs.c */
extern void add_version_entry(), save_output_file();          	/* sys.c */
extern void answer_yes(), file_exist(), dassign(), sassign();  	/* sys.c */
extern void setup_id_info(), update_id_info();                	/* sys.c */

/* non-initialized variables */
#if !SEJNOWSKI
double id_number;                                           	/* sys.c */
#endif
#endif
