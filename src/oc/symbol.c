#include <../../nrnconf.h>
/* /local/src/master/nrn/src/oc/symbol.c,v 1.9 1999/02/25 18:01:58 hines Exp */
/* version 7.2.1 2-jan-89 */

#if HAVE_POSIX_MEMALIGN
#define HAVE_MEMALIGN 1
#endif
#if defined(DARWIN) /* posix_memalign seems not to work on Darwin 10.6.2 */
#undef HAVE_MEMALIGN
#endif
#if HAVE_MEMALIGN
#define _XOPEN_SOURCE 600
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hoc.h"
#include "parse.h"
#include "hoclist.h"
#if MAC
#undef HAVE_MALLOC_H
#endif
#if HAVE_MALLOC_H 
#include <malloc.h>
#endif
#if HAVE_ALLOC_H
#include <alloc.h>	/* at least for turbo C 2.0 */
#endif

#if OOP
Symlist	*hoc_built_in_symlist = (Symlist *)0; /* keywords, built-in functions,
	all name linked into hoc. Look in this list last */
Symlist	*hoc_top_level_symlist = (Symlist *)0; /* all user names seen at top-level
		(non-public names inside templates do not appear here) */
extern Objectdata *hoc_top_level_data;
#endif /*OOP*/
						
Symlist	*symlist = (Symlist *)0;	/* the current user symbol table: linked list */
Symlist	*p_symlist = (Symlist *)0; /* current proc, func, or temp table */
			/* containing constants, strings, and auto */
			/* variables. Discarding these lists at */
			/* appropriate times prevents storage leakage. */

void print_symlist(s, tab) char* s; Symlist *tab; {
	Symbol *sp;
	printf("%s\n", s);
	if (tab) for (sp=tab->first ; sp != (Symbol *) 0; sp = sp->next) {
		printf("%s %lx\n", sp->name, (long)sp);
	}
}

Symbol *hoc_table_lookup(s, tab) /* find s in specific table */
	char *s;
	Symlist *tab;
{
	Symbol *sp;
	if (tab) for (sp=tab->first ; sp != (Symbol *) 0; sp = sp->next) {
		if (strcmp(sp->name, s) == 0) {
			return sp;
		}
	}
	return (Symbol *)0;
}

Symbol *
lookup(s)	/* find s in symbol table */
	char *s; /* look in p_symlist then built_in_symlist then symlist */
{
	Symbol *sp;

	if ((sp = hoc_table_lookup(s, p_symlist)) != (Symbol *)0) {
		return sp;
	}
	if ((sp = hoc_table_lookup(s, symlist)) != (Symbol *)0) {
		return sp;
	}
#if OOP
	if ((sp = hoc_table_lookup(s, hoc_built_in_symlist)) != (Symbol *)0) {
		return sp;
	}
#endif

	return 0;	/* 0 ==> not found */
}

Symbol *
install(s, t, d, list)	/* install s in the list symbol table */
	char *s;
	int t;
	double d;
	Symlist **list;
{
	Symbol *sp;

	sp = (Symbol *) emalloc(sizeof(Symbol));
	sp->name = (char *)emalloc((unsigned)(strlen(s)+1));	/* +1 for '\0' */
	Strcpy(sp->name, s);
	sp->type = t;
	sp->subtype = NOTUSER;
	sp->defined_on_the_fly = 0;
	sp->public = 0;
	sp->s_varn = 0;
	sp->arayinfo = (Arrayinfo *)0;
	sp->extra = (HocSymExtension*)0;
	if (!(*list)) {
		*list = (Symlist *)emalloc(sizeof(Symlist));
		(*list)->first = (*list)->last = (Symbol *)0;
	}
	hoc_link_symbol(sp, *list);
	switch (t) {
	case NUMBER:
		sp->u.pnum = (double *)emalloc(sizeof(double));
		*sp->u.pnum = d;
		break;
	case VAR:
		hoc_install_object_data_index(sp);
		OPVAL(sp) = (double *)emalloc(sizeof(double));
		*(OPVAL(sp)) = d;
		break;
	case PROCEDURE:
	case FUNCTION:
	case FUN_BLTIN:
	case OBFUNCTION:
	case STRFUNCTION:
		sp->u.u_proc = (Proc *)emalloc(sizeof(Proc));
		sp->u.u_proc->list = (Symlist*)0;
		sp->u.u_proc->size = 0;
		break;
	default:
		sp->u.pnum = (double *)0;
		break;
	}
	return sp;
}

