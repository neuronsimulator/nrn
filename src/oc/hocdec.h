
#ifndef hocdec_h
#define hocdec_h
#define	INCLUDEHOCH	1
#define OOP 1


#include    <stdio.h>
#include	"nrnapi.h"
#include	"hocassrt.h" /* hoc_execerror instead of abort */
#include "nrnassrt.h" /* assert in case of side effects (eg. scanf) */
#include	<string.h>

#define gargstr hoc_gargstr
#define getarg hoc_getarg

/* the dec alpha cxx doesn't understand struct foo* inside a struct */

#if defined(__cplusplus)
#define HocStruct /**/
#define HocTypedef /**/
#define HocUnion /**/
union Inst;
struct Symbol;
struct Arrayinfo;
struct Proc;
struct Symlist;
union Datum;
struct cTemplate;
union Objectdata;
struct Object;
struct hoc_Item;
#else
#define HocStruct struct
#define HocUnion union
#define HocTypedef typedef
#endif

typedef int	(*Pfri)(void);
typedef void	(*Pfrv)(void);
typedef double	(*Pfrd)(void);
typedef struct Object** (*Pfro)(void);
typedef const char** (*Pfrs)(void);

typedef int	(*Pfri_vp)(void*);
typedef void	(*Pfrv_vp)(void*);
typedef double	(*Pfrd_vp)(void*);
typedef struct Object** (*Pfro_vp)(void*);
typedef const char** (*Pfrs_vp)(void*);

typedef union Inst { /* machine instruction list type */
	Pfrv	pf;
	Pfrd	pfd;
	Pfro	pfo;
	Pfrs	pfs;
	Pfrv_vp	pfv_vp;
	Pfrd_vp	pfd_vp;
	Pfro_vp	pfo_vp;
	Pfrs_vp	pfs_vp;
	HocUnion Inst	*in;
	HocStruct Symbol	*sym;
	void*	ptr;
	int	i;
} Inst;

#define STOP	(Inst *)0

typedef struct Arrayinfo  { /* subscript info for arrays */
	unsigned *a_varn;	/* dependent variable number for array elms */
	int	nsub;		/* number of subscripts */
	int	refcount;	/* because one object always uses symbol's */
	int	sub[1];		/* subscript range */
} Arrayinfo;

typedef struct Proc {
	Inst	defn;	/* FUNCTION, PROCEDURE, FUN_BLTIN */
	unsigned long size;	/* length of instruction list */
	HocStruct Symlist	*list;	/* For constants and strings */
				/* not used by FUN_BLTIN */
	int	nauto;		/* total # local variables */
	int	nobjauto;	/* the last of these are pointers to objects */
} Proc;

typedef struct Symlist {
	HocStruct Symbol *first;
	HocStruct Symbol *last;
}Symlist;

typedef char	*Upoint;

# define	NOTUSER		0
# define	USERINT		1	/* For subtype */
# define	USERDOUBLE	2
#define		USERPROPERTY	3	/* for newcable non-range variables */
#define		USERFLOAT	4	/* John Miller's NEMO uses floats */
#if NEMO
#define		NEMONODE	5	/* looks syntactically like vector */
#define		NEMOAREA	6	/* looks like vector */
#endif
#define		SYMBOL		7	/* for stack type */
#define		OBJECTTMP	8	/* temporary object on stack */
#define		STKOBJ_UNREF	9	/* already unreffed temporary object on stack */
#define		DYNAMICUNITS	10	/* {modern, legacy} units pair */
#define		CPLUSOBJECT	16	/* c++ registered class */
#define		JAVAOBJECT	32	/* c++ registered class */
/* above two are bits, next must start at 64 */
#define OBJECTALIAS 1
#define VARALIAS 2

typedef struct HocSymExtension {
	float *parmlimits;	/* some variables have suggested bounds */
	char* units;
	float tolerance;	/* some states have cvode absolute tolerance */
}HocSymExtension;

