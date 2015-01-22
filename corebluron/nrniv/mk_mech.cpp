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

#include <string.h>
#include "corebluron/nrnconf.h"
#include "corebluron/nrnoc/multicore.h"
#include "corebluron/nrnoc/nrnoc_decl.h"
#include "corebluron/nrniv/nrniv_decl.h"
#include "corebluron/nrniv/ivtable.h"
#include "corebluron/nrniv/nrn_assert.h"
#include "corebluron/utils/sdprintf.h"

int nrn_need_byteswap;
// following copied (except for nrn_need_byteswap line) from NEURON ivocvect.cpp
#define BYTEHEADER uint32_t _II__;  char *_IN__; char _OUT__[16]; int BYTESWAP_FLAG=0;
#define BYTESWAP(_X__,_TYPE__) \
    BYTESWAP_FLAG = nrn_need_byteswap; \
    if (BYTESWAP_FLAG == 1) { \
        _IN__ = (char *) &(_X__); \
        for (_II__=0;_II__< sizeof(_TYPE__);_II__++) { \
                _OUT__[_II__] = _IN__[sizeof(_TYPE__)-_II__-1]; } \
        (_X__) = *((_TYPE__ *) &_OUT__); \
    }

class StringKey {
public:
  StringKey() { s_ = NULL; }
  StringKey(const char* s) { s_ = s; }
  virtual ~StringKey() { }
  bool operator!=(const StringKey& s) { return strcmp(s.s_, s_) != 0; }
  bool operator==(const StringKey& s) { return strcmp(s.s_, s_) == 0; }
  const char* s_;
};
static unsigned long key_to_hash(const StringKey& s){return s.s_[0]+7*(s.s_[1] + 11*s.s_[2]); }

declareTable(Mech2Type, StringKey, int)
implementTable(Mech2Type, StringKey, int)

static void set_mechtype(const char* name, int type);
static Mech2Type* mech2type;

void mk_mech(const char* datpath) {
  char fnamebuf[1024];
  sd_ptr fname=sdprintf(fnamebuf, sizeof(fnamebuf), "%s/%s", datpath, "bbcore_mech.dat");
  FILE* f;
  f = fopen(fname, "r");
  assert(f);
//  printf("reading %s\n", fname);
  int n=0;
  nrn_assert(fscanf(f, "%d\n", &n) == 1);
  mech2type = new Mech2Type(2*n);
  alloc_mech(n);
  for (int i=2; i < n; ++i) {
    char mname[100];
    int type=0, pnttype=0, is_art=0, is_ion=0, dsize=0, pdsize=0;
    nrn_assert(fscanf(f, "%s %d %d %d %d %d %d\n", mname, &type, &pnttype, &is_art, &is_ion, &dsize, &pdsize) == 7);
    nrn_assert(i == type);
#ifdef DEBUG
    printf("%s %d %d %d %d %d %d\n", mname, type, pnttype, is_art, is_ion, dsize, pdsize);
#endif
    set_mechtype(mname, type);
    pnt_map[type] = (char)pnttype;
    nrn_prop_param_size_[type] = dsize;
    nrn_prop_dparam_size_[type] = pdsize;
    nrn_is_artificial_[type] = is_art;
    if (is_ion) {
      double charge = 0.;
      nrn_assert(fscanf(f, "%lf\n", &charge) == 1);
      // strip the _ion
      char iname[100];
      strcpy(iname, mname);
      iname[strlen(iname) - 4] = '\0';
      //printf("%s %s\n", mname, iname);
      ion_reg(iname, charge);
    }
    //printf("%s %d %d\n", mname, nrn_get_mechtype(mname), type);
  }

  // an int32_t binary 1 is at this position. After reading can decide if
  // binary info in files needs to be byteswapped.
  int32_t x;
  nrn_assert(fread(&x, sizeof(int32_t), 1, f) == 1);
  nrn_need_byteswap = 0;
  if (x != 1) {
    BYTEHEADER;
    nrn_need_byteswap = 1;
    BYTESWAP(x, int32_t);
    assert(x == 1);
  }

  fclose(f);
  hoc_last_init();
}

static void set_mechtype(const char* name, int type) {
  char* s1 = new char[strlen(name) + 1];
  strcpy(s1, name);
  StringKey* s = new StringKey(s1);
  assert(!mech2type->find(type, *s));
  mech2type->insert(*s, type);
}

int nrn_get_mechtype(const char* name) {
  int type;
  StringKey s(name);
  if (!mech2type->find(type, s))
    return -1;
/*
 {
    printf("could not find %s\n", name);
    abort();
  }
*/
  return type;
}
