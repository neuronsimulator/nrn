/*
Copyright (c) 2014 EPFL-BBP, All rights reserved.

THIS SOFTWARE IS PROVIDED BY THE BLUE BRAIN PROJECT "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE BLUE BRAIN PROJECT
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdlib.h>
#include "coreneuron/nrnconf.h"
#include "coreneuron/nrnoc/multicore.h"

extern void nrnmpi_finalize(void);

int stoprun;
int nrn_nthread;
NrnThread* nrn_threads;
int v_structure_change;
int diam_changed;
#define MAXERRCOUNT 5
int hoc_errno_count;

char* pnt_name(Point_process* pnt) {
  return memb_func[pnt->_type].sym;
}

void nrn_exit(int err) {
  nrnmpi_finalize();
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
  if (posix_memalign(memptr, 64, size) != 0) {
    fprintf(stderr, "posix_memalign not working\n");
    assert(0);
  }
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


/* used by nmodl and other c, c++ code */
double hoc_Exp(double x)
{
        if (x < -700.) {
                return 0.;
        }else if (x > 700) {
                errno = ERANGE;
                if (++hoc_errno_count < MAXERRCOUNT) {
                        fprintf(stderr, "exp(%g) out of range, returning exp(700)\n", x);
                }
                if (hoc_errno_count == MAXERRCOUNT) {
                        fprintf(stderr, "No more errno warnings during this execution\n");
                }
                return exp(700.);
        }
        return exp(x);
}

