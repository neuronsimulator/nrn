#include <nrnconf.h>
#include <nrniv_decl.h>
#include <string.h>
#include <ivtable.h>

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

void mk_mech(const char* fname) {
  FILE* f;
  f = fopen(fname, "r");
  assert(f);
  printf("reading %s\n", fname);
  int n;
  assert(fscanf(f, "%d\n", &n) == 1);
  mech2type = new Mech2Type(2*n);
  alloc_mech(n);
  for (int i=2; i < n; ++i) {
    char mname[100];
    int type, pnttype, is_ion, dsize, pdsize;
    assert(fscanf(f, "%s %d %d %d %d %d\n", mname, &type, &pnttype, &is_ion, &dsize, &pdsize) == 6);
    assert(i == type);
    //printf("%s %d %d %d %d %d\n", mname, type, pnttype, is_ion, dsize, pdsize);
    set_mechtype(mname, type);
    pnt_map[type] = (char)pnttype;
    nrn_prop_param_size_[type] = dsize;
    nrn_prop_dparam_size_[type] = pdsize;
    if (is_ion) {
      double charge;
      assert(fscanf(f, "%lf\n", &charge) == 1);
      // strip the _ion
      char iname[100];
      strcpy(iname, mname);
      iname[strlen(iname) - 4] = '\0';
      //printf("%s %s\n", mname, iname);
      ion_reg(iname, charge);
    }
    //printf("%s %d %d\n", mname, nrn_get_mechtype(mname), type);
  }
  fclose(f);
  hoc_last_init();
  exit(0);
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
  if (!mech2type->find(type, s)) {
    printf("could not find %s\n", name);
    abort();
  }
  return type;
}
