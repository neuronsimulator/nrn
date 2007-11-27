#include <../../nrnconf.h>
#if defined(CYGWIN)
#define nrnCYGWIN
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <setjmp.h>
#include <math.h>
#include <io.h>
#include <assert.h>

#define __attribute__(arg) /**/
#include "coff.h"
#undef CYGWIN
#define CYGWIN 1

#if defined(__MWERKS__)|| defined(_MSC_VER)
#undef SYMESZ
#define SYMESZ 18
#undef RELSZ
#define RELSZ 10
#endif

#include "limits.h"
#include "dll.h"

#if defined(nrnCYGWIN)
extern "C" {
extern FILE __files[]; // declared in mswinprt.c. FILE* __files does not work here.
int _flsbuf(unsigned i, FILE* f) {
	printf("\n_flsbuf i=%d\n", i);
	return i;
}
}
#endif

char* dllstrdup(const char* s) {
	char* p = new char[strlen(s) + 1];
	strcpy(p, s);
	return p;
}

#if defined(__MWERKS__) && !defined(_MSC_VER)
extern "C" {
#if __MWERKS__ >= 7
FILE __files[]; // would be from file_struc.h except for RC_INVOKED
#endif

extern int flsbuf(unsigned, FILE*);
int _flsbuf(unsigned i, FILE* f) {
	printf("\n_flsbuf i=%d\n", i);
	return i;
}

}
#endif

#define PRINT 0
#define PRDEBUG printf(

//#define PRDEBUGFILE "c:/temp.tmp"
#if defined(PRDEBUGFILE)
#undef PRDEBUG
#define PRDEBUG fprintf(prdebug_,
static FILE* prdebug_;
#endif

typedef void (*CDTOR)();

struct Symtab {
  Symtab *next, *prev;
  static Symtab *symtabs;
  char **name;
  void **value;
  int num;
  int max;
  Symtab();
  ~Symtab();
  void add(char *name, void *value);
  void *get(char *name);
  static void *lookup(char *name);
};

struct dll_s {
  dll_s *next, *prev;
  char *loadpath;
  int valid;
  static dll_s *top;
  int load_count;
  int num_sections;
  char *bytes;
  char *dtor_section;
  int dtor_count;
  Symtab symtab;
  CDTOR uninit_func;
};

dll_s *dll_s::top = 0;

Symtab *Symtab::symtabs = 0;

Symtab::Symtab()
{
  next = symtabs;
  prev = 0;
  symtabs = this;
  num = 0;
  max = 10;
  name = (char **)malloc(max*sizeof(char *));
  value = (void **)malloc(max*sizeof(void *));
}

Symtab::~Symtab()
{
  int i;
  for (i=0; i<num; i++)
    free(name[i]);
  free(name);
  free(value);
  if (next)
    next->prev = prev;
  if (prev)
    prev->next = next;
  else
    symtabs = next;
}

void Symtab::add(char *Pname, void *Pvalue)
{
  if (num >= max)
  {
    max += 10;
    name = (char **)realloc(name, max * sizeof(char *));
    value = (void **)realloc(value, max * sizeof(void *));
  }
  name[num] = dllstrdup(Pname);
  value[num] = Pvalue;
  num++;
}

void *Symtab::get(char *Pname)
{
  int i;
  for (i=0; i<num; i++)
    if (strcmp(Pname, name[i]) == 0)
		return value[i];
  return 0;
}

void *Symtab::lookup(char *Pname)
{
  Symtab *s;
  for (s=symtabs; s; s=s->next)
  {
    void *v = s->get(Pname);
    if (v)
    {
      return v;
    }
  }
  return 0;
}

static struct {
  int val;
  char *name;
} flags[] = {
  F_RELFLG, "REL",
  F_EXEC, "EXEC",
  F_LNNO, "LNNO",
  F_LSYMS, "LSYMS",
  0, 0
};

static struct {
  int val;
  char *name;
} sflags[] = {
  STYP_TEXT, "text",
  STYP_DATA, "data",
  STYP_BSS, "bss",
  0, 0
};

static char *dll_argv0 = 0;
static Symtab *local_symtab = 0;
static Symtab *common_symtab = 0;

static void dll_exitfunc(void)
{
  while (dll_s::top)
    dll_unload((struct DLL *)dll_s::top);
}

void dll_register(char *Psymbol, void *Paddress)
{
  if (local_symtab == 0)
    local_symtab = new Symtab;
  if (common_symtab == 0)
    common_symtab = new Symtab;
  local_symtab->add(Psymbol, Paddress);
  local_symtab->get(Psymbol);
}

