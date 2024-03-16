#include <../../nrnconf.h>
/* /local/src/master/nrn/src/oc/symbol.cpp,v 1.9 1999/02/25 18:01:58 hines Exp */
/* version 7.2.1 2-jan-89 */

#include "hoc.h"
#include "hocdec.h"
#include "hoclist.h"
#include "nrncore_write/utils/nrncore_utils.h"
#include "oc_ansi.h"
#include "ocnotify.h"
#include "parse.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if HAVE_MALLOC_H
#include <malloc.h>
#endif

#include "nrnmpiuse.h"

#if defined(__APPLE__) && defined(__MACH__)
#include <mach/mach.h>
#endif

Symlist* hoc_built_in_symlist = nullptr;  /* keywords, built-in functions,
     all name linked into hoc. Look in this list last */
Symlist* hoc_top_level_symlist = nullptr; /* all user names seen at top-level
        (non-public names inside templates do not appear here) */
extern Objectdata* hoc_top_level_data;

Symlist* symlist = nullptr;   /* the current user symbol table: linked list */
Symlist* p_symlist = nullptr; /* current proc, func, or temp table */
                              /* containing constants, strings, and auto */
                              /* variables. Discarding these lists at */
                              /* appropriate times prevents storage leakage. */

void print_symlist(const char* s, Symlist* tab) {
    Printf("%s\n", s);
    if (tab)
        for (Symbol* sp = tab->first; sp != nullptr; sp = sp->next) {
            Printf("%s %p\n", sp->name, sp);
        }
}

Symbol* hoc_table_lookup(const char* s, Symlist* tab) /* find s in specific table */
{
    if (tab)
        for (Symbol* sp = tab->first; sp != nullptr; sp = sp->next) {
            if (strcmp(sp->name, s) == 0) {
                return sp;
            }
        }
    return nullptr;
}

Symbol* lookup(const char* s) /* find s in symbol table */
                              /* look in p_symlist then built_in_symlist then symlist */
{
    Symbol* sp;

    if ((sp = hoc_table_lookup(s, p_symlist)) != nullptr) {
        return sp;
    }
    if ((sp = hoc_table_lookup(s, symlist)) != nullptr) {
        return sp;
    }
    if ((sp = hoc_table_lookup(s, hoc_built_in_symlist)) != nullptr) {
        return sp;
    }

    return nullptr; /* nullptr ==> not found */
}

Symbol* install(/* install s in the list symbol table */
                const char* s,
                int t,
                double d,
                Symlist** list) {
    Symbol* sp = (Symbol*) emalloc(sizeof(Symbol));
    sp->name = (char*) emalloc((unsigned) (strlen(s) + 1)); /* +1 for '\0' */
    Strcpy(sp->name, s);
    sp->type = t;
    sp->subtype = NOTUSER;
    sp->defined_on_the_fly = 0;
    sp->cpublic = 0;
    sp->s_varn = 0;
    sp->arayinfo = nullptr;
    sp->extra = nullptr;
    if (!(*list)) {
        *list = (Symlist*) emalloc(sizeof(Symlist));
        (*list)->first = (*list)->last = nullptr;
    }
    hoc_link_symbol(sp, *list);
    switch (t) {
    case NUMBER:
        sp->u.pnum = (double*) emalloc(sizeof(double));
        *sp->u.pnum = d;
        break;
    case VAR:
        hoc_install_object_data_index(sp);
        OPVAL(sp) = (double*) emalloc(sizeof(double));
        *(OPVAL(sp)) = d;
        break;
    case PROCEDURE:
    case FUNCTION:
    case FUN_BLTIN:
    case OBFUNCTION:
    case STRFUNCTION:
        sp->u.u_proc = (Proc*) ecalloc(1, sizeof(Proc));
        sp->u.u_proc->list = nullptr;
        sp->u.u_proc->size = 0;
        break;
    default:
        sp->u.pnum = nullptr;
        break;
    }
    return sp;
}

Symbol* hoc_install_var(const char* name, double* pval) {
    Symbol* s = hoc_install(name, UNDEF, 0., &symlist);
    s->type = VAR;
    s->u.pval = pval;
    s->subtype = USERDOUBLE;
    return s;
}

