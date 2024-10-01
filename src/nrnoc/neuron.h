#include "options.h"

extern void disconnect();
extern void batch_run(), batch_save();
extern void pt3dadd(), n3d(), x3d(), y3d(), z3d(), arc3d(), diam3d();
extern void pt3dclear(), pt3dinsert(), pt3dremove(), pt3dchange();
extern void define_shape(), pt3dconst(), pt3dstyle();
extern void spine3d(), setSpineArea(), getSpineArea();
extern void area(), ri(), distance();
extern void initnrn(), nrnhoc_topology(), fadvance(), finitialize();
extern void fstim(), fstimi();
extern void ion_style(), ion_register(), ion_charge(), nernst(), ghk();
extern void section_owner(); /* returns object that created section */
extern void make_mechanism(), make_pointprocess();
extern void nrnpython();
extern void nrnunit_use_legacy();

extern void fsyn(), fsyng(), fsyni();
extern void fclamp(), fclampi(), fclampv(), prstim();
extern void fcurrent(), fmatrix(), frecord_init();
extern void issection(), ismembrane(), sectionname(), psection();
extern void pop_section(), push_section(), section_exists();
extern void delete_section();
extern int secondorder, diam_changed, nrn_shape_changed_;
extern int nrn_netrec_state_adjust, nrn_sparse_partrans;
extern double clamp_resist;
extern double celsius;
extern int stoprun;
extern void fit_praxis(), attr_praxis(), pval_praxis(), stop_praxis();
#if KEEP_NSEG_PARM
extern void keep_nseg_parm();
#endif
#if EXTRACELLULAR
extern void nlayer_extracellular();
#endif
extern void nrnallsectionmenu(), nrnallpointmenu(), nrnsecmenu();
extern void nrnglobalmechmenu(), nrnmechmenu(), nrnpointmenu();
extern void this_section(), this_node(), parent_section(), parent_node();
extern void parent_connection(), section_orientation();
extern void print_local_memory_usage();
