/* /local/src/master/nrn/src/memacs/epath.h,v 1.1.1.1 1994/10/12 17:21:23 hines Exp */
/*
epath.h,v
 * Revision 1.1.1.1  1994/10/12  17:21:23  hines
 * NEURON 3.0 distribution
 *
 * Revision 1.1  89/07/08  15:37:48  mlh
 * Initial revision
 * 
*/

/*	PATH:	This file contains certain info needed to locate the
		MicroEMACS files on a system dependant basis.

									*/

/*	possible names and paths of help files under different OSs	*/

char *pathname[] = {

#if	AMIGA
	".emacsrc",
	"emacs.hlp",
	"",
	":c/",
	":t/"
#endif

#if	MSDOS
	"emacs.rc",
	"emacs.hlp",
	"",
	"\\lib\\",
	"\\mail\\lib\\",
	"\\",
	"\\bin\\"
#endif

#if	V7
	".emacsrc",
	"emacs.hlp",
	"/usr/local/lib/memacs36b1/",
	""
#endif

#if	VMS
	"emacs.rc",
	"emacs.hlp",
	"",
	"sys$sysdevice:[vmstools]"
#endif

};

#define	NPNAMES	(sizeof(pathname)/sizeof(char *))
