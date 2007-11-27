/* /local/src/master/nrn/src/oc/list.h,v 1.3 1997/08/29 21:03:44 hines Exp */
/*
list.h,v
 * Revision 1.3  1997/08/29  21:03:44  hines
 * administrative modifications to prepare for distribution.
 *
 * Revision 1.2  1995/02/13  20:17:30  hines
 * small mods for portability
 *
 * Revision 1.1.1.1  1994/10/12  17:22:03  hines
 * NEURON 3.0 distribution
 *
 * Revision 2.81  1994/04/27  11:21:31  hines
 * support for ivoc/SRC/checkpoint.c
 * usage is checkpoint("filename")
 * then ivoc filename ...
 * so far only for oc
 *
 * Revision 2.43  1993/04/20  08:40:23  hines
 * lists can have void* elements (use insertvoid lappendvoid VOIDITM(q) )
 *
 * Revision 1.2  92/10/27  12:08:16  hines
 * list.c list.h moved from nrnoc to oc
 * all templates maintain a list of their objects
 * 
 * Revision 1.1  92/10/27  11:40:17  hines
 * Initial revision
 * 
 * Revision 3.9  92/09/23  13:13:07  hines
 * list.h stuff prefixed by hoc so doesn't conflict with InterViews List
 * 
 * Revision 2.83  92/08/11  12:39:17  hines
 * list.h can be included in c++ files
 * 
 * Revision 2.75  92/08/07  16:12:00  hines
 * sections as objects. nsection and section gone. now list of sections
 * in nmodl list style in its place. vectorization no longer optional
 * probably many bugs at this point but all examples run correctly.
 * Need tests with complete code coverage.
 * 
 * Revision 1.1  92/08/06  14:38:42  hines
 * Initial revision
 * 
*/

#ifndef hoc_list_h
#define hoc_list_h

#if HOC_L_LIST
#define stralloc	hoc_l_stralloc
#define newitem		hoc_l_newitem
#define newlist		hoc_l_newlist
#define freelist	hoc_l_freelist
#define next		hoc_l_next
#define prev		hoc_l_prev
#define insertstr	hoc_l_insertstr
#define insertitem	hoc_l_insertitem
#define insertlist	hoc_l_insertlist
#define insertsym	hoc_l_insertsym
#define insertsec	hoc_l_insertsec
#define insertobj	hoc_l_insertobj
#define insertvoid	hoc_l_insertvoid
#define linsertsym	hoc_l_linsertsym
#define linsertstr	hoc_l_linsertstr
#define lappendsym	hoc_l_lappendsym
#define lappendstr	hoc_l_lappendstr
#define lappenditem	hoc_l_lappenditem
#define lappendlst	hoc_l_lappendlst
#define lappendsec	hoc_l_lappendsec
#define lappendobj	hoc_l_lappendobj
#define lappendvoid	hoc_l_lappendvoid
#define delete		hoc_l_delete
#define delitems	hoc_l_delitems
#define move		hoc_l_move
#define movelist	hoc_l_movelist
#define replacstr	hoc_l_replacstr
#define Item		hoc_Item
#define List		hoc_List

typedef struct hoc_Item    hoc_List;		/* list of mixed items */
#else
#define hoc_List struct hoc_Item
#endif

typedef struct hoc_Item {
	union	{
			struct hoc_Item *itm;
			hoc_List *lst;
			char *str;
			struct Symbol *sym;
			struct Section* sec;
			struct Object* obj;
			void* vd;
	}	element;	/* pointer to the actual item */
	struct hoc_Item    *next;
	struct hoc_Item    *prev;
	short           itemtype;
}               hoc_Item;
#define ITEM0	(hoc_Item *)0
#define LIST0	(hoc_List *)0

#define ITERATE(itm,lst) for (itm = (lst)->next; itm != (lst); itm = itm->next)
/*
 * this is convenient way to get the element pointer if you know what type
 * the item is 
 */
#define SYM(q)	((q)->element.sym)
#define STR(q)	((q)->element.str)
#define ITM(q)	((q)->element.itm)
#define LST(q)	((q)->element.lst)
#define hocSEC(q)	((q)->element.sec)
#define OBJ(q)	((q)->element.obj)
#define VOIDITM(q)	((q)->element.vd)

