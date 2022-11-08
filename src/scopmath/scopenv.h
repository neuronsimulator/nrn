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

#define BSD4_2        /* For Berkeley UNIX or EUNICE          */

/****************************************************************************/

/* Do NOT modify any of the following statements */

#ifdef BSD4_2
#define UNIX                    /* Always define UNIX if BSD4_2 is defined       */
#endif

/*
 * Default directory paths for SCoP and SCoPfit help files
 * You must edit the path strings if you want to put the help files
 * in another directory.  Remember to use "\\" in the path string for
 * the IBM PC; a single '\' is the "escape" signal in the C language.
 */
 
#define DIRECTORY "/usr/local/lib/scop/"
