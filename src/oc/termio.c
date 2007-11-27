#include <../../nrnconf.h>
#include <stdio.h>
/*
 * Automake doesn't deal well with sources that live in other directories, so
 * this is a quick and dirty workaround.
 */
#if !defined(WITHOUT_MEMACS)
#include "../memacs/termio.c"
#endif