// following works around the c++ overloading of math functions
// eg. want sin(double) not sin(float)
static void dll_registerdf1(char* Psymbol, double(*Paddress)(double)) {
	dll_register(Psymbol, (void*)Paddress);
}

static void dll_registerdf2(char* Psymbol, double(*Paddress)(double, double)) {
	dll_register(Psymbol, (void*)Paddress);
}

#define MKDLL(a,b) extern void b();
#define DFMKDLL(a,b) extern double b();
#define DFMKDLL1(a,b) /**/
#define DFMKDLL2(a,b) /**/
#define DMKDLL(a,b) extern double b;
#define IMKDLL(a,b) extern int b;
#define MKDLLdec(a,b) /**/
#define MKDLLvpf(a,b) extern void* b();
#define MKDLLvp(a,b) extern void* b;
#define MKDLLif(a,b) extern int b();
extern "C" {
#include "nrnmath.h"
#include "nrnmech.h"
#if OCMATRIX
#include "ocmatdll.h"
#endif
}
#undef MKDLL
#undef DFMKDLL
#undef DFMKDLL1
#undef DFMKDLL2
#undef DMKDLL
#undef IMKDLL
#undef MKDLLdec
#undef MKDLLvpf
#undef MKDLLvp
#undef MKDLLif

void dll_init(char *argv0)
{
	// this was causing an unhandled exception on
	// exit on one machine. I don't believe, but
	// am not certain, that it is any longer necessary
  //atexit(dll_exitfunc);
  if (dll_argv0)
    free(dll_argv0);
  else
  {
    dll_register("_dll_load", (void*)dll_load);
    dll_register("_dll_unload", (void*)dll_unload);
    dll_register("_dll_lookup", (void*)dll_lookup);
#define MKDLL(a,b) dll_register(a, (void*)b);
#define DFMKDLL(a,b) dll_register(a, (void*)b);
#define DFMKDLL1(a,b) dll_registerdf1(a, b);
#define DFMKDLL2(a,b) dll_registerdf2(a, b);
#define DMKDLL(a,b) dll_register(a, (void*)(&b));
#define IMKDLL(a,b) DMKDLL(a,b)
#define MKDLLdec(a,b) MKDLL(a, b)
#define MKDLLvpf(a,b) MKDLL(a,b)
#define MKDLLif(a,b) MKDLL(a,b)
#define MKDLLvp(a,b) DMKDLL(a,b)

#include "nrnmath.h"
#include "nrnmech.h"
#if OCMATRIX
#include "ocmatdll.h"
#endif
#undef MKDLL
#undef DFMKDLL
#undef DMKDLL
#undef IMKDLL
#undef MKDLLdec
#undef MKDLLvpf
#undef MKDLLif
  }
  dll_argv0 = dllstrdup(argv0);
}

char *find_file(char *fn)
{
#if 0
  char *bp, *ep, *pp;
  static char buf[PATH_MAX];
  if (strpbrk(fn, ":\\/"))
    return fn;

//  printf("find: try `%s'\n", fn);
  if (access(fn,0) == 0)
    return fn;

  if (dll_argv0)
  {
    strcpy(buf, dll_argv0);
    ep = buf;
    for (bp=buf; *bp; bp++)
      if (strchr(":\\/", *bp))
        ep = bp+1;
    strcpy(ep, fn);
//    printf("find: try `%s'\n", buf);
    if (access(buf, 0) == 0)
      return buf;
  }
  
  bp = getenv("PATH");
  while (*bp)
  {
    pp = buf;
    while (*bp && *bp != ';')
      *pp++ = *bp++;
    *pp++ = '/';
    strcpy(pp, fn);
//    printf("find: try `%s'\n", buf);
    if (access(buf, 0) == 0)
      return buf;
    if (*bp == 0)
      break;
    bp++;
  }
//  printf("find: default `%s'\n", fn);
#endif
  return fn;
}

struct DLL *dll_load(char *filename)
{
#if defined(PRDEBUGFILE)
prdebug_ = fopen(PRDEBUGFILE, "wb");
#endif
  if (dll_argv0 == 0)
    dll_init("");

  dll_s *dll;
  for (dll=dll_s::top; dll; dll=dll->next)
    if (strcmp(dll->loadpath, filename) == 0)
    {
      dll->load_count ++;
      return (DLL *)dll;
    }
  DLL* d = dll_force_load(filename);
#if defined(PRDEBUGFILE)
fclose(prdebug_);
#endif
  return d;
}

struct DLL *dll_force_load(char *filename)
{
  int i, s;
  int error = 0;
  int max_bytes = 0;
  dll_s *dll;

