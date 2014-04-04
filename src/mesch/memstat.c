#include <../../nrnconf.h>

/**************************************************************************
**
** Copyright (C) 1993 David E. Steward & Zbigniew Leyk, all rights reserved.
**
**			     Meschach Library
** 
** This Meschach Library is provided "as is" without any express 
** or implied warranty of any kind with respect to this software. 
** In particular the authors shall not be liable for any direct, 
** indirect, special, incidental or consequential damages arising 
** in any way from use of the software.
** 
** Everyone is granted permission to copy, modify and redistribute this
** Meschach Library, provided:
**  1.  All copies contain this copyright notice.
**  2.  All modified copies shall carry a notice stating who
**      made the last modification and the date of such modification.
**  3.  No charge is made for this software or works derived from it.  
**      This clause shall not be construed as constraining other software
**      distributed on the same medium as this software, nor is a
**      distribution fee considered a charge.
**
***************************************************************************/


/*  mem_stat.c    6/09/93  */

/* Deallocation of static arrays */


#include <stdio.h>
#include  "matrix.h"
#include  "meminfo.h"
#ifdef COMPLEX   
#include  "zmatrix.h"
#endif
#ifdef SPARSE
#include  "sparse.h"
#include  "iter.h"
#endif

static char rcsid[] = "memstat.c,v 1.1 1997/12/04 17:55:39 hines Exp";

/* global variable */

extern MEM_CONNECT mem_connect[MEM_CONNECT_MAX_LISTS];


/* local type */

typedef struct {
   void **var;    /* for &A, where A is a pointer */
   int type;     /* type of A */
   int mark;      /* what mark is chosen */
} MEM_STAT_STRUCT;


/* local variables */

/* how many marks are used */
static int mem_stat_mark_many = 0;

/* current mark */
static int mem_stat_mark_curr = 0;


static MEM_STAT_STRUCT mem_stat_var[MEM_HASHSIZE];

/* array of indices (+1) to mem_stat_var */
static unsigned int mem_hash_idx[MEM_HASHSIZE];

/* points to the first unused element in mem_hash_idx */
static unsigned int mem_hash_idx_end = 0;



/* hashing function */

static unsigned int mem_hash(ptr)
void **ptr;
{
   unsigned long lp = (size_t)ptr;

   return (lp % MEM_HASHSIZE);
}


/* look for a place in mem_stat_var */
static int mem_lookup(var)
void **var;
{
   int k, j;

   k = mem_hash(var);

   if (mem_stat_var[k].var == var) {
      return -1;
   }
   else if (mem_stat_var[k].var == NULL) {
      return k;
   }
   else {  /* look for an empty place */
      j = k;
      while (mem_stat_var[j].var != var && j < MEM_HASHSIZE
	     && mem_stat_var[j].var != NULL) 
	j++;

      if (mem_stat_var[j].var == NULL) return j;
      else if (mem_stat_var[j].var == var) return -1; 
      else { /* if (j == MEM_HASHSIZE) */
	 j = 0;
	 while (mem_stat_var[j].var != var && j < k
		&& mem_stat_var[j].var != NULL) 
	   j++;
	 if (mem_stat_var[j].var == NULL) return j;
	 else if (mem_stat_var[j].var == var) return -1; 
	 else { /* if (j == k) */
	    fprintf(stderr,
              "\n WARNING !!! static memory: mem_stat_var is too small\n");
	    fprintf(stderr,
	      " Increase MEM_HASHSIZE in file: %s (currently = %d)\n\n",
		    MEM_HASHSIZE_FILE, MEM_HASHSIZE);
	    if ( !isatty(fileno(stdout)) ) {
	       fprintf(stdout,
                "\n WARNING !!! static memory: mem_stat_var is too small\n");
	       fprintf(stdout,
	        " Increase MEM_HASHSIZE in file: %s (currently = %d)\n\n",
		    MEM_HASHSIZE_FILE, MEM_HASHSIZE);
	    }
	    error(E_MEM,"mem_lookup");
	 }
      }
   }

   return -1;
}


/* register static variables;
   Input arguments:
     var - variable to be registered,
     type - type of this variable; 
     list - list of types

   returned value < 0  --> error,
   returned value == 0 --> not registered,
   returned value >= 0 --> registered with this mark;
*/

int mem_stat_reg_list(var,type,list)
void **var;
int type,list;
{
   int n;

   if ( list < 0 || list >= MEM_CONNECT_MAX_LISTS )
     return -1;

   if (mem_stat_mark_curr == 0) return 0;  /* not registered */
   if (var == NULL) return -1;             /* error */

   if ( type < 0 || type >= mem_connect[list].ntypes || 
       mem_connect[list].free_funcs[type] == NULL )
   {
      warning(WARN_WRONG_TYPE,"mem_stat_reg_list");
      return -1;
   }
   
   if ((n = mem_lookup(var)) >= 0) {
      mem_stat_var[n].var = var;
      mem_stat_var[n].mark = mem_stat_mark_curr;
      mem_stat_var[n].type = type;
      /* save n+1, not n */
      mem_hash_idx[mem_hash_idx_end++] = n+1;
   }

   return mem_stat_mark_curr;
}