void hoc_unlink_symbol(Symbol* s, Symlist* list) {
    assert(list);

    if (list->first == s) {
        list->first = s->next;
        if (list->last == s) {
            list->last = nullptr;
        }
    } else {
        Symbol* sp;
        for (sp = list->first; sp != nullptr; sp = sp->next) {
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
    s->next = nullptr;
}

void hoc_link_symbol(Symbol* sp, Symlist* list) {
    /* put at end of list */
    if (list->last) {
        list->last->next = sp;
    } else {
        list->first = sp;
    }
    list->last = sp;
    sp->next = nullptr;
}

void hoc_free_symspace(Symbol* s1) { /* frees symbol space. Marks it UNDEF */
    if (s1 && s1->cpublic != 2) {
        switch (s1->type) {
        case UNDEF:
        case STRING:
        case VAR:
            break;
        case NUMBER:
            free((char*) (s1->u.pnum));
            break;
        case CSTRING:
            free(s1->u.cstr);
            break;
        case PROCEDURE:
        case FUNCTION:
            if (s1->u.u_proc != nullptr) {
                if (s1->u.u_proc->defn.in != STOP)
                    free((char*) s1->u.u_proc->defn.in);
                free_list(&(s1->u.u_proc->list));
                free((char*) s1->u.u_proc);
            }
            break;
        case AUTO:
        case AUTOOBJ:
            break;
        case TEMPLATE:
            hoc_free_allobjects(s1->u.ctemplate, hoc_top_level_symlist, hoc_top_level_data);
            free_list(&(s1->u.ctemplate->symtable));
            {
                hoc_List* l = s1->u.ctemplate->olist;
                if (l->next == l) {
                    hoc_l_freelist(&s1->u.ctemplate->olist);
                    free(s1->u.ctemplate);
                } else {
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
            Fprintf(stderr,
                    "In free_symspace may not free all of %s of type=%d\n",
                    s1->name,
                    s1->type);
        }
        if (s1->arayinfo != nullptr) {
            free_arrayinfo(s1->arayinfo);
            s1->arayinfo = nullptr;
        }
    }
    if (s1->extra) {
        if (s1->extra->parmlimits) {
            free(s1->extra->parmlimits);
        }
        if (s1->extra->units) {
            free(s1->extra->units);
        }
        free(s1->extra);
        s1->extra = nullptr;
    }
    s1->type = UNDEF;
}

void sym_extra_alloc(Symbol* sym) {
    if (!sym->extra) {
        sym->extra = (HocSymExtension*) ecalloc(1, sizeof(HocSymExtension));
    }
}

void free_list(Symlist** list) { /* free the space in a symbol table */
    if (!*list) {
        return;
    }

    Symbol* s1 = (*list)->first;
    while (s1) {
        Symbol* s2 = s1->next;
        hoc_free_symspace(s1);
        if (s1->name) {
            free(s1->name);
        }
        free((char*) s1);
        s1 = s2;
    }
    free((char*) (*list));
    *list = nullptr;
}

void hoc_free_val(double* p) {
    notify_freed(p);
    free(p);
}

void hoc_free_val_array(double* p, size_t size) {
    notify_freed_val_array(p, size);
    free(p);
}

void hoc_free_object(Object* p) {
    if (p) {
        notify_pointer_freed(p);
        free(p);
    }
}

void hoc_free_string(char* p) {
    free(p);
}

void hoc_free_pstring(char** p) {
    notify_freed((void*) p);
    if (*p) {
        free(*p);
        free(p);
    }
}

size_t nrn_mallinfo(int item) {
#if defined(__APPLE__) && defined(__MACH__)
    /* OSX ------------------------------------------------------
     * Returns the current resident set size (physical memory use) measured
     * in bytes, or zero if the value cannot be determined on this OS.
     */
    struct mach_task_basic_info info;
    mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t) &info, &infoCount) !=
        KERN_SUCCESS)
        return (size_t) 0L; /* Can't access? */
    return (size_t) info.resident_size;
#elif HAVE_MALLINFO || HAVE_MALLINFO2
    // *nix platforms with mallinfo[2]()
    size_t r;
    // prefer mallinfo2, fall back to mallinfo
#if HAVE_MALLINFO2
    auto const m = mallinfo2();
#else
    auto const m = mallinfo();
#endif
    if (item == 1) {
        r = m.uordblks;
    } else if (item == 2) {
        r = m.hblkhd;
    } else if (item == 3) {
        r = m.arena;
    } else if (item == 4) {
        r = m.fordblks;
    } else if (item == 5) {
        r = m.hblks;
    } else if (item == 6) {
        r = m.hblkhd + m.arena;
    } else {
        r = m.hblkhd + m.uordblks;
    }
    return r;
#else
    /* UNSUPPORTED PLATFORM ------------------------------------ */
    return 0;
#endif
}

void hoc_mallinfo() {
    int const i = chkarg(1, 0, 6);
    auto const x = nrn_mallinfo(i);
    hoc_ret();
    hoc_pushx(x);
}

// TODO: would it be useful to return the handle itself to Python, for use with
// ctypes CDLL and so on?
void hoc_coreneuron_handle() {
    bool success{false};
    try {
        success = get_coreneuron_handle();
    } catch (std::runtime_error const& e) {
    }
    hoc_retpushx(success);
}