Symbol* hoc_install_var(name, pval) char* name; double* pval; {
	Symbol* s;
	s = hoc_install(name, UNDEF, 0.0, &symlist);
	s->type = VAR;
	s->u.pval = pval;
	s->subtype = USERDOUBLE;
	return s;
}

hoc_unlink_symbol(s, list)
	Symbol* s;
	Symlist* list;
{
	Symbol *sp;
	assert(list);
	
	if (list->first == s) {
		list->first = s->next;
		if (list->last == s) {
			list->last = (Symbol*)0;
		}
	}else {
		for (sp = list->first ; sp != (Symbol *) 0; sp = sp->next) {
			if (sp->next == s) {
				break;
			}
		}
		assert(sp);
		sp->next = s->next;
		if (list->last == s) {
			list->last = sp;
		}
	}
	s->next = (Symbol*)0;
}

hoc_link_symbol(sp, list)
	Symbol* sp;
	Symlist* list;
{
	/* put at end of list */
	if (list->last) {
		list->last->next = sp;
	}else{
		list->first= sp;
	}
	list->last = sp;
	sp->next = (Symbol *)0;
}

static int emalloc_error=0;

hoc_malchk() {
	if (emalloc_error) {
		emalloc_error = 0;
		execerror("out of memory", (char *) 0);
	}
}
	
#if LINT
double *
#else
char *
#endif
hoc_Emalloc(n)	/* check return from malloc */
	unsigned n;
{
	char *p;

	p = (char *)malloc(n);
	if (p == 0)
		emalloc_error = 1;
#if LINT
	{static double *p; return p;}
#else
	return p;
#endif
}

#if LINT
double *
#else
char *
#endif
hoc_Ecalloc(n, size)	/* check return from calloc */
	unsigned n, size;
{
	char *p;

	if (n == 0) {
		return (char*)0;
	}
	p = (char *)calloc(n, size);
	if (p == 0)
		emalloc_error = 1;
#if LINT
	{static double *p; return p;}
#else
	return p;
#endif
}

void* nrn_cacheline_alloc(void** memptr, size_t size) {
#if HAVE_MEMALIGN
	static int memalign_is_working = 1;
	if (memalign_is_working) {
		if (posix_memalign(memptr, 64, size) != 0) {
fprintf(stderr, "posix_memalign not working, falling back to using malloc\n");
			memalign_is_working = 0;
			*memptr = hoc_Emalloc(size); hoc_malchk();
		}
	}else
#endif
	*memptr = hoc_Emalloc(size); hoc_malchk();
	return *memptr;
}

void* nrn_cacheline_calloc(void** memptr, size_t nmemb, size_t size) {
	int i, n;
#if HAVE_MEMALIGN
	nrn_cacheline_alloc(memptr, nmemb*size);
	memset(*memptr, 0, nmemb*size);
#else
	*memptr = hoc_Ecalloc(nmemb, size); hoc_malchk();
#endif
	return *memptr;
}
#if LINT
double *
#else
char *
#endif
hoc_Erealloc(ptr, size)	/* check return from realloc */
	char *ptr;
	unsigned size;
{
	char *p;

	if (!ptr) {
		return hoc_Emalloc(size);
	}
	p = (char *)realloc(ptr, size);
	if (p == 0) {
		free(ptr);
		emalloc_error = 1;
	}
#if LINT
	{static double *p; return p;}
#else
	return p;
#endif
}