  char *loadpath = find_file(filename);
 
#if PRINT
  PRDEBUG "load: `%s'\n", loadpath);
#endif
  FILE *file = fopen(loadpath, "rb");
  if (file == 0)
  {
	 printf("Error: unable to load %s\n", filename);
    perror("The error was");
    return 0;
  }

  dll = new dll_s;
  dll->valid = 0;
  dll->loadpath = dllstrdup(filename);
  dll->num_sections = 0;
  dll->next = dll_s::top;
  if (dll->next)
    dll->next->prev = dll;
  dll_s::top = dll;
  dll->prev = 0;
  dll->load_count = 1;
  dll->dtor_count = 0;
  dll->uninit_func = 0;
  CDTOR init_func = 0;
  
  FILHDR filhdr;
  fread(&filhdr, 1, FILHSZ, file);
#if PRINT
  PRDEBUG "file: %s, magic=%#x\n", filename, filhdr.f_magic);
#endif
  if (filhdr.f_magic != 0x14c)
  {
    fprintf(stderr, "Not a COFF file\n");
    return 0;
  }
#if PRINT
  PRDEBUG "nscns=%d, nsyms=%d, symptr=%#x\n", filhdr.f_nscns, filhdr.f_nsyms,
    filhdr.f_symptr);
  PRDEBUG "flags: ");
  for (i=0; flags[i].val; i++)
    if (filhdr.f_flags & flags[i].val)
      PRDEBUG " %s", flags[i].name);
  PRDEBUG "\n");
#endif

  if (filhdr.f_opthdr)
    fseek(file, filhdr.f_opthdr, 1);

  SCNHDR *section;
  section = new SCNHDR[filhdr.f_nscns];
  dll->num_sections = filhdr.f_nscns;
#if defined(__MWERKS__) || defined(nrnCYGWIN)
  for (i=0; i < filhdr.f_nscns; ++i) {
  	fread(section+i, 1, SCNHSZ, file);
#if PRINT == 2  
  PRDEBUG "%8s paddr %x vaddr %x size %x scnptr %x relptr %x lnnoptr %x nreloc %x nlnno %x flags %x\n",
  section[i].s_name, section[i].s_paddr, section[i].s_vaddr, section[i].s_size,
  section[i].s_scnptr, section[i].s_relptr, section[i].s_lnnoptr,
  section[i].s_nreloc, section[i].s_nlnno, section[i].s_flags);
#endif
  }
#else
  fread(section, sizeof(SCNHDR), filhdr.f_nscns, file);
#endif
  max_bytes = 0;
    for (s=0; s<filhdr.f_nscns; s++) {
#if CYGWIN
      if (max_bytes < section[s].s_vaddr + section[s].s_size + section[s].s_paddr)
        max_bytes = section[s].s_vaddr + section[s].s_size + section[s].s_paddr;
#else
      if (max_bytes < section[s].s_vaddr + section[s].s_size)
        max_bytes = section[s].s_vaddr + section[s].s_size;
#endif
    }

  dll->bytes = new char[max_bytes];
  // there was a problem with the hoc_scdoub and hoc_vdoub terminator 0's not
  // being set to 0 in all cases. Perhaps cygwin has changed its section zeroing
  // policy and s_size is back in favor at s_paddr expense. Anyway, the
  // following at least starts us out with a zero array and the segmentation
  // violation seen occasionally with NEURONMainMenu/File/WorkingDir
  // no longer occurs.  See the next #if CYGWIN for my earlier hack which
  // may be obsolete.
  for (i=0; i < max_bytes; ++i) {dll->bytes[i] = 0;}

  for (s=0; s<filhdr.f_nscns; s++)
  {
    if (section[s].s_scnptr)
    {
#if PRINT
      PRDEBUG "section %d from file at 0x%x size 0x%x\n",
      	s, dll->bytes + section[s].s_vaddr, section[s].s_size);
#endif
      if (section[s].s_size)
      {
        fseek(file, section[s].s_scnptr, 0);
        fread(dll->bytes+section[s].s_vaddr, 1, section[s].s_size, file);
      }
    }
    else
    {
#if CYGWIN
#if PRINT
      PRDEBUG "section %d zeroed 0x%x bytes at 0x%x\n", s, section[s].s_paddr, dll->bytes+section[s].s_vaddr);
      PRDEBUG "if not CYGWIN then we would\n");
      PRDEBUG "section %d zeroed %d bytes at 0x%x\n", s, section[s].s_size, dll->bytes+section[s].s_vaddr);
#endif
    if (section[s].s_paddr)
        memset(dll->bytes+section[s].s_vaddr, 0, section[s].s_paddr);
    }
