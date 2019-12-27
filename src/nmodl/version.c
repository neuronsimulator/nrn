#include <../../nmodlconf.h>
/* $Header$ */
/*
$Log$
Revision 1.2  2005/10/04 20:55:32  hines
The bg1 branch is merged into the Main trunk. That branch contains all
the BlueGene specific enhancements and many general Parallel Network simulation
enhancements that were useful for efficient simulation on the BlueGene.
The bottom line is that a 25000 cell Cortical network eleptogenesis model
by Paul Bush, re-written for parallel simulation by Bill Lytton shows linear
speedup on up to 2500 cpus on the BlueGene. This model generated approximately
200000 spikes and managed over 100,000,000 NetCons.

Revision 1.1.1.1.72.1  2005/09/25 16:59:51  hines
A first pass at configure support for cross compiling.  Not perfect but
much superior to what existed prior to this commit.  The problem is that
although most of the binaries constructed are executed on the host
machine, there are three, hoc_e, modlunit, and nocmodl, that are
executed on the build machine.  This means potentially different
configuration, compilers, and makefile dependency mechanisms.  The
current solution requires two passes.  The first pass is to configure
with the --with-nmodl-only option and then make and make install.  That
pass constructs a nmodlconf.h file which is the dual to nrnconf.h.  The
second pass should configure with the --without-nmodl option and then
make and make install. Note that it is important to properly set
CC, CXX, CFLAGS, CXXFLAGS, LIBS and any other thing that is distinct between
the configurations. Also note that the BlueGene is special since those
variables are set to proper default values for both passes. In this
case the nmodl pass should have the configuration options
--enable-bluegene --with-nmodl-only
and the second pass does NOT need the --without-nmodl option and the single
option --enable-bluegene suffices.
One can switch to the build configuration without affecting the host and
vice versa since .h.in files are not shared. That is helpful as there are
no files that have to be recompiled due to the switch.

Revision 1.1.1.1  2003/02/11 18:36:08  hines
NEURON Simulation environment. Unreleased 5.4 development version.

Revision 1.1.1.1  2003/01/01 17:46:33  hines
NEURON --  Version 5.4 2002/12/23 from prospero as of 1-1-2003

Revision 1.1.1.1  2002/06/26 16:23:07  hines
NEURON 5.3 2002/06/04

 * Revision 1.1.1.1  2001/01/01  20:30:34  hines
 * preparation for general sparse matrix
 *
 * Revision 1.2  2000/03/27  14:17:44  hines
 * All sources now include nrnconf.h instead of config.h so that minor
 * option changes (eg in src/parallel/bbsconf.h) do not require recompilation
 * of everything.
 *
 * Revision 1.1.1.1  2000/03/09  13:55:34  hines
 * almost working
 *
 * Revision 4.3  1997/11/13  19:53:08  hines
 * LONGITUDINAL_DIFFUSION area {state} allowed in KINETIC block
 * same syntax as COMPARTMENT statement.
 * Units checking not established yet but area should be in um2 and
 * the COMPARTMENT volume units should be um3/um.
 * If the longitudinal area is affected by diam then diam should
 * appear explicitly in the area expression (generally as diam*diam/4
 * but perhaps some coordinate systems may have area that scales
 * linearly with diam).
 * Longitudinal diffusion works correctly with changing diameter and
 * rallbranch. It works with cvode but the linear solver for it
 * is currently equivalent to modified euler. With CVODE and
 * cvode.jacobian(1) to use a full matrix the approach to steady state
 * is fast if there are not too many states. It would be nice to
 * do some approimate tridiagonal system approaches for the built-in
 * linear solver.
 *
 * Revision 4.2  1997/10/20  14:59:33  hines
 * nocmodl file.mod allowed (ie suffix is allowed)
 *
 * Revision 4.1  1997/08/30  20:45:38  hines
 * cvs problem with branches. Latest nmodl stuff should now be a top level
 *
 * Revision 4.0.1.1  1997/08/08  17:24:06  hines
 * nocmodl version 4.0.1
 *
 * Revision 4.0  1997/08/08  17:22:24  hines
 * version 4.0  of nocmodl
 *
 * Revision 1.4  1997/08/08  17:21:52  hines
 * version 4.0 of nocmodl
 *
 * Revision 1.3  1997/08/08  17:06:34  hines
 * proper nocmodl version number
 *
 * Revision 1.2  1997/08/08  17:03:28  hines
 * correct nocmodl language version
 *
 * Revision 1.1.1.1  1994/10/12  17:21:37  hines
 * NEURON 3.0 distribution
 *
 * Revision 9.194  1994/10/03  17:49:49  hines
 * more flexibility as to relation of erev and concentrations and their types
 *
 * Revision 9.193  1994/09/26  18:37:06  hines
 * forgot to checkin last time
 *
 * Revision 9.192  1994/09/26  18:20:19  hines
 * USEION ... VALENCE real
 * required when a model uses an ion other than na, k, ca
 * USEION ... WRITE cai, cao
 * promotes cai, cao to STATE
 * USEION ... READ cai, cao
 * promotes cai, cao to CONST
 *
 * Revision 9.191  1994/09/20  14:45:44  hines
 * port to dec alpha
 *
 * Revision 9.190  1994/09/20  14:44:11  hines
 * port to dec alpha
 *
 * Revision 9.189  1994/08/23  15:45:09  hines
 * In SOLVE procedure the VERBATIM return 0; is no longer
 * requires since all procedures have this by default.
 * In nocmodl, INDEPENDENT no longer required since if doesn't exist then
 * t (ms) automatically declared.
 *
 * Revision 9.188  1994/08/18  12:12:43  hines
 * properly put pragmas in with if... else stmts.
 * Dont use pragmas in nonvectorized part of kinetic block
 *
 * Revision 9.187  1994/08/05  14:11:14  hines
 * NullParameter replaces null args in Imakefile
 *
 * Revision 9.186  1994/08/04  20:03:55  hines
 * forgot to remove 1 && from kinetic vectorize testbed
 *
 * Revision 9.185  1994/07/30  17:14:21  hines
 * standard check for bison and changes to kinetic vectorization surrounded
 * by proper conditional preprocessor directives
 *
 * Revision 9.184  1994/07/26  14:04:22  hines
 * avoid objectcenter warning about retrieving int from pointer when
 * procedure is solved. The pointer in question is null and the int
 * is never used.
 *
 * Revision 9.183  1994/07/25  20:59:38  hines
 * wrong number of args in an emission of MATELM in new style.
 *
 * Revision 9.182  1994/07/21  13:11:16  hines
 * allow vectorization of kinetic blocks on cray when using sparse method.
 * not tested yet
 *
 * Revision 9.181  1994/05/23  17:56:52  hines
 * error in handling MODL_INCLUDE when no file exists
 *
 * Revision 9.180  1994/05/18  18:11:56  hines
 * INCLUDE "file"
 * tries originalpath/file ./file MODL_INCLUDEpaths/file
 *
 * Revision 9.179  1994/05/09  19:10:30  hines
 * pointprocess.has_loc() returns true if point process located in section
 *
 * Revision 9.178  1994/05/02  12:48:35  hines
 * on abort with NOCMODL call nrn_complain(_p) to tell location where
 * failure occurred (only SOLVE statement) and print line and .mod file
 *
 * Revision 9.177  1994/03/16  20:51:30  hines
 * use getcwd instead of getwd. More portable
 *
 * Revision 9.176  1994/03/08  17:51:05  hines
 * allow help syntax, ? treated same as :
 * send the filename and mechname to hoc during registration
 *
 * Revision 9.175  1994/02/24  19:25:58  hines
 * better error message if no declaration and gets caught in decode_ustr
 *
 * Revision 9.174  1993/11/04  15:53:20  hines
 * port to solaris2 (no more warnings)
 *
 * Revision 9.173  1993/07/09  17:21:34  hines
 * fix error in vectorization of solve from last change
 *
 * Revision 9.172  93/07/09  16:28:36  hines
 * point processes now work correctly when they are electrodes and an
 * extracellular mechananism is present.
 * vclmp.mod and stim.mod are electrodes
 * positive electrode current causes depolarization (normal mechanism 
 * current is positive outward)
 *  When ELECTRODE_CURRENT keyword is present then v refers to
 * vmembrane+vext
 * Note : no warning is given if there are membrane currents within
 * the same model. they are also silently treated the same way as
 * an electrode
 * 
 * Revision 9.171  93/07/08  14:46:09  hines
 * Alternative to NONSPECIFIC_CURRENT is ELECTRODE_CURRENT
 * at this time is identical to old way. Introduced because membrane
 * mechanisms for electrode currents don't work right because the
 * current is assumed to pass from the extracellular space to the inside
 * and thus current injection does not change vext (and it is opposite in
 * sign to what one would expect). This change is not complete here. Only
 * the scaffolding is now in place and it has been verified to be identical
 * to the old way. The change is mostly to non-vectorized models and
 * now
 * void nrn_cur(Node*, double*, Datum*) so that the model gets v directly
 * from the node and changes d and rhs from the node.
 * 
 * Revision 9.170  93/05/17  11:06:54  hines
 * PI defined by some <math.h> so undefing it
 * 
 * Revision 9.169  93/04/28  10:01:12  hines
 * local right hand side for sparse. used to be extern modl_rhs now it is
 * coef...  This allows multiple models to use sparse.
 * 
 * Revision 9.168  93/04/23  11:13:45  hines
 * model keeps sparse method state in its own static area so sparse
 * can solve different sets of equations
 * 
 * Revision 9.167  93/04/21  11:06:10  hines
 * lineq blocks do not return error number
 * nocpout point processes dparam element 2 reserved for a pointer to
 * Point_process
 * 
 * Revision 9.166  93/03/21  17:47:57  hines
 * for some reason vectorized models with functions with no args
 * were begin translated at f(ix_,)
 * 
 * Revision 9.165  93/02/25  13:25:51  hines
 * need_memb called with symbol instead of name
 * 
 * Revision 9.164  93/02/23  14:51:59  hines
 * compiles with dos
 * 
 * Revision 9.163  93/02/23  14:48:15  hines
 * compiles with DOS
 * 
 * Revision 9.162  93/02/23  14:43:33  hines
 * works under DOS
 * 
 * Revision 9.161  93/02/15  08:57:33  hines
 * linux
 * 
 * Revision 9.159  93/02/11  16:55:48  hines
 * minor mods for NeXT
 * 
 * Revision 9.158  93/02/02  10:32:50  hines
 * create static func before usage
 * 
 * Revision 9.157  93/02/01  15:18:19  hines
 * static functions should be declared before use.
 * inline is keyword for some compilers.
 * 
 * Revision 9.156  93/01/23  13:13:57  hines
 * The NeXT does not like to include files twice so nmodl_redef.h
 * has been separated into nmodl1_redef.h and nmodl2_redef.h
 * 
 * Revision 9.155  93/01/05  11:05:46  hines
 * functions with no suffix still need to be declared
 * 
 * Revision 9.154  92/11/10  11:56:22  hines
 * when compiling a translated model on the CRAY (with _CRAY defined as 1)
 * the check table functions are taken out of the loops. This is
 * not entirely safe if tables are used (and dependencies have changed)
 * in direct calls to functions that call functions with tables. It is
 * reasonably safe for finitialize(), fadvance(), fcurrent(), and
 * calling the hoc_function itself.
 * 
 * Revision 9.153  92/10/30  16:06:36  hines
 * echo proper number of shift/reduce conflicts
 * 
 * Revision 9.152  92/10/27  09:28:20  hines
 * all point processes use same steer_point_process in nrnoc/point.c
 * so there is no need to register a special one.
 * 
 * Revision 9.151  92/10/24  16:31:43  hines
 * sets prop->param_size for use in notifying interviews when arrays are freed
 * 
 * Revision 9.150  92/10/11  15:40:49  hines
 * forgot to declare _n_func for tables
 * 
 * Revision 9.149  92/10/09  08:15:13  hines
 * for nrnoc, change in point process output style. now creates a built
 * in object for point processes. lives in SUN4nocmod
 * 
 * Revision 9.148  92/10/05  13:05:03  hines
 * table functions separated into name(), _check_name(), _f_name(),
 * and _n_name().
 * name() is robust in that it will steer to analytic or table and will
 * check that the table is uptodate. note _n_name is a table that does
 * no checking.
 * 
 * Revision 9.147  92/09/27  17:46:21  hines
 * vectorized channel densities with _method3 fill thisnode.GC,EC instead
 * of rhs,d
 * 
 * Revision 9.146  92/09/25  13:55:18  hines
 * the includes at beginning of lines must be hidden from the preprocessor
 * or else some machines will try to include them.
 * 
 * Revision 9.145  92/08/06  10:13:37  hines
 * the check table function did not have _ix defined
 * 
 * Revision 9.144  92/08/06  09:59:17  hines
 * put cray pragmas at appropriate places
 * get rid of initmodel references to saveing t and incrementing ninit
 * 
 * Revision 9.143  92/08/06  09:04:22  hines
 * table checking and creation moved into separate procedure. still called
 * everytime function is called.
 * 
 * Revision 9.142  92/08/05  16:23:18  hines
 * can vectorize hh. need work on tables though.
 * 
 * Revision 9.140  92/07/27  11:34:15  hines
 * some bugs fixed. no vectorizing except for SOLVE procedure (and that not done yet)
 * 
 * Revision 9.139  92/07/27  10:11:25  hines
 * Can do some limited vectorization. Much remains. Often fails
 * 
 * Revision 9.138  92/06/01  12:04:16  hines
 * NEURON { EXTERNAL name, name, ...}
 * translates to extern double name;
 * GLOBAL changed to translate to
 * #define name name_suffix
 * extern double name
 * 
 * Revision 9.137  92/05/05  08:39:51  hines
 * last addition botched because line put in wrong place
 * 
 * Revision 9.136  92/05/05  08:33:27  hines
 * need to link with math library (at least on RS6000)
 * 
 * Revision 9.135  92/04/15  16:21:35  hines
 * recursive define problem with GLOBFUNCT when suffix was nothing, fixed
 * 
 * Revision 9.134  92/03/31  06:51:21  hines
 * CONSTANT could not be a negative number. Fixed
 * 
 * Revision 9.133  92/03/19  15:15:18  hines
 * creates a nrn_initialize function that will be called from finitialize().
 * 
 * Revision 9.132  92/02/18  13:25:51  hines
 * modl_rhs must be extern or else turboc won't connect it with sparse.c
 * 
 * Revision 9.131  92/02/17  12:53:55  hines
 * GLOBFUNCT for parsact so table functions declare external instead of
 * static.
 * 
 * Revision 9.130  92/02/17  10:25:13  hines
 * global modl functions declared with suffixes using #define
 * 
 * Revision 9.129  92/02/15  21:13:08  hines
 * if bison used compile with -DBISON=1
 * 
 * Revision 9.128  92/02/05  14:47:56  hines
 * saber warning free
 * FUNCTION made global for use by other models. #define GLOBFUNC 1
 * 
 * Revision 9.127  92/02/05  12:32:05  hines
 * creates setdata_suffix(x) to allow use of data when functions called
 * from hoc.
 * 
 * Revision 9.126  92/02/05  08:34:09  hines
 * NEURON {POINTER ...} allows a model to connect to external variables
 * #includes in nparout moved to beginning of defslist so crucial fields in
 * section structures not defined into nonsense.
 * 
 * Revision 9.125  92/01/28  14:08:00  hines
 * fix Imakefile
 * 
 * Revision 9.123  91/12/27  12:29:44  hines
 * no message when COMPARTMENT does not specify STATE since
 * it may specify an assigned or parameter. Note that
 * this statement is irrelevant but it is necessary for modlunit
 * 
 * Revision 9.122  91/10/28  09:10:19  hines
 * wathey's improvements. different binary architectures in different
 * directories.
 * 
 * Revision 9.121  91/10/01  11:30:13  hines
 * SUFFIX nothing now really adds nothing (usd to add _)
 * 
 * Revision 9.120  91/09/16  15:46:15  hines
 * reorganization of how NEURON variables are declared.
 * RANGE: definitely range variables, can be param, assigned, or state
 * GLOBAL: definitely global variables, can be param or assigned
 * SECTION: not implemented
 * By default params are global variables.
 * By default states are range
 * By default  assigned are in p array but not accessible from NEURON
 * NON-SPECIFIC current not accessible unless specified in RANGE.
 * 
 * Revision 9.119  91/09/11  11:51:38  hines
 * extraneous rule in conststmt: removed
 * 
 * Revision 9.118  91/09/11  11:14:53  hines
 * bad function number used in multiple kinetic models.
 * 
 * Revision 9.116  91/08/23  08:30:01  hines
 * MACHDEP was changed to MACHDEF
 * 
 * Revision 9.115  91/08/22  17:07:43  hines
 * for distribution to work on RS6000
 * 
 * Revision 9.114  91/08/15  11:18:31  hines
 * point process calls to point.c in neuron don't use getarg, etc. This
 * means a change to the calling sequence doesn't necessarily mean a change
 * to nparout.c
 * 
 * Revision 9.113  91/08/13  09:17:41  hines
 * Comments inside strings do not need to be escaped. And they cause warnings
 * on some machines.
 * 
 * Revision 9.112  91/08/12  11:35:41  hines
 * creates a get_loc_suffix(i) function to tell what location and
 * section of ith index is.
 * 
 * Revision 9.111  91/08/12  11:34:33  hines
 * more generic makefile. Change MACHDEF for machine dependent definitions.
 * 
 * Revision 9.110  91/08/12  09:57:56  hines
 * set_seed missing in extdef.h
 * 
 * Revision 9.109  91/07/30  10:22:30  hines
 * eion, ioni, iono are constants, iion is assigned
 * 
 * Revision 9.107  91/06/29  08:51:45  hines
 * left out hparout.o in nmodl.mk
 * 
 * Revision 9.106  91/06/29  08:45:32  hines
 * more general units factor and slightly different syntax for conversion
 * 
 * Revision 9.105  91/06/25  09:02:47  hines
 * last change broke saber...
 * 
 * Revision 9.104  91/06/25  08:40:30  hines
 * makefile fairly generic, no sun4 isms
 * 
 * Revision 9.103  91/06/25  08:18:48  hines
 * first attempt at generic makefile, no sun4 isms
 * 
 * Revision 9.102  91/05/22  13:42:51  hines
 * last change botched
 * 
 * Revision 9.101  91/05/22  13:39:24  hines
 * get modlunit stuff from SRC directory instead of RCS directory.
 * 
 * Revision 9.100  91/05/06  13:28:01  hines
 * malloc not properly declared when compiling with turboc
 * 
 * Revision 9.99  91/04/17  16:15:35  hines
 * DISCRETE block integrates by delta_indepvar instead of 1.0.
 * Also delta_indepvar not changed automatically and  model() may return
 * at indepvar at value different from request if break not an integral
 * value of delta_indepvar.
 * 
 * Revision 9.98  91/03/18  09:11:45  hines
 * FORALL i {} loops over all elements of vectors in statements. vector
 * length must be the same.
 * 
 * Revision 9.97  91/02/15  13:35:23  hines
 * allow READ and WRITE of same ionic variable
 * 
 * Revision 9.96  91/02/08  09:49:26  hines
 * "nothing" allowed as suffix (auto functions get base filename)
 * trivial model works since v, t, dt, etc auto created if not declared.
 * 
 * Revision 9.95  91/01/29  09:16:04  hines
 * nmodl doesn't create modl_set_dt because multiple mechanisms would make
 * multiple declarations.
 * 
 * Revision 9.94  91/01/24  14:00:45  hines
 * translation error when last token of LOCAL statement was the first token
 * after the LOCAL statement. Fixed by changing symbols at the parser instead
 * of the lexical analyser.
 * 
 * Revision 9.93  91/01/03  08:00:15  hines
 * some static neuron variables were not declared under some circumstances.
 * 
 * Revision 9.92  91/01/03  07:55:10  hines
 * nparout.c  version comment moved to beginning of file
 * 
 * Revision 9.91  91/01/03  07:18:40  hines
 * stepforce and schedule added to extdef.h
 * 
 * Revision 9.90  91/01/02  08:00:53  hines
 * stepforce and schedule added to list of functions which handle 
 * discontinuities.
 * 
 * Revision 9.89  90/12/14  15:44:24  hines
 * nmodl: point process working with proper current dimensions. tested with
 * stim.mod and pascab1.hoc
 * 
 * Revision 9.88  90/12/14  12:52:35  hines
 * nmodl: point processes allowed via POINT_PROCESS
 * 
 * Revision 9.87  90/12/13  11:25:11  hines
 * calculation of _nstep cast to (int)
 * 
 * Revision 9.86  90/12/12  18:18:14  hines
 * FROM index : index is an integer and cast to double wherever it
 * appears in expressons. It is not cast to int whenever it appears
 * in intexpr
 * 
 * Revision 9.85  90/12/12  17:24:04  hines
 * nmodl: FROM i translates i as double  fh.mod works properly
 * 
 * Revision 9.84  90/12/12  10:06:59  hines
 * nmodl: GLOBAL allowed in NEURON block
 * 
 * Revision 9.83  90/12/12  09:02:14  hines
 * nmodl: nparout allocates the p-array
 * 
 * Revision 9.82  90/12/12  09:01:29  hines
 * LOCAL arrays allowed
 * 
 * Revision 9.81  90/12/12  08:05:47  hines
 * NMODL: better partition of global and p-array. parameters that are
 * not explicitly defined become static.
 * 
 * Revision 9.80  90/12/11  11:23:46  hines
 * forgot to take care of case where there is no TABLE statement
 * 
 * Revision 9.79  90/12/11  10:55:29  hines
 * TABLE works with hocmodl
 * 
 * Revision 9.78  90/12/10  16:56:48  hines
 * TABLE allowed in FUNCTION and PROCEDURE
 * 
 * Revision 9.76  90/12/07  09:27:32  hines
 * new list structure that uses unions instead of void *element
 * 
 * Revision 9.75  90/12/05  11:03:38  hines
 * forgot to add pindex to s->varnum to get index into parray for specific index.
 * 
 * Revision 9.74  90/12/04  13:08:30  hines
 * kept calling initmodel from model. Now use hoc_pindepindex
 * 
 * Revision 9.73  90/12/04  12:00:24  hines
 * model version displayed only as comment in generated c file
 * format of plot lines for scalar in .var file is
 * name nl vindex pindex 1 nl
 * for vector with specific index:
 * name[index] vindex pindex 1
 * for vector without index
 * name[size] vindex pindex size
 * 
 * Revision 9.72  90/11/30  13:10:07  hines
 * dcurdv calculated for ionic currents.
 * 
 * Revision 9.71  90/11/30  08:22:24  hines
 * modl_set_dt should only be created for time dependent models
 * 
 * Revision 9.70  90/11/28  15:35:18  hines
 * much work on case when ion is a state
 * 
 * Revision 9.69  90/11/28  12:54:22  hines
 * ion variables do not all appear in the hoc name list.
 * diam understood
 * 
 * Revision 9.68  90/11/28  09:12:07  hines
 * location where we are checking for STADYSTATE may not be a SYMBOL
 * 
 * Revision 9.67  90/11/27  12:10:46  hines
 * bug in bnd introduced by change to partial 
 * 
 * Revision 9.66  90/11/27  10:47:49  hines
 * allow multiple partial equations within a partial block
 * 
 * Revision 9.65  90/11/26  09:14:06  hines
 * merge didn't recognize units
 * 
 * Revision 9.64  90/11/26  08:16:14  hines
 * external functions of type EXTDEF3 get two extra arguments.
 * The first arg is a pointer to _reset which should be set by the
 * function upon the occurence of a discontinuity.
 * The second arg is a pointer to a double which may be used by the
 * function to store information to help determine if a discontinuity
 * has occurred.  init.c gives a list of these functions.
 * 
 * Revision 9.63  90/11/23  15:07:22  hines
 * factor calculation using units.c of model
 * 
 * Revision 9.62  90/11/23  10:33:18  hines
 * steadystate in BREAKPOINT block had failed to allocate delta_t
 * 
 * Revision 9.61  90/11/23  10:01:50  hines
 * STEADYSTATE of kinetic and derivative blocks
 * 
 * Revision 9.60  90/11/21  10:47:17  hines
 * _ninits is first argument in call to integrators
 * 
 * Revision 9.59  90/11/21  08:41:10  hines
 * merge allows BREAKPOINT and PARAMETER. makefile merge works
 * 
 * Revision 9.58  90/11/20  17:24:22  hines
 * CONSTANT changed to PARAMETER
 * CONSTANT now refers to variables that don't get put in .var file
 * 
 * Revision 9.57  90/11/20  16:04:27  hines
 * EQUATION changed to BREAKPOINT
 * STEADYSTATE keyword added, EQUATION keyword still exists
 * 
 * Revision 9.56  90/11/15  16:16:02  hines
 * unit parentheses for numbers and functions were not being removed.
 * 
 * Revision 9.55  90/11/15  16:05:23  hines
 * units definition syntax is (unit) (unit)
 * 
 * Revision 9.54  90/11/13  15:49:45  hines
 * allow function definitions and function definition arguments to
 * have units specifications. Also numbers can have units specifications.
 * 
 * Revision 9.53  90/11/13  13:10:21  hines
 * nmodl: cachan works pretty well. ions generating current works.
 * 
 * Revision 9.52  90/11/10  15:45:00  hines
 * nmodl: uses new NEURON { USEION ... format
 * passive.c works
 * 
 * Revision 9.51  90/11/02  09:12:02  hines
 * TABLE keyword added. For now it is translated just as FUNCTION
 * 
 * Revision 9.50  90/11/02  08:31:52  hines
 * Allow UNITS block and make a  units defintion scale factor a static
 * variable so that it doesn't have to appear in the var file. eg.
 * UNITS { FARADAY = 96520 (coul) }
 * 
 * Revision 9.49  90/11/02  07:01:15  hines
 * works better. Problem is that conditional macros cannot be used
 * in explicit target.
 * 
 * Revision 9.48  90/11/02  06:59:36  hines
 * new way of specifying NEURON info. Keywords are
 * NEURON SUFFIX CURRENT NEED GET SET
 * 
 * Revision 9.47  90/11/01  15:21:19  hines
 * SOLVE procedure allowed. The procedure needs to return an error code
 * with a VERBATIM statement
 * 
 * Revision 9.46  90/10/30  14:29:23  hines
 * calc_dt is gone. Not needed because due to scopfit, dt needs to be
 * calculated at every break point.
 * _reset no longer used to say that dt has changed. The integrator will
 * have to check that itself.
 * 
 * Revision 9.45  90/10/30  13:56:57  hines
 * derivative blocks (this impacts kinetic and sens as well) now return
 * _reset which can be set with RESET statement.  _reset is static in the
 * file and set to 0 on entry to a derivative or kinetic block.
 * 
 * Revision 9.44  90/10/30  10:25:16  hines
 * modl: saber warning free except for ytab.c and lex.c
 * 
 * Revision 9.43  90/10/30  10:21:30  hines
 * ytab.o was not placed in proper directory
 * 
 * Revision 9.42  90/10/30  10:01:06  hines
 * hmodl saber warning free and test.mod works
 * 
 * Revision 9.41  90/10/30  08:37:13  hines
 * saber warning free except for ytab.c and lex.c
 * 
 * Revision 9.40  90/10/30  08:07:07  hines
 * nmodl: Passive.mod working with index vectors. No longer copying
 * doubles. Just copying two pointers.
 * 
 * Revision 9.39  90/10/30  07:32:47  hines
 * modl, nmodl, hmodl, smodl objects appear in different directories
 * but sources and archive files come from SRC and RCS respectively
 * 
 * Revision 9.38  90/10/29  18:30:47  hines
 * restore after accidental erasure
 * 
 * Revision 9.37  90/10/15  13:08:40  hines
 * parout separated into parout for modl, hparout for hmodl nparout for nmodl
 * 
 * Revision 9.35  90/10/15  12:13:05  hines
 * mistake with checkin
 * 
 * Revision 9.34  90/10/15  11:47:09  hines
 * hmodl: use _p[]
 * 
 * Revision 9.33  90/10/11  15:45:06  hines
 * bugs fixed with respect to conversion from pointer vector to index vector.
 * 
 * Revision 9.32  90/10/08  14:12:58  hines
 * index vector instead of pointer vector for slist and dlist
 * 
 * Revision 9.31  90/10/08  11:34:06  hines
 * simsys prototype
 * 
 * Revision 9.30  90/09/10  14:12:00  hines
 * extdef2.h added to makefile and checked in
 * 
 * Revision 9.29  90/09/10  14:09:18  hines
 * certain functions specified in extdef2.h can have arguments which are
 * pointers to functions and pointers to vectors.
 * 
 * Revision 9.28  90/09/09  17:28:29  hines
 * merge the summer's work at nbsr into modl
 * 
 * Revision 9.27  90/09/05  07:08:26  hines
 * changed spar_rhs to modl_rhs in kinetic.c so that it would not
 * conflict with hoc sparse matrix.  The only externs is sparse.c
 * are now _getelm sparse modl_rhs
 * 
 * Revision 9.26  90/08/30  11:29:07  hines
 * units computations removed and can link with saber-c
 * 
 * Revision 9.25  90/08/30  11:28:24  hines
 * units computations removed
 * 
 * Revision 9.24  90/08/17  08:45:00  hines
 * mistake on last checkin
 * 
 * Revision 9.23  90/08/17  08:39:30  hines
 * no more modltype for double scalars
 * 
 * Revision 9.22  90/08/17  08:15:02  hines
 * HMODL change define from HOC to HMODL, ensure all .c files compiled
 * 
 * Revision 9.21  90/08/17  08:13:46  hines
 * flux input to array state missing a )
 * 
 * Revision 9.20  90/08/15  15:17:03  hines
 * need to pass error value from crank back through the
 * block called by SOLVE.
 * 
 * Revision 9.19  90/08/07  15:36:45  hines
 * forgot name of mechanism. Also append suffix to all range names
 * 
 * Revision 9.18  90/08/07  15:36:03  hines
 * computation of rhs was bad
 * 
 * Revision 9.17  90/08/07  08:43:58  hines
 * functions and procedures interfaced to NMODL and HMODL
 * 
 * Revision 9.16  90/08/02  08:58:49  hines
 * NMODL can use more than one mechanism. Integrators that use ninits and
 * reset will not work, though.
 * 
 * Revision 9.15  90/08/01  07:48:49  hines
 * repair HOC portion after all the errors introduced by NMODL
 * 
 * Revision 9.14  90/07/31  17:03:54  hines
 * NMODL getting close. Compiles but multiple .mod files cause
 * multiple definitions.
 * 
 * Revision 9.13  90/07/30  14:37:20  hines
 * NMODL looks pretty good. Ready to start testing. Have not yet tried
 * to compile the .c file.
 * 
 * Revision 9.12  90/07/30  12:38:04  hines
 * wrong name for ASSIGNED
 * 
 * Revision 9.11  90/07/30  11:51:18  hines
 * NMODL getting better, almost done
 * 
 * Revision 9.10  90/07/27  13:58:11  hines
 * nmodl handles declarations about right for first pass at this.
 * 
 * Revision 9.9  90/07/26  09:25:10  hines
 * beginning interface to NEURON
 * new keywords are PARAMETER and CURRENT
 * 
 * Revision 9.8  90/07/26  07:58:45  hines
 * interface modl to hoc and call the translator hmodl
 * 
 * Revision 9.7  90/07/21  11:36:18  hines
 * allow more than one reactant on -> reaction
 * fix bug where wrong reactant index used in constructing term.
 * 
 * Revision 9.6  90/07/18  08:15:44  hines
 * PARAMETER is synonym for CONSTANT
 * Distinguishing them is necessary for NEURON since PARAMETER's can
 * vary with position on the neuron.
 * 
 * Revision 9.5  90/07/18  08:00:21  hines
 * turn off units.
 * define for arrays now (p + n) instead of &p[n]. This allows the c file
 * to have arrays that look like a[i] instead of *(a + i).
 * 
 * Revision 9.4  90/06/04  08:23:27  mlh
 * extend exponents from both expressions dimensionless to
 * allowing exponent to be positive integer.
 * 
 * Revision 9.3  90/06/01  11:02:07  mlh
 * checks for conformability of units but does not check for
 * correct conversion factors.
 * in x^y, both x and y must be dimensionless
 * 
 * Revision 9.2  90/05/30  17:01:16  mlh
 * not working yet, just adding files administratively
 * 
 * Revision 8.30  90/04/09  08:41:42  mlh
 * implicit method for derivative blocks (allows mixed equations also).
 * The solve statement must precede the derivative block
 * 
 * Revision 8.29  90/04/04  14:15:00  mlh
 * when multiple sparse models were called, MATELM was defined only
 * once and getelm was multiply declared.  Now fixed.
 * 
 * Revision 8.28  90/04/03  07:49:12  mlh
 * typo in previous change. Due to nature of RCS on nbsr, it is unfortunately
 * necessary to check things back in before testing.
 * 
 * Revision 8.27  90/04/03  07:47:02  mlh
 * for turbo-c defs.h stuff must appear after inclusion of scoplib.h
 * This is done by now saving defs.h stuff in defs_list and printing
 * it in cout.c.  Note that each item is type VERBATIM so that there
 * is no prepended space.
 * 
 * Revision 8.26  90/02/15  10:11:54  mlh
 * defs.h no longer included. #defines come from parraycount during
 * handling of .var file.  Thsi handling must be before putting info
 * in .c file
 * 
 * Revision 8.25  90/02/15  08:57:59  mlh
 * first check for .mrg file and translate if it exists
 * if not the check for .mor file and translate that.
 * This is done if there is only one argument.
 * If two arguments then use first as basename and
 * second as input file
 * print message about what it is doing
 * 
 * Revision 8.24  90/02/07  10:23:27  mlh
 * It is important that blocks for derivative and sensitivity also
 * be declared static before their possible use as arguments to other
 * functions and that their body also be static to avoid multiple
 * declaration errors.
 * 
 * Revision 8.23  90/01/29  14:07:37  mlh
 * simplex is a nonlinear method
 * 
 * Revision 8.22  90/01/23  16:11:09  mlh
 * delta_t is calculated for the first two calls after
 * a call to initmodel().  This is because the breakpoint
 * interval can change aftera the first call to model().
 * The implementation is accomplished by _calc_dt=2
 * in initmodel() and _calc_dt-- when _calc_dt is nonzero
 * in model()
 * 
 * Revision 8.21  90/01/19  07:38:07  mlh
 * _modl_cleanup() called by abort_run()
 * 
 * Revision 8.20  90/01/18  11:46:34  mlh
 * SOLVEFOR statement added
 * syntax is 
 *   blocktype blockname [SOLVEFOR name, name, ...] {statment}
 * where blocktype is KINETIC, NONLINEAR, or LINEAR
 * Only the states in the SOLVEFOR statement are solved
 * in that block. If the statement is not present then
 * all states are solved that appear in that block.
 * 
 * Revision 8.19  90/01/18  08:14:22  mlh
 * serious error in calculating p-array index for
 * plot variables in .var file.
 * Fixed by using symbol->varnum to hold the p-index.
 * 
 * Revision 8.18  90/01/16  11:06:28  mlh
 * error checking and cleanup after error and call to abort_run()
 * 
 * Revision 8.17  90/01/03  16:13:44  mlh
 * discrete array variables had their for loops switched.
 * 
 * Revision 8.16  89/12/18  07:58:23  mlh
 * Fixed problem in .var file in which x-axis array variable's dimension
 * was 1 too large.
 * 
 * Revision 8.15  89/12/15  10:11:02  mlh
 * newton's last arg changed to vector of double pointers.
 * For nonlinear equations dlist holds the doubles and
 * pdlist points to them individually.
 * 
 * Revision 8.14  89/11/21  08:18:46  mlh
 * More reliable method for ensuring that solve for loop ends up exactly
 * at the final time value.
 * 
 * Revision 8.13  89/11/21  07:37:11  mlh
 * _calc_dt used in scopcore and therefore must be declared even for
 * time independent models.
 * 
 * Revision 8.12  89/11/17  16:08:28  mlh
 * _ninits tells how many times initmodel() is called. Used by
 * scopmath routines that need to know when to self_initialize.
 * 
 * Revision 8.11  89/11/17  15:12:43  nfh
 * Changed modl_version string printed by SCoP to read: "Language version..."
 * 
 * Revision 8.10  89/11/14  16:38:00  mlh
 * _reset set to true whenever initmodel is called and whenever _calc_dt
 * is true
 * 
 * Revision 8.9  89/11/13  15:17:48  mlh
 * _called_init changed to _calc_dt and made global so
 * that scop can cause dt to be recomputed on an
 * extend run command
 * 
 * Revision 8.8  89/11/01  10:21:17  mlh
 * Three changes
 * consist moved after solvehandler to avoid warning that delta_indep is
 * >> undeclared.
 * Warning given if state is assigned a value in a DERIVATIVE block
 * identical names in nested LOCAL statements declared incorrectly.
 * >> The problem was fixed by not allowing extra _l prepended to
 * >> an already local variable.
 * 
 * Revision 8.7  89/10/30  15:04:34  mlh
 * error message when no derivative equations in DERIVATIVE block
 * 
 * Revision 8.6  89/10/27  15:44:25  mlh
 * initqueue() called in  initmodel() when PUTQ or GETQ used.
 * 
 * Revision 8.5  89/10/25  13:48:15  mlh
 * PUTQ and GETQ implemented
 * 
 * Revision 8.4  89/10/25  13:43:47  mlh
 * Bug with discrete models in that delta_indep is used
 * Fixed by generating the proper line only when type is DERF
 * 
 * Revision 8.3  89/10/11  08:43:27  mlh
 * _reset apparently bein declared elsewhere
 * 
 * Revision 8.2  89/10/11  08:35:29  mlh
 * generate modl_version string in .c file
 * declare _reset=1
 * 
 * Revision 8.1  89/09/29  16:26:09  mlh
 * ifdef for VMS and SYSV and some fixing of assert
 * 
 * Revision 8.0  89/09/22  17:27:06  nfh
 * Freezing
 * 
 * Revision 7.2  89/09/07  07:45:48  mlh
 * 1) a scop independent variable may be a constant and an independent
 * 2)was failing to prevent multiple declarations involveing the SWEEP variable
 * 3) was not initializing time after match
 * 4) many bugs in handling exact for loop of t to the break point
 * >> 
 * 
 * 
 * Revision 7.1  89/09/05  08:10:09  mlh
 * ensure an integer number of steps/break
 * allow us to deal easily with lists of pointers to item pointers
 * COMPARTMENT keyword for KINETIC block
 * 
 * Revision 7.0  89/08/30  13:32:43  nfh
 * Rev 7 is now Experimental; Rev 6 is Testing
 * 
 * Revision 6.3  89/08/24  12:08:11  mlh
 * failed to declare reprime(), make lint free
 * 
 * Revision 6.2  89/08/23  12:03:22  mlh
 * kinetic reaction may involve stepped and independent (sweeped) variable
 * 
 * Revision 6.1  89/08/23  10:32:18  mlh
 * derivative variables in .var file have names like var' instead of Dvar
 * 
 * Revision 6.0  89/08/14  16:27:22  nfh
 * Rev 6.0 is latest of 4.x; now the Experimental version
 * 
 * Revision 4.8  89/08/12  11:04:13  mlh
 * In hierarchical models, CONSTANT and ASSIGNED can be promoted to STATE
 * 
 * Revision 4.7  89/08/11  09:57:29  mlh
 * simultaneous nonlinear equations allowed in DERIVATIVE block
 * CONSTANT and ASSIGNED variables can appear as terms in reactions
 * in a KINETIC block
 * 
 * Revision 4.6  89/08/07  15:35:37  mlh
 * freelist now takes pointer to list pointer and 0's the list pointer.
 * Not doing this is a bug for multiple sens blocks, etc.
 * 
 * Revision 4.5  89/08/05  11:38:45  mlh
 * axis units info now in .var file
 * units for higher order derivatives auto generated
 * .var syntax for higher order derivative states now a:b
 * 
 * Revision 4.4  89/08/01  07:35:47  mlh
 * DEPENDENT keyword changed to ASSIGNED
 * 
 * Revision 4.3  89/07/31  07:56:26  mlh
 * Slight problem with error messages. Now missing partial equation in PARTIAL
 * block says DEL2 instead of DEL
 * 
 * Revision 4.2  89/07/27  13:28:18  mlh
 * crank calling sequence sends indepsym as a sentinal value
 * ugh!
 * 
 * Revision 4.1  89/07/25  19:22:22  mlh
 * fixed error in copying string into too small a space in
 * section handling higher order derivatives.
 * 
 * Revision 4.0  89/07/24  17:03:53  nfh
 * Freezing rev 3.  Rev 4 is now Experimental
 * 
 * Revision 3.10  89/07/21  09:28:21  mlh
 * Discrete equitions evaluated at time given by independent variable
 * in the sense that the state on the left hand side refers to state(t)
 * and explicit dependence on t works naturally.
 * 
 * Revision 3.9  89/07/19  15:05:44  mlh
 * pass t by value introduces bug, return to version before that.
 * 
 * Revision 3.8  89/07/18  15:29:14  mlh
 * integrators now are passed value of time instead of pointer to time
 * 
 * Revision 3.7  89/07/18  11:55:29  mlh
 * first_time removed and MODEL_LEVEL used for declaration precedence
 * 
 * Revision 3.6  89/07/12  16:34:06  mlh
 * 2nd optional argument gives input file.  First arg is prefix to .var
 * and .c files (and .mod input file if 2nd arg not present).
 * 
 * Revision 3.5  89/07/12  12:24:03  mlh
 * state@1 now refers to state value at end of previous step
 * state@0 is a syntax error
 * 
 * Revision 3.4  89/07/11  19:32:26  mlh
 * missing ) from previous checkin
 * 
 * Revision 3.3  89/07/11  19:30:00  mlh
 * Bug in which initial condition could not be a STEPPED variable is fixed
 * 
 * Revision 3.2  89/07/11  16:56:58  mlh
 * remove p array from calling sequence of crank
 * 
 * Revision 3.1  89/07/07  16:55:24  mlh
 * FIRST LAST START in independent SWEEP higher order derivatives
 * 
 * Revision 1.1  89/07/07  16:25:06  mlh
 * Initial revision
 * 
 * Revision 3.1  89/07/07  08:57:37  mlh
 * major revision
 * 
 * Revision 1.2  89/07/06  15:35:04  mlh
 * inteface with version.c
 * 
 * Revision 1.1  89/07/06  15:27:11  mlh
 * Initial revision
 * 
*/
char *RCS_version = "$Revision: 1098 $";
char *RCS_date = "$Date: 2005-10-04 22:55:37 +0200 (Tue, 04 Oct 2005) $";