typedef struct Symbol {	/* symbol table entry */
	char	*name;
	short	type;
	short	subtype;	/* Flag for user integers */
/**
 * Note: `public` is a reserved keyword. Keeping following __cplusplus comments a bit longer
 * for future reference,  with upcoming work for NMODL + eventual mod files to support cpp.
 */
//#if defined(__cplusplus)
	short	cpublic;		/* flag set public variable */
//#else
//	short	public;		/* flag set public variable */
//#endif
	short	defined_on_the_fly;/* moved here because otherwize gcc and borland do not align the same way */
	union {
		int	oboff;	/* offset into object data pointer space */
#if 0 /* these are now found via oboff. */
		char	*str;		/* STRING */
		HocStruct Object **objvar; /* possibly an array of object variables */
#endif
		double	*pval;		/* User defined doubles - also for alias to scalar */
		HocStruct Object* object_;	/* alias to an object */
		char	*cstr;		/* constant string */
		double	*pnum;		/* Numbers */
		int	*pvalint;	/* User defined integers */
		float	*pvalfloat;	/* User defined floats */
		int	u_auto;		/* stack offset # for AUTO variable */
		double	(*ptr)(double);	/* if BLTIN */ //TODO: double as parameter?
		Proc	*u_proc;
		struct {
			short type;	/* Membrane type to find Prop */
			int index;	/* prop->param[index] */
		}rng;
		HocStruct Symbol **ppsym;	/* Pointer to symbol pointer array */
//#if defined(__cplusplus)
		HocStruct cTemplate *ctemplate;
//#else
//		HocStruct cTemplate *template;
//#endif
		HocStruct Symbol* sym;	/* for external */
	} u;
	unsigned   s_varn;	/* dependent variable number - 0 means indep */
	Arrayinfo *arayinfo;	/* ARRAY information if null then scalar */
	HocSymExtension* extra; /* additions to symbol allow compatibility
					with old nmodl dll's */
	HocStruct Symbol	*next;	/* to link to another */
} Symbol;
#define	ISARRAY(arg)	(arg->arayinfo != (Arrayinfo *)0)


#ifndef hoc_list_h
#if defined(__cplusplus)
#define hoc_List struct hoc_Item
#else
typedef struct hoc_Item	hoc_List;
#define List hoc_List
#define Item hoc_Item
#endif
#endif

typedef union Datum {	/* interpreter stack type */
	double	val;
	Symbol	*sym;
	int i;
	double	*pval;	/* first used with Eion in NEURON */
	HocStruct Object **pobj;
	HocStruct Object *obj;	/* sections keep this to construct a name */
	char	**pstr;
	HocStruct hoc_Item* itm;
	hoc_List* lst;
	void* _pvoid;	/* not used on stack, see nrnoc/point.cpp */
} Datum;

#if OOP
//#if defined(__cplusplus)
typedef struct cTemplate {
//#else
//typedef struct Template {
//#endif
	Symbol *sym;
	Symlist *symtable;
	int dataspace_size;
	int is_point_; /* actually the pointtype > 0 if a point process */
	Symbol *init;	/* null if there is no initialization function */
	Symbol *unref;  /* null if there is no function to call when refcount is decremented */
	int index;	/* next  unique integer used for name for section */
	int count;	/* how many of this kind of object */
	hoc_List* olist;	/* list of all instances */
	int id;
	void* observers;	/* hook to c++ ClassObservable */
	void* (*constructor)(struct Object*);
	void (*destructor)(void*);
	void (*steer)(void*);	/* normally nil */
	int (*checkpoint)(void**);
//#if defined(__cplusplus)
} cTemplate;
//#else
//}
//#endif

typedef union Objectdata{
	double *pval;	/* pointer to array of doubles, usually just 1 */
	char **ppstr;	/* pointer to pointer to string ,allows vectors someday*/
	HocStruct Object **pobj;	/* pointer to array of object pointers, usually just 1*/
	HocStruct hoc_Item** psecitm;   /* array of pointers to section items, usually just 1 */
	hoc_List** plist;	/* array of pointers to linked lists */
	Arrayinfo* arayinfo;
	void* _pvoid;		/* Point_process */
}Objectdata;

