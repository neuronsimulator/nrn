#include <../../nmodlconf.h>
/* /local/src/master/nrn/src/modlunit/version.c,v 1.1.1.1 1994/10/12 17:22:51 hines Exp */

const char* RCS_version = "1.1.1.1";
const char* RCS_date = "1994/10/12 17:22:51";

/* version.c,v
 * Revision 1.1.1.1  1994/10/12  17:22:51  hines
 * NEURON 3.0 distribution
 *
 * Revision 1.53  1994/09/26  18:51:06  hines
 * USEION ... VALENCE real
 *
 * Revision 1.52  1994/09/20  14:43:18  hines
 * port to dec alpha
 *
 * Revision 1.51  1994/08/23  15:43:48  hines
 * if no independent block then t (ms) declared for neuron
 *
 * Revision 1.50  1994/08/18  12:09:42  hines
 * check units of CONSERVE stmt
 *
 * Revision 1.49  1994/08/05  13:59:11  hines
 * NullParameter replaces null arg in Imakefile
 *
 * Revision 1.48  1994/07/30  17:12:53  hines
 * standard check for BISON
 *
 * Revision 1.47  1994/06/01  17:31:39  hines
 * look for gnu version of units.lib in $NEURONHOME/lib/nrnunits.lib
 *
 * Revision 1.46  1994/05/23  17:56:02  hines
 * error in handling MODL_INCLUDE when no file exists
 *
 * Revision 1.45  1994/05/18  18:08:13  hines
 * INCLUDE "file"
 * tries originalpath/file ./file MODL_INCLUDEpaths/file
 *
 * Revision 1.44  1994/03/17  15:21:11  hines
 * ? token same as comment : token
 *
 * Revision 1.43  1993/11/04  15:52:23  hines
 * port to solaris2 (no more warnings)
 *
 * Revision 1.42  1993/07/08  14:36:58  hines
 * An alternative to NONSPECIFIC_CURRENT is ELECTRODE_CURRENT
 *
 * Revision 1.41  93/02/15  08:58:51  hines
 * linux
 *
 * Revision 1.40  93/02/11  17:00:10  hines
 * now works on NeXT
 *
 * Revision 1.39  93/02/01  15:15:52  hines
 * static functions should be declared before use
 *
 * Revision 1.38  92/11/16  14:15:16  hines
 * small error in Imakefila
 *
 * Revision 1.37  92/10/24  11:31:35  hines
 * when no units table , say what to do.
 *
 * Revision 1.36  92/06/01  13:25:52  hines
 * NEURON {EXTERNAL name, name, ...} allowed
 *
 * Revision 1.35  92/05/05  08:30:07  hines
 * need to link with math library (at least for RS6000)
 *
 * Revision 1.34  92/03/09  17:24:18  hines
 * internal error in handling stoichiometry units. fixed.
 *
 * Revision 1.33  92/02/17  12:31:02  hines
 * constant states with a compartment size didn't have space allocated
 * to store the compartment size.
 *
 * Revision 1.32  92/02/15  21:20:23  hines
 * if translated by bison use -DBISON=1
 * NEURON { POINTER ...} allowed
 *
 * Revision 1.31  92/01/28  14:17:29  hines
 * distribution ready
 *
 * Revision 1.30  91/12/27  10:57:25  hines
 * units for f_flux and b_flux in KINETIC block.
 * Implemented by turning off units checking on pass 3 of restartpass 0
 * and then turning it back on on pass3 of restartpass 1.
 *
 * Revision 1.29  91/10/28  08:53:36  hines
 * Wathey's improvements. Runs on MIPS. Binaries go to bin directory.
 *
 * Revision 1.28  91/09/16  16:04:04  hines
 * NEURON { RANGE SECTION GLOBAL} syntax
 *
 * Revision 1.27  91/08/13  10:05:13  hines
 * to work on rs6000
 *
 * Revision 1.26  91/08/12  11:38:03  hines
 * more generic makefile. MACHDEF changes if machine dependent definitions
 * are needed.
 *
 * Revision 1.25  91/06/29  08:46:57  hines
 * more general units factor of form name=(unit) (unit)
 * slight change to syntax
 *
 * Revision 1.24  91/06/25  09:18:15  hines
 * fix loose ends from last major change to makefile
 *
 * Revision 1.23  91/06/25  07:27:49  hines
 * more generic . remove sun4 isms
 *
 * Revision 1.22  91/02/09  16:39:43  hines
 * special neuron variables checked for correct units.
 *
 * Revision 1.21  91/01/29  07:10:50  hines
 * POINT_PROCESS keyword allowed
 *
 * Revision 1.20  91/01/28  17:26:16  hines
 * works with turboc++  (different varargs method)
 *
 * Revision 1.19  91/01/25  14:01:17  hines
 * forgot to checkin on last fix
 *
 * Revision 1.18  91/01/25  09:31:40  hines
 * botched last fix
 *
 * Revision 1.17  91/01/24  15:25:23  hines
 * translation error when last token of LOCAL statement was the first token
 * after the LOCAL statement. Fixed by changing symbols at the parser insteadof the lexical
 * analyser.
 *
 * Revision 1.16  91/01/24  11:46:24  hines
 * PROCEDURE argument units were not being saved.
 *
 * Revision 1.15  91/01/07  14:31:57  hines
 * Compartment list is space separated not comma separated
 *
 * Revision 1.14  91/01/07  14:17:13  hines
 * in kinunit, wrong itemsubtype.  Fix lint messages
 *
 * Revision 1.13  90/12/12  11:33:14  hines
 * LOCAL vectors allowed. Some more NEURON syntax added
 *
 * Revision 1.12  90/12/06  15:25:55  hines
 * 5 of 7 lint messages in units.c removed
 *
 * Revision 1.11  90/11/26  13:20:09  hines
 * after adding unit_swap, the comparison needs to be reversed.
 * Now the last expression in on the top of the stack normally.
 *
 * Revision 1.10  90/11/26  13:09:54  hines
 * sometimes need to swap top of units stack.
 *
 * Revision 1.9  90/11/23  15:08:40  hines
 * used by MODL to calculate factors
 *
 * Revision 1.8  90/11/23  13:58:16  hines
 * don't print units init message.
 *
 * Revision 1.7  90/11/23  13:44:01  hines
 * BREAKPOINT PARAMETER
 *
 * Revision 1.6  90/11/20  15:40:03  hines
 * comment out print statements that debug value, units for factordef:
 *
 * Revision 1.5  90/11/20  15:33:12  hines
 * added 4 varieties of unit factors. They are
 * name = (real)
 * name = ((unit) -> (unit))	must be conformable
 * name = (physical_constant)
 * name = (physical_constant (unit))	must be conformable
 *
 * Revision 1.4  90/11/16  14:23:07  hines
 * units for partial derivative equation. fixed bug in array declarations
 * get sources from rcs correctly
 *
 * Revision 1.3  90/11/16  07:53:48  hines
 * take out the .c and .var file
 *
 * Revision 1.2  90/11/15  13:01:23  hines
 * function units and number units work. accepts NEURON block
 *
 * Revision 1.1  90/11/13  16:10:38  hines
 * Initial revision
 *  */
