#include <nrnconf.h>
#include <nrniv_decl.h>

// file format defined in cooperation with nrncore/src/nrniv/nrnbbcore_write.cpp

static void read_nrnthread(const char* fname, NrnThread& nt);
static void setup_ThreadData(NrnThread& nt);

static Point_process* point_processes; // NetCon targets
static NetCon* netcons;

void nrn_setup(int nthread) {
  char fname[100];
  assert(nthread > 0);
  nrn_threads_create(nthread, 0); // serial threads
  for (int i = 0; i < nthread; ++i) {
    sprintf(fname, "bbcore_thread.%d.%d.dat", i, 0);
    read_nrnthread(fname, nrn_threads[i]);
    setup_ThreadData(nrn_threads[i]); // nrncore does this in multicore.c in thread_memblist_setup
    nrn_mk_table_check(); // was done in nrn_thread_memblist_setup in multicore.c
  }
}

void setup_ThreadData(NrnThread& nt) {
  for (NrnThreadMembList* tml = nt.tml; tml; tml = tml->next) {
    Memb_func& mf = memb_func[tml->index];
    Memb_list* ml = tml->ml;
    if (mf.thread_size_) {
      ml->_thread = (ThreadDatum*)ecalloc(mf.thread_size_, sizeof(ThreadDatum));
      if (mf.thread_mem_init_) {
        (*mf.thread_mem_init_)(ml->_thread);
      }
    }
  }
}

static int read_int_(FILE* f) {
  int i;
  char line[100];
  assert(fgets(line, 100, f));
  assert(sscanf(line, "%d", &i) == 1);
  return i;
}
#define read_int() read_int_(f)

static int chkpnt;

static double* read_dbl_array_(double* p, size_t n, FILE* f) {
  double* a;
  if (p) {
    a = p;
  }else{
    a = new double[n];
  }
  int i= -1;
  char line[100];
  assert(fgets(line, 100, f));
  assert(sscanf(line, "chkpnt %d\n", &i) == 1);
  assert(i == chkpnt++);
  assert(fread(a, sizeof(double), n, f) == n);
  return a;
}
#define read_dbl_array(p, n) read_dbl_array_(p, n, f)

static int* read_int_array_(int* p, size_t n, FILE* f) {
  int* a;
  if (p) {
    a = p;
  }else{
    a = new int[n];
  }
  int i= -1;
  char line[100];
  assert(fgets(line, 100, f));
  assert(sscanf(line, "chkpnt %d\n", &i) == 1);
  assert(i == chkpnt++);
  assert(fread(a, sizeof(int), n, f) == n);
  return a;
}
#define read_int_array(p,n) read_int_array_(p, n, f)

