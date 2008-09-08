#include "options.h"

#if METHOD3
extern int spatial_method();
#endif

#if NEMO
extern int neuron2nemo(), nemo2neuron();
#endif
extern int node_data(), disconnect();
extern int batch_run(), batch_save();
extern int pt3dclear(), pt3dadd(), n3d(), x3d(), y3d(), z3d(), arc3d(), diam3d();
extern int pt3dinsert(), pt3dremove(), pt3dchange();
extern int define_shape(), pt3dconst(), pt3dstyle();
extern int spine3d(), setSpineArea(), getSpineArea();
extern int area(), ri();
extern int initnrn(), topology(), fadvance(), distance(), finitialize();
extern int fstim(), fstimi();
extern int ion_style(), ion_register(), ion_charge(), nernst(), ghk();
extern int section_owner(); /* returns object that created section */
extern int make_mechanism(), make_pointprocess();
extern int nrnpython();
#if !SEJNOWSKI
extern int fsyn(), fsyng(), fsyni();
#endif
extern int fclamp(), fclampi(), fclampv(), prstim();
extern int fcurrent(), fmatrix(), frecord_init();
extern int issection(), ismembrane(), sectionname(), psection();
extern int pop_section(), push_section(), section_exists();
extern int delete_section();
extern int secondorder, diam_changed, nrn_shape_changed_;
extern double clamp_resist;
extern double celsius;
extern int stoprun;

extern int fit_praxis(), attr_praxis(), pval_praxis(), stop_praxis();
#if KEEP_NSEG_PARM
extern int keep_nseg_parm();
#endif

extern int nrnallsectionmenu(), nrnallpointmenu(), nrnsecmenu();
extern int nrnglobalmechmenu(), nrnmechmenu(), nrnpointmenu();

extern int this_section(), this_node(), parent_section(), parent_node();
extern int parent_connection(), section_orientation();

#if SEJNOWSKI
extern int fdefault();			 		      /* for sej_menu.c     */
extern int dump_vars();                                       /* sej_default.c      */
extern int update_id_info(), params(), private_menu(); 	      /* sej_menu.c    	*/
extern int save_run(), save_params(), flush(), file_exist();  /* sej_menu.c    	*/
extern int sassign(), dassign(), setup_id_info(), clean_dir();/* sej_menu.c    	*/
extern int ftime(), fseed(), fran(), rand(), norm(), pois();  /* sej_ransyn.c  	*/
extern int syn_reset(), fsyn(), fsyn_set(), fsyng(), fsyni(); /* sej_synapse.c 	*/
extern int con_reset(), fcon(), fcon_set(), fcong(), fconi(); /* sej_connect.c 	*/
extern int top2(), dump(), dump_all();                        /* sej_dump.c         */

extern int ptest(); 

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
extern int rcsdiff_file(), rcs_version(), rcs_co_file();   	/* rcs.c */
extern int rcs_ci_file(), rcs_view_file();               	/* rcs.c */
extern int add_version_entry(), save_output_file();          	/* sys.c */
extern int answer_yes(), file_exist(), dassign(), sassign();  	/* sys.c */
extern int setup_id_info(), update_id_info();                	/* sys.c */

/* non-initialized variables */
#if !SEJNOWSKI
double id_number;                                           	/* sys.c */
#endif
#endif
