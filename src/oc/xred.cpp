#include <../../nrnconf.h>
/* /local/src/master/nrn/src/oc/xred.cpp,v 1.3 1996/02/16 16:19:33 hines Exp */
/*
xred.cpp,v
 * Revision 1.3  1996/02/16  16:19:33  hines
 * OCSMALL used to throw out things not needed by teaching programs
 *
 * Revision 1.2  1995/04/03  13:58:43  hines
 * Port to MSWindows
 *
 * Revision 1.1.1.1  1994/10/12  17:22:16  hines
 * NEURON 3.0 distribution
 *
 * Revision 2.22  93/02/12  08:51:42  hines
 * beginning of port to PC-Dos
 *
 * Revision 2.20  93/02/03  11:27:59  hines
 * a bit more generic for portability. will possibly work on NeXT
 *
 * Revision 2.7  93/01/12  08:58:48  hines
 * to assign to a hoc string use
 * hoc_assign_str(char** cpp, char* buf)
 * Some minor modifications to allow use of some functions by File class
 *
 * Revision 1.2  92/08/12  10:45:40  hines
 * Changes of sejnowski lab. also gets sred from hoc. Major addition is
 * a new x.cpp which is modified for screen updating following an expose
 * event. This is in x_sejnowski.cpp and will compile to x.o when
 * Sejnowski is defined as 1 in Imakefile (don't forget imknrn -a when
 * changed and delete old x.o)
 * Does not contain get_default on this checkin
 *
 * Revision 1.2  1992/06/30  23:14:11  fisher
 * added strstr() function for the MIPS - the MIPS' string.h doesn't
 * contain strstr() (see hoc.h comment).
 *
 * Revision 1.1  1992/05/22  19:35:47  fisher
 * Initial revision
 *
 * Revision 4.59  92/04/13  11:09:08  hines
 * Stewart Jasloves contribution of sred(). usage is
 * i = sred("prompt", "default", "charlist")
 * type one of the characters in the charlist. the default becomes that
 * character. return value is position in charlist (0 if first char).
 *
 * Revision 1.1  90/02/14  09:47:19  mlh
 * Initial revision
 *
*/

#include "hoc.h"


/* input a n integer in range > min and < max */
int ired(const char* prompt, int defalt, int min, int max) {
    return ((int) xred(prompt, (double) defalt, (double) min, (double) max));
}
/* input a double number in range > min  and < max
    program loops til proper number is typed in by user
    prompt and default are typed by computer
    default is used if user types RETURN key
    input is freeform as scanf can make it.
*/
#include <stdio.h>
double xred(const char* prompt, double defalt, double min, double max) {
#if !OCSMALL
    char istr[80], c[2];
    double input;
    for (;;) {
        IGNORE(fprintf(stderr, "%s (%-.5g)", prompt, defalt));
#ifdef WIN32
        if (gets(istr) != NULL) {
            strcat(istr, "\n");
#else
        if (fgets(istr, 79, stdin) != NULL) {
#endif
            if (istr[0] == '\n') {
                input = defalt;
                goto label;
            }
            if (sscanf(istr, "%lf%1s", &input, c) == 1)
                if (sscanf(istr, "%lf", &input) == 1)
                label: {
                    if (input >= min && input <= max)
                        return (input);
                    IGNORE(fprintf(stderr, "must be > %-.5g and < %-.5g\n", min, max));
                    continue;
                }
        } else {
            rewind(stdin);
        }
        IGNORE(fprintf(stderr, "input error\n"));
    }
#else
    return 0.;
#endif
}


/*   hoc_Sred.cpp  SW Jaslove   March 23, 1992
      This is the hoc interface for the sred() function, which follows.
*/

void hoc_Sred(void) {
#if !OCSMALL
    char defalt[80], **pdefalt;
    double x;
    strcpy(defalt, gargstr(2));
    pdefalt = hoc_pgargstr(2);
    x = (double) hoc_sred(gargstr(1), defalt, gargstr(3));
    hoc_assign_str(pdefalt, defalt);
#else
    double x = 0.;
#endif
    ret();
    hoc_pushx(x);
}
#if !OCSMALL

/*   sred.cpp  SW Jaslove    March 23, 1992
          n = sred(prompt,default,charlist)
     Outputs a prompt and inputs a string which MUST be a member of charlist.
     If default exists: default is returned if user types RETURN key only.
     If default is null: a string MUST be entered, there is no default.
     If charlist is null and default exists: any typed string is accepted.
     Default MUST be a member of charlist, so a return value can be specified.
     Input is terminated by RETURN or first space after a nonspace char.
     Program loops until proper input is typed in by user.
     RETURNS: Starting position of input in charlist, beginning with 0.
           *** NOTE: default is replaced by entered string ***
*/

int hoc_sred(const char* prompt, char* defalt, char* charlist) {
    char istr[80], c[2], instring[40], *result;

    for (;;) {                                              /* cycle until done */
        IGNORE(fprintf(stderr, "%s (%s)", prompt, defalt)); /* print prompt */
#ifdef WIN32
        if (gets(istr) != NULL) {
            strcat(istr, "\n");
#else
        if (fgets(istr, 79, stdin) != NULL) { /* read input */
#endif
            if (defalt[0] != '\0' && istr[0] == '\n') {
                strcpy(istr, defalt); /* if CR only, use default */
            } else {
                istr[strlen(istr) - 1] = '\0'; /* if real input, strip return */
            }
            if (sscanf(istr, "%s%s", instring, c) == 1) { /* only single input */
                if (charlist == NULL) {                   /* if charlist is null: */
                    strcpy(defalt, instring);             /* accept any input, so */
                    return (0);                           /* update default and return 0 */
                }
                if ((result = strstr(charlist, instring)) != NULL) {
                    strcpy(defalt, instring);   /* if input is in charlist: */
                    return (result - charlist); /* update default and return pos */
                }
            }
            IGNORE(fprintf(stderr, "input must be a substring of <<%s>>\n", charlist));
            continue; /* go back for another cycle */
        } else {
            rewind(stdin);
        }
        IGNORE(fprintf(stderr, "input error\n")); /* recycle */
    }
    return 0;
}

#endif