typedef struct Object {
	int refcount;		/* how many object variables point to this */
	int index;		/* unique integer used for names of sections */
	union {
	Objectdata *dataspace;	/* Points to beginning of object's data */
	void* this_pointer;	/* the c++ object */
	}u;
#if defined(__cplusplus)
	cTemplate *ctemplate;
#else
	cTemplate *template;
#endif
	void* aliases;	/* more convenient names for e.g. Vector or List elements dynamically created by this object*/
	HocStruct hoc_Item* itm_me;/* this object in the template list */
	HocStruct hoc_Item* secelm_; /* last of a set of contiguous section_list items used by forall */
	void* observers;	/* hook to c++ ObjObservable */
	short recurse;		/* to stop infinite recursions */
	short unref_recurse_cnt; /* free only after last return from unref callback */
} Object;
#endif

typedef struct {		/* User Functions */
	const char 	*name;
	void	(*func)(void);
} VoidFunc;

typedef struct {		/* User Double Scalars */
	const char 	*name;
	double	*pdoub;
} DoubScal;

typedef struct {		/* User Vectors */
	const char 	*name;
	double	*pdoub;
	int	index1;
} DoubVec;

typedef struct {		/* recommended limits for symbol values */
	const char	*name;
	float	bnd[2];
} HocParmLimits;

typedef struct {		/* recommended tolerance for CVODE */
	const char	*name;
	float tolerance;
} HocStateTolerance;

typedef struct {		/* units for symbol values */
	const char	*name;
	char	*units;
} HocParmUnits;

#include "oc_ansi.h"

#if defined(__cplusplus)
extern "C" {
#endif

extern void* emalloc(size_t n);
extern void* ecalloc(size_t n, size_t size);
extern void* erealloc(void* ptr, size_t n);

#if defined(__cplusplus)
}
#endif


extern	Inst *hoc_progp, *hoc_progbase, *hoc_prog, *hoc_prog_parse_recover;
extern Inst *hoc_pc;

extern Objectdata *hoc_objectdata;
extern Objectdata *hoc_top_level_data;
extern Object* hoc_thisobject;
extern Symlist* hoc_symlist;
extern Symlist* hoc_top_level_symlist;
extern Symlist* hoc_built_in_symlist;
extern Objectdata *hoc_objectdata_save(void);
extern Objectdata* hoc_objectdata_restore(Objectdata*);
#define OPVAL(sym) hoc_objectdata[sym->u.oboff].pval
#define OPSTR(sym) hoc_objectdata[sym->u.oboff].ppstr
#define OPOBJ(sym) hoc_objectdata[sym->u.oboff].pobj
#define OPSECITM(sym) hoc_objectdata[sym->u.oboff].psecitm
#define OPLIST(sym) hoc_objectdata[sym->u.oboff].plist
#define OPARINFO(sym) hoc_objectdata[sym->u.oboff + 1].arayinfo

#if LINT
#undef assert
#define assert(arg)	{if (arg) ;}	/* so fprintf doesn't give lint */
#undef IGNORE
#define	IGNORE(arg)	{if(arg);}
#define LINTUSE(arg)	{if(arg);}
char	*cplint;
int	ilint;
#define Strcat          cplint = strcat
#define Strncat         cplint = strncat
#define Strcpy          cplint = strcpy
#define Strncpy         cplint = strncpy
#define Sprintf         cplint = sprintf
#define Printf		ilint = printf
#else
#if defined(__TURBOC__)
#undef IGNORE
#define IGNORE
#else
#undef IGNORE
#define IGNORE(arg)	arg
#endif
#define LINTUSE(arg)
#define Strcat          strcat
#define Strncat         strncat
#define Strcpy          strcpy
#define Strncpy         strncpy
#define Sprintf         sprintf
#define Printf		nrnpy_pr
#endif

/* EINTR handling for LINDA */
#if LINDA
#include <errno.h>
#define ERRCHK(c1) {errno=EINTR;while(errno==EINTR){errno=0;c1}}

#else
#define ERRCHK(c1) c1
#endif

#define IFGUI if (hoc_usegui){
#define ENDGUI }

extern int hoc_usegui; /* when 0 does not make interviews calls */
extern int nrn_istty_;
extern int parallel_sub; /* for use with parallel neuron (see parallel.cl) */

#define NOT_PARALLEL_SUB(c1)  {if (!parallel_sub) c1}

/* Enter handling for PVM  NJP 11/21/94 */
#ifdef PVM
extern int init_parallel( );
int num_procs;
int *tids;
int node_num;
int mytid;
#endif


#endif