void read_nrnthread(const char* fname, NrnThread& nt) {
  FILE* f = fopen(fname, "r");
  NrnThreadMembList* tml;
  assert(f);
  nt.ncell = read_int();
  nt.end = read_int();
  int nmech = read_int();
  printf("ncell=%d end=%d nmech=%d\n", nt.ncell, nt.end, nmech);
  NrnThreadMembList* tml_last = NULL;
  for (int i=0; i < nmech; ++i) {
    tml = (NrnThreadMembList*)emalloc(sizeof(NrnThreadMembList));
    tml->ml = (Memb_list*)emalloc(sizeof(Memb_list));
    tml->next = NULL;
    tml->index = read_int();
    tml->ml->nodecount = read_int();;
    printf("index=%d nodecount=%d membfunc=%s\n", tml->index, tml->ml->nodecount, memb_func[tml->index].sym?memb_func[tml->index].sym:"None");
    if (nt.tml) {
      tml_last->next = tml;
    }else{
      nt.tml = tml;
    }
    tml_last = tml;
  }
  nt._ndata = read_int();
  nt._nidata = read_int();
  nt._nvdata = read_int();
  int nnetcon = read_int();
  int nweight = read_int();

  nt._data = (double*)ecalloc(nt._ndata, sizeof(double));
  if (nt._nidata) nt._idata = (int*)ecalloc(nt._nidata, sizeof(int));
  if (nt._nvdata) nt._vdata = (void*)ecalloc(nt._nvdata, sizeof(void*));
  int n = nt.end;
  nt._actual_rhs = nt._data + 0*n;
  nt._actual_d = nt._data + 1*n;
  nt._actual_a = nt._data + 2*n;
  nt._actual_b = nt._data + 3*n;
  nt._actual_v = nt._data + 4*n;
  nt._actual_area = nt._data + 5*n;
  size_t offset = 6*n;
  for (tml = nt.tml; tml; tml = tml->next) {
    Memb_list* ml = tml->ml;
    int n = ml->nodecount;
    int sz = nrn_prop_param_size_[tml->index];
    ml->data = nt._data + offset;
    offset += n*sz;
  }
  printf("offset=%ld ndata=%ld\n", offset, nt._ndata);
  assert(offset == nt._ndata);

  int* output_gid = read_int_array(NULL, nt.ncell);
  int* output_vindex = read_int_array(NULL, nt.ncell);
  double* output_threshold = read_dbl_array(NULL, nt.ncell);
  for (int i=0; i < nt.ncell; ++i) {
    BBS_set_gid2node(output_gid[i], nrnmpi_myid);
    assert(output_vindex[i] < nt.end);
    PreSyn* ps = new PreSyn(nt._actual_v + output_vindex[i], NULL, &nt);
    BBS_cell(output_gid[i], ps);
    ps->threshold_ = output_threshold[i];
  }
  delete [] output_gid;
  delete [] output_vindex;
  delete [] output_threshold;

  nt._v_parent_index = read_int_array(NULL, nt.end);
  read_dbl_array(nt._actual_a, nt.end);
  read_dbl_array(nt._actual_b, nt.end);
  read_dbl_array(nt._actual_area, nt.end);
  read_dbl_array(nt._actual_v, nt.end);
  Memb_list** mlmap = new Memb_list*[n_memb_func];
  for (tml = nt.tml; tml; tml = tml->next) {
    Memb_list* ml = tml->ml;
    mlmap[tml->index] = ml;
    int n = ml->nodecount;
    int sz = nrn_prop_param_size_[tml->index];
    ml->nodeindices = read_int_array(NULL, ml->nodecount);
    read_dbl_array(ml->data, n*sz);
    sz = nrn_prop_dparam_size_[tml->index];
    if (sz) {
      ml->pdata = read_int_array(NULL, n*sz);
    }else{
      ml->pdata = NULL;
    }
  }

printf("nnetcon=%d nweight=%d\n", nnetcon, nweight);
  int* srcgid = read_int_array(NULL, nnetcon);
  int* pnttype = read_int_array(NULL, nnetcon);
  int* pntindex = read_int_array(NULL, nnetcon);
  // it is likely that Point_process structures will be made unnecessary
  // by factoring into NetCon. For now we create and save both Point_process
  // and NetCon.
  point_processes = new Point_process[nnetcon];
  netcons = new NetCon[nnetcon];
  for (int i=0; i < nnetcon; ++i) {
    Point_process* pnt = point_processes + i;
    pnt->type = pnttype[i];
    Memb_list* ml = mlmap[pnt->type];
    pnt->data = ml->data + pntindex[i]*nrn_prop_param_size_[pnt->type];
    pnt->pdata = ml->pdata + pntindex[i]*nrn_prop_dparam_size_[pnt->type];
    pnt->presyn_ = NULL;
    pnt->_vnt = &nt;
    BBS_gid_connect(srcgid[i], pnt, netcons[i]);
  }
  delete [] mlmap;
  delete [] srcgid;
  delete [] pnttype;
  delete [] pntindex;
  double* weights = read_dbl_array(NULL, nweight);
  int iw = 0;
  for (int i=0; i < nnetcon; ++i) {
    NetCon& nc = netcons[i];
    for (int j=0; j < nc.cnt_; ++j) {
      nc.weight_[j] = weights[iw++];
    }
  }
  assert(iw == nweight);
  delete [] weights;
  double* delay = read_dbl_array(NULL, nnetcon);
  for (int i=0; i < nnetcon; ++i) {
    NetCon& nc = netcons[i];
    nc.delay_ = delay[i];
  }
  delete [] delay;

  fclose(f);
}
