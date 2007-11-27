/******************************************************************************
 *
 * File: scopenv.h
 *
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989
 *   Duke University
 *
 * scopenv.h,v 1.1.1.1 1994/10/12 17:22:18 hines Exp
 *
 ******************************************************************************/

/****************************************************************************/
/*                                                                          */
/* Enable one of the following statements to define the program environment */
/*                                                                          */
/****************************************************************************/

/*
 *  Define the system environment by removing the initial "/*" for only one of
 *  the definitions for VMS, BSD4_2, SYSTEMV, MSIBMPC, or TURBOIBMPC
 */

#define BSD4_2        /* For Berkeley UNIX or EUNICE          */

/*#define SYSTEMV       /* For Bell Labs SYSTEM V UNIX          */

/*#define VMS           /* For VAX-11 C under VMS               */

/*define MSIBMPC        /* For IBMPC with Microsoft C compiler  */

/*#define TURBOIBMPC      /* For Borland Turbo C                  */

/****************************************************************************/

/* Do NOT modify any of the following statements */

#ifdef MSIBMPC
#define IBMPC
#endif

#ifdef TURBOIBMPC
#define IBMPC
#endif

#ifdef VMS
#define UNIX                    /* Always define UNIX if VMS is defined          */
#endif

#ifdef BSD4_2
#define UNIX                    /* Always define UNIX if BSD4_2 is defined       */
#endif

#ifdef SYSTEMV
#define UNIX                    /* Always define UNIX if SYSTEM V is defined     */
#endif
 
/*
 * Default directory paths for SCoP and SCoPfit help files
 * You must edit the path strings if you want to put the help files
 * in another directory.  Remember to use "\\" in the path string for
 * the IBM PC; a single '\' is the "escape" signal in the C language.
 */
 
#define VMSDIR "SCOP$LIB:"
#define UNIXDIR "/usr/local/lib/scop/"
#define MSDOSDIR "C:\\scop\\"

#ifdef VMS                      /* For VMS environment */
#define DIRECTORY VMSDIR
 
#else
#ifdef UNIX                     /* For Unix envirnoment */
#define DIRECTORY UNIXDIR

#else                           /* For IBM PC environment */
#define DIRECTORY MSDOSDIR
#endif
 
#endif