#else
#if PRINT
      PRDEBUG "section %d zeroed %d bytes at 0x%x\n", s, section[s].s_size, dll->bytes+section[s].s_vaddr);
#endif
    if (section[s].s_size)
        memset(dll->bytes+section[s].s_vaddr, 0, section[s].s_size);
    }
#endif
  }
  
  SYMENT *syment = new SYMENT[filhdr.f_nsyms];
  unsigned long *symaddr = new unsigned long [filhdr.f_nsyms];
  fseek(file, filhdr.f_symptr, 0);
  //printf("SYMESZ=%d\n", SYMESZ);
#if defined(__MWERKS__) || defined(nrnCYGWIN)
	for (i=0; i < filhdr.f_nsyms; ++i) {
		fread(syment+i, 1, SYMESZ, file);
	}
#else
  fread(syment, filhdr.f_nsyms, SYMESZ, file);
#endif
  unsigned long strsize = 4;
  fread(&strsize, 1, sizeof(unsigned long), file);
  //printf("strsize=%ld\n", strsize);
  char *strings = new char[strsize];
  strings[0] = 0;
  if (strsize > 4)
    fread(strings+4, strsize-4, 1, file);

  for (i=0; i<filhdr.f_nsyms; i++)
  {
    char snameb[9], *sname, *scname;
#if PRINT
    PRDEBUG "[0x%08x] ", syment[i].e_value);
#endif
    if (syment[i].e.e.e_zeroes)
    {
      sprintf(snameb, "%.8s", syment[i].e.e_name);
      sname = snameb;
    }
    else
      sname = strings + syment[i].e.e.e_offset;

    if (syment[i].e_scnum > 0)
    {
      symaddr[i] = syment[i].e_value + (long)(dll->bytes);
#if CYGWIN
	  symaddr[i] += section[syment[i].e_scnum-1].s_vaddr;
#endif
      if (syment[i].e_sclass == 2)
        dll->symtab.add(sname, (void *)symaddr[i]);
      if (strcmp(sname, "_dll_unloadfunc") == 0)
        dll->uninit_func = (CDTOR)symaddr[i];
      if (strcmp(sname, "_dll_loadfunc") == 0)
        init_func = (CDTOR)symaddr[i];
    }
    else if (syment[i].e_scnum == N_UNDEF)
    {
      if (syment[i].e_value)
      {
        void *stv = common_symtab->get(sname);
        if (stv)
          symaddr[i] = (long)stv;
        else
        {
          stv = calloc(syment[i].e_value,1);
          common_symtab->add(sname, stv);
          symaddr[i] = (long)stv;
        }
      }
      else
      {
        symaddr[i] = (long)Symtab::lookup(sname);
        if (symaddr[i] == 0)
        {
          fprintf(stderr, "Undefined symbol %s referenced from %s\n",
            sname, filename);
          error = 1;
        }
      }
    }

#if PRINT
    if (syment[i].e_scnum >= 1)
      scname = section[syment[i].e_scnum-1].s_name;
    else
      scname = "N/A";
    PRDEBUG "[%2d] 0x%08x %2d %-8.8s %04x %02x %d %s\n", i,
      symaddr[i],
      syment[i].e_scnum,
      scname,
      syment[i].e_type,
      syment[i].e_sclass,
      syment[i].e_numaux,
      sname);
    for (int a=0; a<syment[i].e_numaux; a++)
    {
      i++;
#if 0
      unsigned char *ap = (unsigned char *)(syment+i);
      printf("\033[0m");
      for (int b=0; b<SYMESZ; b++)
        printf(" %02x\033[32m%c\033[37m", ap[b], ap[b]>' '?ap[b]:' ');
      printf("\033[1m\n");
#endif
    }
#else
    i += syment[i].e_numaux;
#endif
  }

  for (s=0; s<filhdr.f_nscns; s++)
  {
#if PRINT
    PRDEBUG "\nS[%d] `%-8s' pa=%#x va=%#x s=%#x ptr=%#x\n",
      s, section[s].s_name, section[s].s_paddr,section[s].s_vaddr,
      section[s].s_size, section[s].s_scnptr);
    PRDEBUG "  rel=%#x nrel=%#x  flags: ",
      section[s].s_relptr, section[s].s_nreloc);
    for (i=0; sflags[i].val; i++)
      if (section[s].s_flags & sflags[i].val)
        PRDEBUG " %s", sflags[i].name);
    PRDEBUG "\n");
