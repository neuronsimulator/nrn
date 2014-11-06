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

// Want to have the classical NEURON PatternStim functionality available
// in corebluron to allow debugging and trajectory verification on
// desktop single process tests.  Since pattern.mod provides most of what
// we need even in the corebluron context, we placed a minimally modified
// version of that in corebluron/mech/modfile/pattern.mod and this file
// provides an interface that creates an instance of the
// PatternStim ARTIFICIAL_CELL in thread 0 and attaches the spike raster
// data to it.

#include "corebluron/nrnconf.h"
#include "corebluron/nrnoc/multicore.h"
#include "corebluron/nrniv/nrniv_decl.h"
#include "corebluron/nrnoc/nrnoc_decl.h"
#include "corebluron/nrniv/output_spikes.h"
#include "corebluron/nrniv/nrn_assert.h"

extern "C" {
void _pattern_reg(void);
extern void pattern_stim_setup_helper(int size, double* tvec, int* gidvec,
  int cnt, double* _p, Datum* _ppvar, ThreadDatum* _thread, NrnThread* _nt);
}

static int read_raster_file(const char* fname, double** tvec, int** gidvec);

int nrn_extra_thread0_vdata;

void nrn_set_extra_thread0_vdata() {
  // limited to PatternStim for now.
  // if called, must be called before nrn_setup and after mk_mech.
  int type = nrn_get_mechtype("PatternStim");
  if (!memb_func[type].sym) {
    // the NEURON mod file version is not vectorized so the param size
    // differs by 1 from the corebluron version.
     nrn_prop_param_size_[type] += 1;
    _pattern_reg();
  }
  nrn_extra_thread0_vdata = nrn_prop_dparam_size_[type];
}

// fname is the filename of an output_spikes.h format raster file.
void nrn_mkPatternStim(const char* fname) {
  int type = nrn_get_mechtype("PatternStim");
  if (!memb_func[type].sym) {
    printf("nrn_set_extra_thread_vdata must be called (after mk_mech, and before nrn_setup\n");
    assert(0);
  }

  double* tvec;
  int* gidvec;
  int size = read_raster_file(fname, &tvec, &gidvec);
  printf("raster size = %d\n", size);
#if 0
  for (int i=0; i < size; ++i) { printf("%g %d\n", tvec[i], gidvec[i]);}
#endif

  Point_process* pnt = nrn_artcell_instantiate("PatternStim");
  NrnThread* nt = nrn_threads + pnt->_tid;
  Memb_list* ml =  nt->_ml_list[type];
  int layout = nrn_mech_data_layout_[type];
  int sz = nrn_prop_param_size_[type];
  int psz = nrn_prop_dparam_size_[type];
  int _cntml = ml->nodecount;
  int _iml = pnt->_i_instance;
  double* _p = ml->data;
  Datum* _ppvar = ml->pdata;
  if (layout == 1) {
    _p += _iml*sz; _ppvar += _iml*psz;
  }else if (layout == 0) {
    _p += _iml; _ppvar += _iml;
  }else{
    assert(0);
  }    
  pattern_stim_setup_helper(size, tvec, gidvec, _cntml, _p, _ppvar, NULL, nt); 
}

int read_raster_file(const char* fname, double** tvec, int** gidvec) {
  FILE* f = fopen(fname, "r");
  assert(f);
  int size = 0;
  int bufsize = 10000;
  *tvec = (double*)emalloc(bufsize*sizeof(double));
  *gidvec = (int*)emalloc(bufsize*sizeof(int));

  double st;
  int gid;
  char dummy[100];
  nrn_assert(fgets(dummy, 100, f));
  while (fscanf(f, "%lf %d\n", &st, &gid) == 2) {
    if (size >= bufsize) {
	bufsize *= 2;
	*tvec = (double*)erealloc(*tvec, bufsize*sizeof(double));
	*gidvec = (int*)erealloc(*gidvec, bufsize*sizeof(int));
    }
    (*tvec)[size] = st;
    (*gidvec)[size] = gid;
    ++size;
  }
  fclose(f);
  return size;
}


