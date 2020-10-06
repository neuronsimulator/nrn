
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
extern void section_owner();
extern void make_mechanism(), make_pointprocess();
extern void nrnpython();
extern void fsyn(), fsyng(), fsyni();
extern void fclamp(), fclampi(), fclampv(), prstim();
extern void fcurrent(), fmatrix(), frecord_init();
extern void issection(), ismembrane(), sectionname(), psection();
extern void pop_section(), push_section(), section_exists();
extern void delete_section();
extern int secondorder, diam_changed, nrn_shape_changed_, nrn_netrec_state_adjust, nrn_sparse_partrans;
extern double clamp_resist;
extern double celsius;
extern int stoprun;
extern void fit_praxis(), attr_praxis(), pval_praxis(), stop_praxis();
extern void keep_nseg_parm();
extern void nrnallsectionmenu(), nrnallpointmenu(), nrnsecmenu();
extern void nrnglobalmechmenu(), nrnmechmenu(), nrnpointmenu();
extern void this_section(), this_node(), parent_section(), parent_node();
extern void parent_connection(), section_orientation();

/* Functions */
static VoidFunc functions[] = {

"node_data", node_data,
"disconnect", disconnect,
"batch_run", batch_run,
"batch_save", batch_save,
"pt3dclear", pt3dclear,
"pt3dadd", pt3dadd,
"n3d", n3d,
"x3d", x3d,
"y3d", y3d,
"z3d", z3d,
"arc3d", arc3d,
"diam3d", diam3d,
"pt3dinsert", pt3dinsert,
"pt3dremove", pt3dremove,
"pt3dchange", pt3dchange,
"define_shape", define_shape,
"pt3dconst", pt3dconst,
"pt3dstyle", pt3dstyle,
"spine3d", spine3d,
"setSpineArea", setSpineArea,
"getSpineArea", getSpineArea,
"area", area,
"ri", ri,
"initnrn", initnrn,
"topology", nrnhoc_topology,
"fadvance", fadvance,
"distance", distance,
"finitialize", finitialize,
"fstim", fstim,
"fstimi", fstimi,
"ion_style", ion_style,
"ion_register", ion_register,
"ion_charge", ion_charge,
"nernst", nernst,
"ghk", ghk,
"section_owner", section_owner,
"make_mechanism", make_mechanism,
"make_pointprocess", make_pointprocess,
"nrnpython", nrnpython,
"fsyn", fsyn,
"fsyng", fsyng,
"fsyni", fsyni,
"fclamp", fclamp,
"fclampi", fclampi,
"fclampv", fclampv,
"prstim", prstim,
"fcurrent", fcurrent,
"fmatrix", fmatrix,
"frecord_init", frecord_init,
"issection", issection,
"ismembrane", ismembrane,
"sectionname", sectionname,
"psection", psection,
"pop_section", pop_section,
"push_section", push_section,
"section_exists", section_exists,
"delete_section", delete_section,
"fit_praxis", fit_praxis,
"attr_praxis", attr_praxis,
"pval_praxis", pval_praxis,
"stop_praxis", stop_praxis,
"keep_nseg_parm", keep_nseg_parm,
"nrnallsectionmenu", nrnallsectionmenu,
"nrnallpointmenu", nrnallpointmenu,
"nrnsecmenu", nrnsecmenu,
"nrnglobalmechmenu", nrnglobalmechmenu,
"nrnmechmenu", nrnmechmenu,
"nrnpointmenu", nrnpointmenu,
"this_section", this_section,
"this_node", this_node,
"parent_section", parent_section,
"parent_node", parent_node,
"parent_connection", parent_connection,
"section_orientation", section_orientation,
0, 0
};

static struct {  /* Integer Scalars */
  const char *name;
  int  *pint;
} scint[] = {

"secondorder", &secondorder,
"diam_changed", &diam_changed,
"nrn_shape_changed_", &nrn_shape_changed_,
"nrn_netrec_state_adjust", &nrn_netrec_state_adjust,
"nrn_sparse_partrans", &nrn_sparse_partrans,
"stoprun", &stoprun,
0, 0
};

static struct {  /* Vector integers */
  const char *name;
  int  *pint;
  int  index1;
} vint[] = {

0,0
};

static struct {  /* Float Scalars */
  const char *name;
  float  *pfloat;
} scfloat[] = {

0, 0
};

static struct {  /* Vector float */
  const char *name;
  float *pfloat;
  int index1;
} vfloat[] = {

0,0,0
};

/* Double Scalars */
DoubScal scdoub[] = {

"clamp_resist", &clamp_resist,
"celsius", &celsius,
0,0
};

/* Vectors */
DoubVec vdoub[] = {

0, 0, 0
};

static struct {  /* Arrays */
  const char *name;
  double *pdoub;
  int index1;
  int index2;
} ardoub[] = {

0, 0, 0, 0
};

static struct {  /* triple dimensioned arrays */
  const char *name;
  double *pdoub;
  int index1;
  int index2;
  int index3;
} thredim[] = {

0, 0, 0, 0, 0
};