#endif

    if (section[s].s_nreloc)
    {
      fseek(file, section[s].s_relptr, 0);
//printf("RELSZ=%d\n", RELSZ);
      RELOC *r = new RELOC[section[s].s_nreloc];
#if defined(__MWERKS__) || defined(nrnCYGWIN)
		for (i=0; i < section[s].s_nreloc; ++i) {
			fread(r+i, 1, RELSZ, file);
		}
#else
      fread(r, RELSZ, section[s].s_nreloc, file);
#endif
      for (i=0; i<section[s].s_nreloc; i++)
      {
        long *ptr = (long *)(dll->bytes + r[i].r_vaddr);
        long old_value = *ptr;
#if PRINT
        PRDEBUG "  [%02d]  0x%08x(0x%08x)  %2d  0x%04x 0%02o (was 0x%08x",
          i, r[i].r_vaddr, ptr, r[i].r_symndx, r[i].r_type, r[i].r_type, old_value);
#endif
        switch (r[i].r_type)
        {
          case 0x06:
#if !CYGWIN
            old_value -= syment[r[i].r_symndx].e_value;
#endif
            old_value += symaddr[r[i].r_symndx];
            break;
          case 0x14:
            if (syment[r[i].r_symndx].e_scnum == 0 || 1)
            {
#if !CYGWIN // old coff format from gcc2.7.2 days
              old_value -= (long)(dll->bytes);              
#else // new pe-coff format from gcc2.95.2
              old_value = -(long)ptr - 4;
#endif
              old_value += symaddr[r[i].r_symndx];
            }
            break;
          default:
            fprintf(stderr, "Error: unexpected relocation type %#x\n",
              r[i].r_type);
            error = 1;
        }
        *ptr = old_value;
#if PRINT
        PRDEBUG ", now 0x%08x)\n", old_value);
        if (r[i].r_type == 0x14) {
        PRDEBUG "pc(0x%08x) + 4 + offset(0x%08x) = external address(0x%08x)\n", (long)ptr, (long)*ptr,
        (long)*ptr+(long)ptr+4);
        PRDEBUG "index %2d  address 0x%08x\n", r[i].r_symndx, symaddr[r[i].r_symndx]);
        }
        if (r[i].r_type == 0x06) {
        	PRDEBUG "e_value=0x%08x\n", syment[r[i].r_symndx].e_value);
        }
#endif
      }
      delete [] r;
    }
  }
  for (s=0; s<filhdr.f_nscns; s++)
  {
    if (strcmp(section[s].s_name, ".ctor") == 0)
    {
      for (i=0; i<section[s].s_size/4; i++)
      {
        CDTOR f;
        void **fv = (void **)(dll->bytes+section[s].s_vaddr);
        f = (CDTOR)(fv[i]);
        f();
      }
    }
    if (strcmp(section[s].s_name, ".dtor") == 0)
    {
      dll->dtor_section = dll->bytes + section[s].s_vaddr;
      dll->dtor_count = section[s].s_size/4;
    }
  }
  if (init_func)
    init_func();

  dll->valid = 1;
  fclose(file);
  delete [] syment;
  delete [] symaddr;
  delete [] strings;
  delete [] section;
  
  #if (PRINT == 2)
  for (i=0; i < max_bytes; ++i) {
  	if ((long)(dll->bytes + i)%8 == 0) {
  		PRDEBUG "\n%08x ", (long)(dll->bytes + i));
  	}
  	PRDEBUG " %02x", dll->bytes[i]&0xff);
  }
  PRDEBUG "\n");
  PRDEBUG "printf=%08x *printf=%02x\n", (long)printf, *((char*)printf)&0xff);  	 
  #endif

  if (error)
    return 0;
  return (struct DLL *)dll;
}

void dll_unload(struct DLL *Pdll)
{
  int i, s;
  CDTOR f;
  if (Pdll == 0)
    return;
  dll_s *dll = (dll_s *)Pdll;
  if (--dll->load_count)
    return;
//  printf("unload: `%s'\n", dll->loadpath);
  if (dll->valid)
  {
    if (dll->uninit_func)
      dll->uninit_func();
    for (i=0; i<dll->dtor_count; i++)
    {
      void **fv = (void **)(dll->dtor_section);
      f = (CDTOR)(fv[i]);
      f();
    }
  }
  if (dll->next)
    dll->next->prev = dll->prev;
  if (dll->prev)
    dll->prev->next = dll->next;
  else
    dll_s::top = dll->next;
  if (dll->bytes)
    delete dll->bytes;
  dll->valid = 0;
  delete dll;
}

void *dll_lookup(struct DLL *Pdll, char *name)
{
  dll_s *dll = (dll_s *)Pdll;
  return dll->symtab.get(name);
}