hoc_free_symspace(s1)	/* frees symbol space. Marks it UNDEF */
	Symbol *s1;
{
	if (s1 && s1->public != 2) {
		switch (s1->type)
		{
		case UNDEF:
			break;
		case STRING:
			break;
		case VAR:
			break;
		case NUMBER:
			free((char *)(s1->u.pnum));
			break;
		case CSTRING:
			free(s1->u.cstr);
			break;
		case PROCEDURE:
		case FUNCTION:
			if (s1->u.u_proc != (Proc *)0) {
			        if (s1->u.u_proc->defn.in != STOP)
			                free((char *) s1->u.u_proc->defn.in);
			        free_list(&(s1->u.u_proc->list));
				free((char *) s1->u.u_proc);
			}
			break;
		case AUTO:
		case AUTOOBJ:
			break;
		case TEMPLATE:
hoc_free_allobjects(s1->u.template, hoc_top_level_symlist, hoc_top_level_data);
			free_list(&(s1->u.template->symtable));
			{hoc_List* l = s1->u.template->olist;
				if (l->next == l) {
					hoc_l_freelist(&s1->u.template->olist);
					free(s1->u.template);
				}else{
hoc_warning("didn't free all objects created with the old template:", s1->name);
				}
			}
			break;
		case OBJECTVAR:
#if 0 /* should have been freed above, otherwise I don't know the exact objects*/
			if (s1->arayinfo) {int i, j, k=0;
			   for (i = 0; i < s1->arayinfo->nsub; i++) {
			      for (j=0; j < s1->arayinfo->sub[i]; j++) {
			         hoc_dec_refcount(OPOBJ(s1) + k);
				 ++k;
			      }
			   }
			}else{
				hoc_dec_refcount(OPOBJ(s1));
			}
			free((char *)OPOBJ(s1));
#endif
			break;
		case OBJECTALIAS:
			hoc_obj_unref(s1->u.object_);
			break;
		case VARALIAS:
			break;
		default:
Fprintf(stderr, "In free_symspace may not free all of %s of type=%d\n", s1->name, s1->type);
		}
		if (s1->arayinfo != (Arrayinfo *)0) {
			free_arrayinfo(s1->arayinfo);
			s1->arayinfo = (Arrayinfo *)0;
		}
	}
	if (s1->extra) {
		if (s1->extra->parmlimits) {
			free((char*)s1->extra->parmlimits);
		}
		if (s1->extra->units) {
			free(s1->extra->units);
		}
		free(s1->extra);
		s1->extra = (HocSymExtension*)0;
	}
	s1->type = UNDEF;
}

sym_extra_alloc(sym) Symbol* sym; {
	if (!sym->extra) {
		sym->extra = (HocSymExtension*)ecalloc(1, sizeof(HocSymExtension));
	}
}

free_list(list)	/* free the space in a symbol table */
	Symlist **list;
{
	Symbol *s1, *s2;

	if (*list) {
		for (s1 = (*list)->first; s1; s1 = s2){
			s2 = s1->next;
			hoc_free_symspace(s1);
			if (s1->name) {
				free(s1->name);
			}
			free((char *) s1);
		}
		free((char *)(*list));
	}
	*list = (Symlist *)0;
}

hoc_free_val(p) double* p;
{
	notify_freed((void*)p);
	free((char*)p);
}

hoc_free_val_array(p, size) double* p; int size;
{
	notify_freed_val_array(p, size);
	free((char*)p);
}

hoc_free_object(p) Object* p;
{
	if (p) {
		notify_pointer_freed((void*)p);
		free((char*)p);
	}
}

hoc_free_string(p) char* p;
{
	free(p);
}

hoc_free_pstring(p) char** p;
{
	notify_freed((void*)p);
	if (*p) {
		free(*p);
		free(p);
	}
}

