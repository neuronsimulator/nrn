#include "simcore/nrnconf.h"
#include "simcore/nrnoc/multicore.h"
#include "simcore/nrniv/nrniv_decl.h"
#include "simcore/nrnoc/nrnoc_decl.h"

// file format defined in cooperation with nrncore/src/nrniv/nrnbbcore_write.cpp

static void read_nrnthread(const char* fname, NrnThread& nt);
static void setup_ThreadData(NrnThread& nt);
static size_t model_size(int prnt);

static Point_process* point_processes; // NetCon targets
static NetCon* netcons;

void nrn_setup(int nthread, const char *path) {
  char fname[100];
  assert(nthread > 0);
  nrn_threads_create(nthread, 0); // serial threads
  for (int i = 0; i < nthread; ++i) {
//    sprintf(fname, "bbcore_thread.%d.%d.dat", i, 0);
    sprintf(fname, "%sbbcore_thread.%d.%d.dat", path, i, 0);
    read_nrnthread(fname, nrn_threads[i]);
    setup_ThreadData(nrn_threads[i]); // nrncore does this in multicore.c in thread_memblist_setup
    nrn_mk_table_check(); // was done in nrn_thread_memblist_setup in multicore.c
  }
  model_size(2);
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
  int nart = read_int();
  int nart_without_gid = read_int();
  int nart_without_gid_netcons = read_int();
  int nart_without_gid_netcons_wcnt = read_int();
  assert (nart == nart_without_gid);

  //printf("ncell=%d end=%d nmech=%d\n", nt.ncell, nt.end, nmech);
  //printf("nart=%d nart_without_gid=%d nart_without_gid_netcons=%d\n", nart, nart_without_gid, nart_without_gid_netcons);
  //printf("nart_without_gid_netcons_wcnt = %d\n", nart_without_gid_netcons_wcnt);
  NrnThreadMembList* tml_last = NULL;
  for (int i=0; i < nmech; ++i) {
    tml = (NrnThreadMembList*)emalloc(sizeof(NrnThreadMembList));
    tml->ml = (Memb_list*)emalloc(sizeof(Memb_list));
    tml->next = NULL;
    tml->index = read_int();
    tml->ml->nodecount = read_int();;
    //printf("index=%d nodecount=%d membfunc=%s\n", tml->index, tml->ml->nodecount, memb_func[tml->index].sym?memb_func[tml->index].sym:"None");
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
  nt.n_weight = nweight + nart_without_gid_netcons_wcnt;
  nt._weight = (double*)ecalloc(nt.n_weight, sizeof(double));

  nt._data = (double*)ecalloc(nt._ndata, sizeof(double));
  if (nt._nidata) nt._idata = (int*)ecalloc(nt._nidata, sizeof(int));
  if (nt._nvdata) nt._vdata = (void**)ecalloc(nt._nvdata, sizeof(void*));
  //printf("_ndata=%d _nidata=%d _nvdata=%d\n", nt._ndata, nt._nidata, nt._nvdata);
  int n = nt.end;
  nt._actual_rhs = nt._data + 0*n;
  nt._actual_d = nt._data + 1*n;
  nt._actual_a = nt._data + 2*n;
  nt._actual_b = nt._data + 3*n;
  nt._actual_v = nt._data + 4*n;
  nt._actual_area = nt._data + 5*n;
  size_t offset = 6*n;
  int nsyn = 0;
  for (tml = nt.tml; tml; tml = tml->next) {
    Memb_list* ml = tml->ml;
    int type = tml->index;
    int n = ml->nodecount;
    int sz = nrn_prop_param_size_[type];
    ml->data = nt._data + offset;
    offset += n*sz;
    if (pnt_map[type] > 0 && nrn_is_artificial_[type] == 0) {
      nsyn += n;
    }
  }
  //printf("offset=%ld ndata=%ld\n", offset, nt._ndata);
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
  int synoffset = 0;
  int acelloffset = 0;
  int* pnt_offset = new int[n_memb_func];
  nt.synapses = new Point_process[nsyn];
  nt.acells = new Point_process[nart];
  nt.n_syn = nsyn;
  nt.n_acell = nart;
  nt.n_acellnetcon = nart_without_gid_netcons;
  for (tml = nt.tml; tml; tml = tml->next) {
    int type = tml->index;
    Memb_list* ml = tml->ml;
    mlmap[type] = ml;
    int is_art = nrn_is_artificial_[type];
    int n = ml->nodecount;
    int szp = nrn_prop_param_size_[type];
    int szdp = nrn_prop_dparam_size_[type];
    if (!is_art) {ml->nodeindices = read_int_array(NULL, ml->nodecount);}
    read_dbl_array(ml->data, n*szp);
    if (szdp) {
      ml->pdata = read_int_array(NULL, n*szdp);
    }else{
      ml->pdata = NULL;
    }
    if (pnt_map[type] > 0) { // POINT_PROCESS mechanism
      int cnt = ml->nodecount;
      Point_process* pnt = NULL;
      if (is_art) {
        pnt = nt.acells + acelloffset;
        pnt_offset[type] = acelloffset;
        acelloffset += cnt;
      }else{
        pnt = nt.synapses + synoffset;
        pnt_offset[type] = synoffset;
        synoffset += cnt;
      }
      for (int i=0; i < cnt; ++i) {
        Point_process* pp = pnt + i;
        pp->type = type;
        pp->data = ml->data + i*szp;
        pp->pdata = ml->pdata + i*szdp;
        nt._vdata[pp->pdata[1]] = pp;
        pp->presyn_ = NULL;
        pp->_vnt = &nt;
      }
    }
  }

  //put the artcells in a different tml list (all the artcells are last)
  for (tml = nt.tml; tml; tml = tml->next) {
    NrnThreadMembList* tmlnext = tml->next;
    if (tmlnext && nrn_is_artificial_[tmlnext->index]) {
      tml->next = NULL;
      nt.acell_tml = tmlnext;
      break;
    }
  }

//printf("nnetcon=%d nweight=%d\n", nnetcon, nweight);
  int* srcgid = read_int_array(NULL, nnetcon);
  int* pnttype = read_int_array(NULL, nnetcon);
  int* pntindex = read_int_array(NULL, nnetcon);
  // it is likely that Point_process structures will be made unnecessary
  // by factoring into NetCon.
  nt.netcons = new NetCon[nnetcon];
  nt.n_netcon = nnetcon;
  // instead of looping once using a dynamically expanding NetConPList
  // used a fixed NetCon** allocation for each PreSyn. So count, allocate
  // and fill
  for (int i=0; i < nnetcon; ++i) {
    gid_connect_count(srcgid[i]);
  }
  gid_connect_allocate(); // and sets all the nc_cnt_ back to 0
  for (int i=0; i < nnetcon; ++i) {
    int type = pnttype[i];
    int index = pnt_offset[type] + pntindex[i];
    Point_process* pnt = nt.synapses + index;
    BBS_gid_connect(srcgid[i], pnt, nt.netcons[i]);
  }
  delete [] srcgid;
  delete [] pntindex;
  double* weights = read_dbl_array(NULL, nweight);
  for (int i=0; i < nweight; ++i) {
    nt._weight[i] = weights[i];
  }
  delete [] weights;
  int iw = 0;
  for (int i=0; i < nnetcon; ++i) {
    NetCon& nc = nt.netcons[i];
    nc.weight_ = nt._weight + iw;
    iw += pnt_receive_size[pnttype[i]];
  }
  assert(iw == nweight);
  delete [] pnttype;
  double* delay = read_dbl_array(NULL, nnetcon);
  for (int i=0; i < nnetcon; ++i) {
    NetCon& nc = nt.netcons[i];
    nc.delay_ = delay[i];
  }
  delete [] delay;

  // now the artcells
  int n_nc = nart_without_gid_netcons;
  int n_wt = nart_without_gid_netcons_wcnt;
  int* art_nc_cnts = read_int_array(NULL, nart);
  int* target_type = read_int_array(NULL, n_nc);
  int* target_index = read_int_array(NULL, n_nc);
  weights = read_dbl_array(NULL, n_wt);
  for (int i = 0; i < n_wt; ++i) {
    nt._weight[i + iw] = weights[i];
  }
  delay = read_dbl_array(NULL, n_nc);
  int inc = 0;
  nt.acell_netcons = new NetCon[nart_without_gid_netcons];
  for (int i = 0; i < nart; ++i) {
    Point_process& acell = nt.acells[i];
    PreSyn* ps = new PreSyn(NULL, NULL, NULL);
    acell.presyn_ = ps;
    for (int j=0; j < art_nc_cnts[i]; ++j) {
      NetCon& nc = nt.acell_netcons[inc];
      int offset = pnt_offset[target_type[inc]];
      Point_process* target = nt.synapses + (offset + target_index[inc]);
      nc.init(ps, target);
      nc.weight_ = nt._weight + iw;
      iw += pnt_receive_size[target_type[inc]];
      nc.delay_ = delay[inc];
      ++inc;
    }
  }
  assert(iw == nt.n_weight);
  delete [] pnt_offset;
  delete [] art_nc_cnts;
  delete [] target_type;
  delete [] target_index;
  delete [] weights;
  delete [] delay;

  // BBCOREPOINTER information
  int npnt = read_int();
  for (int i=0; i < npnt; ++i) {
    int type = read_int();
    assert(nrn_bbcore_read_[type]);
    int sz = read_int();
    int cnt = read_int();
    void* vp;
    if (sz == sizeof(int)) {
      vp = read_int_array(NULL, cnt);
    }else if (sz == sizeof(double)) {
      vp = read_dbl_array(NULL, cnt);
    }else{
      assert(0);
    }
    int k=0;
    Memb_list* ml = mlmap[type];
    int dsz = nrn_prop_param_size_[type];
    int pdsz = nrn_prop_dparam_size_[type];
    for (int j=0; j < ml->nodecount; ++j) {
      double* d = ml->data + j*dsz;
      Datum* pd = ml->pdata + j*pdsz;
      k = (*nrn_bbcore_read_[type])(vp, k, d, pd, ml->_thread, &nt);
    }
    assert(k == cnt);
  }
  delete [] mlmap;

  fclose(f);
}

static size_t memb_list_size(NrnThreadMembList* tml, int prnt) {
  size_t sz_ntml = sizeof(NrnThreadMembList);
  size_t sz_ml = sizeof(Memb_list);
  size_t szi = sizeof(int);
  size_t nbyte = sz_ntml + sz_ml;
  nbyte += tml->ml->nodecount*szi;
  if (prnt > 1) {
    int i = tml->index;
    printf("%s %d psize=%d ppsize=%d cnt=%d nbyte=%ld\n", memb_func[i].sym, i,
      nrn_prop_param_size_[i], nrn_prop_dparam_size_[i], tml->ml->nodecount, nbyte);
  }
  return nbyte;
}

size_t model_size(int prnt) {
  size_t nbyte = 0;
  size_t szd = sizeof(double);
  size_t szi = sizeof(int);
  size_t szv = sizeof(void*);
  size_t sz_th = sizeof(NrnThread);
  size_t sz_pp = sizeof(Point_process);
  size_t sz_nc = sizeof(NetCon);
  NrnThreadMembList* tml;

  for (int i=0; i < nrn_nthread; ++i) {
    NrnThread& nt = nrn_threads[i];
    size_t nb_nt = 0; // per thread

    // Memb_list size
    int nmech = 0;
    for (tml=nt.tml; tml; tml = tml->next) {
      nb_nt += memb_list_size(tml, prnt);
      ++nmech;
    }
    for (tml=nt.acell_tml; tml; tml = tml->next) {
      nb_nt += memb_list_size(tml, prnt);
      ++nmech;
    }

    // basic thread size includes mechanism data and G*V=I matrix
    nb_nt + sz_th;
    nb_nt += nt._ndata*szd + nt._nidata*szi + nt._nvdata*szv;
    nb_nt += nt.end*szi; // _v_parent_index

    if (prnt > 1) {
      printf("ncell=%d end=%d n_acell=%d nmech=%d\n", nt.ncell, nt.end, nt.n_acell, nmech);
      printf("ndata=%d nidata=%d nvdata=%d\n", nt._ndata, nt._nidata, nt._nvdata);
      printf("nbyte so far %ld\n", nb_nt);
      printf("n_syn=%d sz=%ld nbyte=%ld\n", nt.n_syn, sz_pp, nt.n_syn*sz_pp);
      printf("n_netcon=%d sz=%ld nbyte=%ld\n", nt.n_netcon, sz_nc, nt.n_netcon*sz_nc);
      printf("n_acellnetcon=%d\n", nt.n_acellnetcon);
      printf("n_weight = %d\n", nt.n_weight);
    }

    // spike handling
    nb_nt += output_presyn_size(prnt);
    nb_nt += input_presyn_size(prnt);
    nb_nt += nt.n_syn*sz_pp + nt.n_netcon*sz_nc
             + nt.n_acell*sz_pp + nt.n_acellnetcon*sz_nc
             + nt.n_weight*szd;
    nbyte += nb_nt;
    if (prnt) {printf("%d thread %d total bytes %ld\n", nrnmpi_myid, i, nb_nt);}
  }
  return nbyte;
}