/* types not defined in parser */
#define ITEM	2
#define LIST	3
#define VOIDPOINTER	4
/*
 * An item type, STRING is also used as an item type 
 */

#ifdef __cplusplus

extern char* hoc_l_stralloc(char*, char* release=nil);
extern hoc_List* hoc_l_newlist();
extern hoc_Item* hoc_l_insertstr(hoc_Item*, char*);
extern hoc_Item* hoc_l_insertsym(hoc_Item*, Symbol*);
extern hoc_Item* hoc_l_insertitem(hoc_Item*, hoc_Item*);
extern hoc_Item* hoc_l_insertlist(hoc_Item*, hoc_List*);
extern hoc_Item* hoc_l_insertsec(hoc_Item*, Section*);
extern hoc_Item* hoc_l_insertvoid(hoc_Item*, void*);

extern hoc_Item* hoc_l_linsertstr(hoc_List*, char*);
extern hoc_Item* hoc_l_linsertsym(hoc_List*, Symbol*);

extern hoc_Item* hoc_l_lappendstr(hoc_List*, char*);
extern hoc_Item* hoc_l_lappendsym(hoc_List*, Symbol*);
extern hoc_Item* hoc_l_lappenditem(hoc_List*, hoc_Item*);
extern hoc_Item* hoc_l_lappendlst(hoc_List*, hoc_List*);
extern hoc_Item* hoc_l_lappendsec(hoc_List*, Section*);
extern hoc_Item* hoc_l_lappendvoid(hoc_List*, void*);
extern hoc_Item* hoc_l_lappendobj(hoc_List*, Object*);

extern void hoc_l_freelist(hoc_List**);
extern hoc_Item* hoc_l_next(hoc_Item*);
extern hoc_Item* hoc_l_prev(hoc_Item*);
extern void hoc_l_delete(hoc_Item*);
extern void hoc_l_move(hoc_Item*, hoc_Item*, hoc_Item*);
extern void hoc_l_movelist(hoc_Item*, hoc_Item*, hoc_List*);
extern void hoc_l_replacstr(hoc_Item*, char*);

#else
extern char
		*hoc_l_stralloc();	/* copies string to new space */

extern hoc_List
		*hoc_l_newlist();	/* begins new empty list */
extern hoc_Item
		*hoc_l_insertstr(),	/* before a known Item */
		*hoc_l_insertsym(),
		*hoc_l_insertsec(),
		*hoc_l_insertobj(),
		*hoc_l_insertvoid(),
		*hoc_l_linsertstr(),	/* prepend to list */
		*hoc_l_lappendstr(),	/* append to list */
		*hoc_l_linsertsym(),
		*hoc_l_lappendsym(),
		*hoc_l_lappenditem(),
		*hoc_l_lappendlst(),
		*hoc_l_lappendsec(),
		*hoc_l_lappendobj(),
		*hoc_l_lappendvoid(),
		*hoc_l_next(),	/* not used but should be instead of q->next */
		*hoc_l_prev();

/* the following is to get lint to shut up */
#if LINT
#undef assert
#define assert(arg)	{if (arg) ;}	/* so fprintf doesn't give lint */
extern char    *clint;
extern int      ilint;
extern hoc_Item    *qlint;
#define Insertstr	qlint = insertstr
#define Insertsym	qlint = insertsym
#define Insertsec	qlint = insertsec
#define Linsertsym	qlint = linsertsym
#define Linsertstr	qlint = linsertstr
#define Lappendsym	qlint = lappendsym
#define Lappendstr	qlint = lappendstr
#define Lappenditem	qlint = lappenditem
#define Lappendlst	qlint = lappendlst
#define Lappendsec	qlint = lappendsec
#else
#define Insertstr	insertstr
#define Insertsym	insertsym
#define Insertsec	insertsec
#define Linsertsym	linsertsym
#define Linsertstr	linsertstr
#define Lappendsym	lappendsym
#define Lappendstr	lappendstr
#define Lappenditem	lappenditem
#define Lappendlst	lappendlst
#define Lappendsec	lappendsec
#endif
#endif
#endif
