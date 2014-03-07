#include <stdlib.h>
#include "src/simcore/nrnconf.h"
#include "src/simcore/nrnoc/multicore.h"

int stoprun;
int nrn_nthread;
NrnThread* nrn_threads;
int v_structure_change;
int diam_changed;

char* pnt_name(Point_process* pnt) {
  return memb_func[pnt->type].sym;
}

void nrn_exit(int err) {
  exit(err);
}

void hoc_execerror(const char* s1, const char* s2) {
  printf("error: %s %s\n", s1, s2?s2:"");
  abort();
}

void hoc_warning(const char* s1, const char* s2) {
  printf("warning: %s %s\n", s1, s2?s2:"");
}

double* makevector(size_t size) {
	return (double*)ecalloc(size, sizeof(char));
}

void* emalloc(size_t size) {
  void* memptr;
  memptr = malloc(size);
  assert(memptr);
  return memptr;
}

/* some user mod files may use this in VERBATIM */
void* hoc_Emalloc(size_t size) { return emalloc(size);}
void hoc_malchk(void) {}

void* ecalloc(size_t n, size_t size) {
  void* p;
  if (n == 0) { return (void*)0; }
  p = calloc(n, size);
  assert(p);
  return p;
}

void* erealloc(void* ptr, size_t size) {
  void* p;
  if (!ptr) {
    return emalloc(size);
  }
  p = realloc(ptr, size);
  assert(p);
  return p;
}

void* nrn_cacheline_alloc(void** memptr, size_t size) {
#if HAVE_MEMALIGN
  if (posix_memalign(memptr, 64, size) != 0 {
    fprintf(stderr, "posix_memalign not working\n");
    assert(0);
#else
    *memptr = emalloc(size);
#endif
  return *memptr;
}

void* nrn_cacheline_calloc(void** memptr, size_t nmemb, size_t size) {
#if HAVE_MEMALIGN
  nrn_cacheline_alloc(memptr, nmemb*size);
  memset(*memptr, 0, nmemb*size);
#else
  *memptr = ecalloc(nmemb, size);
#endif
  return *memptr;
}

/* 0 means no model, 1 means ODE, 2 means DAE */
int nrn_modeltype() {
	int type;
	type = 1;
	return type;
}