// Opportunistically implemented to create a single PatternStim.
// So only does enough to get that functionally incorporated into the model
// and other types may require additional work. In particular, we
// append a new NrnThreadMembList with one item to the thread 0 tml list
// in order for the artificial cell to get its INITIAL block called but
// we do not modify any of the other thread 0 data arrays or counts.

Point_process* nrn_artcell_instantiate(const char* mechname) {
  int type = nrn_get_mechtype(mechname);
  printf("nrn_artcell_instantiate %s type=%d\n", mechname, type);
  NrnThread* nt = nrn_threads + 0;

  // see nrn_setup.cpp:read_phase2 for how it creates NrnThreadMembList instances.
  // create and append to nt.tml
  assert(nt->_ml_list[type] == NULL); //FIXME
  NrnThreadMembList* tml = (NrnThreadMembList*)emalloc(sizeof(NrnThreadMembList));
  tml->ml = (Memb_list*)emalloc(sizeof(Memb_list));
  nt->_ml_list[type] = tml->ml;
  tml->index = type;
  tml->next = NULL;
  if (!nt->tml) {
    nt->tml = tml;
  }else{
    for (NrnThreadMembList* i = nt->tml; i; i = i->next) {
      if (!i->next) {
        i->next = tml;
        break;
      }
    }
  }

  // fill in tml->ml info. The data is not in the cache efficient
  // NrnThread arrays but there should not be many of these instances.
  int psize = nrn_prop_param_size_[type];
  int dsize = nrn_prop_dparam_size_[type];
  // int layout = nrn_mech_data_layout_[type]; // not needed because singleton
  Memb_list* ml = tml->ml;
  ml->nodecount = 1;
  ml->nodeindices = NULL;
  ml->data = (double*)ecalloc(ml->nodecount*psize, sizeof(double));
  ml->pdata = (Datum*)ecalloc(ml->nodecount*dsize, sizeof(Datum));
  ml->_thread = NULL;

  // Here we have a problem with no easy general solution. ml->pdata are
  // integer indexes into the nt->_data nt->_idata and nt->_vdata array
  // depending on context, 
  // but nrn_setup.cpp allocated these to exactly have the size needed by
  // the file defined model (at least for _vdata) and so there are no slots
  // for pdata to index into for this new instance.
  // So nrn_setup.cpp:phase2 needs to
  // be notified that some extra space will be required. For now, defer
  // the general situation of several instances for several types and
  // demand that this method is never called more than once. We introduce
  // a int nrn_extra_thread0_vdata (only that is needed by PatternStim)
  //  which will be used by
  // nrn_setup.cpp:phase2 to allocate the appropriately larger
  // _vdata arrays for thread 0 (without changing _nvdata so
  // that we can fill in the indices here)
  static int cnt = 0;
  if (++cnt > 1) {
    printf("nrn_artcell_instantiate cannot be called more than once\n");
    assert(0);
  }
  // note that PatternStim internal usage for the 4 ppvar values  is:
  // #define _nd_area  _nt->_data[_ppvar[0]]  (not used since ARTIFICIAL_CELL)
  // #define _p_ptr  _nt->_vdata[_ppvar[2]] (the BBCORE_POINTER)
  // #define _tqitem &(_nt->_vdata[_ppvar[3]]) (for net_send)
  // and general external usage is:
  // _nt->_vdata[_ppvar[1]] = Point_process*
  // 

  Point_process* pnt = new Point_process;
  pnt->_type = type;
  pnt->_tid = nt->id;
  pnt->_i_instance = 0;
  // as though all dparam index into _vdata
  assert(dsize <= nrn_extra_thread0_vdata);
  for (int i=0; i < dsize; ++i) {
    ml->pdata[i] = nt->_nvdata + i;
  }
  nt->_vdata[nt->_nvdata + 1] = (void*)pnt;

  return pnt;
}