/* set a mark;
   Input argument:
   mark - positive number denoting a mark;
   returned: 
             mark if mark > 0,
             0 if mark == 0,
	     -1 if mark is negative.
*/

int mem_stat_mark(mark)
int mark;
{
   if (mark < 0) {
      mem_stat_mark_curr = 0;
      return -1;   /* error */
   }
   else if (mark == 0) {
      mem_stat_mark_curr = 0; 
      return 0; 
   }

   mem_stat_mark_curr = mark;
   mem_stat_mark_many++;

   return mark;
}



/* deallocate static variables;
   Input argument:
   mark - a positive number denoting the mark;

   Returned:
     -1 if mark < 0 (error);
     0  if mark == 0;
*/

int mem_stat_free_list(mark,list)
int mark,list;
{
   u_int i,j;
   int	 (*free_fn)();

   if ( list < 0 || list >= MEM_CONNECT_MAX_LISTS 
       || mem_connect[list].free_funcs == NULL )
     return -1;

   if (mark < 0) {
      mem_stat_mark_curr = 0;
      return -1;
   }
   else if (mark == 0) {
      mem_stat_mark_curr = 0;
      return 0;
   }
   
   if (mem_stat_mark_many <= 0) {
      warning(WARN_NO_MARK,"mem_stat_free");
      return -1;
   }

   /* deallocate the marked variables */
   for (i=0; i < mem_hash_idx_end; i++) {
      j = mem_hash_idx[i];
      if (j == 0) continue;
      else {
	 j--;
	 if (mem_stat_var[j].mark == mark) {
	     free_fn = mem_connect[list].free_funcs[mem_stat_var[j].type];
	     if ( free_fn != NULL )
		 (*free_fn)(*mem_stat_var[j].var);
	     else
		 warning(WARN_WRONG_TYPE,"mem_stat_free");
	    
	    *(mem_stat_var[j].var) = NULL;
	    mem_stat_var[j].var = NULL;
	    mem_stat_var[j].mark = 0;
	    mem_hash_idx[i] = 0;
	 }
      }
   }

   while (mem_hash_idx_end > 0 && mem_hash_idx[mem_hash_idx_end-1] == 0)
     mem_hash_idx_end--;

   mem_stat_mark_curr = 0;
   mem_stat_mark_many--;
   return 0;
}


/* only for diagnostic purposes */

void mem_stat_dump(fp,list)
FILE *fp;
int list;
{
   u_int i,j,k=1;

   if ( list < 0 || list >= MEM_CONNECT_MAX_LISTS 
       || mem_connect[list].free_funcs == NULL )
     return;

   fprintf(fp," Array mem_stat_var (list no. %d):\n",list);
   for (i=0; i < mem_hash_idx_end; i++) {
      j = mem_hash_idx[i];
      if (j == 0) continue;
      else {
	 j--;
	 fprintf(fp," %d.  var = 0x%p, type = %s, mark = %d\n",
		 k,mem_stat_var[j].var,
		 mem_stat_var[j].type < mem_connect[list].ntypes &&
		 mem_connect[list].free_funcs[mem_stat_var[j].type] != NULL ?
		 mem_connect[list].type_names[(int)mem_stat_var[j].type] : 
		 "???",
		 mem_stat_var[j].mark);
	 k++;
      }
   }
   
   fprintf(fp,"\n");
}


/* query function about the current mark */
#ifdef ANSI_C
int mem_stat_show_mark(void)
#else
int mem_stat_show_mark()
#endif
{
   return mem_stat_mark_curr;
}


/* Varying number of arguments */


#ifdef ANSI_C

/* To allocate memory to many arguments. 
   The function should be called:
   mem_stat_vars(list,type,&v1,&v2,&v3,...,VNULL);
   where 
     int list,type;
     void **v1, **v2, **v3,...;
     The last argument should be VNULL ! 
     type is the type of variables v1,v2,v3,...
     (of course they must be of the same type)
*/

int mem_stat_reg_vars(int list,int type,...)
{
   va_list ap;
   int i=0;
   void **par;
   
   va_start(ap, type);
   while ((par = va_arg(ap,void **))) {   /* NULL ends the list*/
      mem_stat_reg_list(par,type,list);
      i++;
   } 

   va_end(ap);
   return i;
}

#elif VARARGS
/* old varargs is used */

/* To allocate memory to many arguments. 
   The function should be called:
   mem_stat_vars(list,type,&v1,&v2,&v3,...,VNULL);
   where 
     int list,type;
     void **v1, **v2, **v3,...;
     The last argument should be VNULL ! 
     type is the type of variables v1,v2,v3,...
     (of course they must be of the same type)
*/

int mem_stat_reg_vars(va_alist) va_dcl
{
   va_list ap;
   int type,list,i=0;
   void **par;
   
   va_start(ap);
   list = va_arg(ap,int);
   type = va_arg(ap,int);
   while ((par = va_arg(ap,void **))) {   /* NULL ends the list*/
      mem_stat_reg_list(par,type,list);
      i++;
   } 

   va_end(ap);
   return i;
}


#endif
