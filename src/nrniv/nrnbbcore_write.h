#ifndef nrnbbcore_write_h
#define nrnbbcore_write_h

#define DatumIndices nrncore_DatumIndices
#define CellGroup nrncore_CellGroup
struct Memb_list;

// assume all Datum.pval point into this cell. In practice, this holds because
// they point either to the area or an ion property of the given node.
// This is tightly coupled to cache_efficient
// NrnThread.NrnThreadMembList.Memb_List.data and pdata and
// NrnThread._actual_area
class DatumIndices {
public:
  DatumIndices();
  virtual ~DatumIndices();
  int type;
  // ordering as though pdata[i][j] was pdata[0][i*sz+j]
  int* ion_type; // -1 -2 -3 -4 -5 means area,iontype,cvodeieq,netsend,pointer
  int* ion_index; // or index into _actual_area
};

class CellGroup {
public:
  CellGroup();
  virtual ~CellGroup();
  Memb_list** type2ml;
  int group_id;
  // PreSyn, NetCon, target info
  int n_presyn;// real first
  int n_output; // real + art with gid
  int n_real_output;
  int n_mech;
  // following three are parallel arrays
  PreSyn** output_ps; //n_presyn of these, real are first, tml order for acell.
  int* output_gid; // n_presyn of these, -(type + 1000*index) if no gid
  int* output_vindex; // n_presyn of these. >=0 if associated with voltage, -(type + 1000*index) for acell.
  int n_netcon; // all that have targets associated with this threads Point_process.
  NetCon** netcons;
  int* netcon_srcgid; // -(type + 1000*index) refers to acell with no gid
  			// -1 means the netcon has no source
  int* netcon_pnttype;
  int* netcon_pntindex;
  // Datum.pval info
  int ntype;
  DatumIndices* datumindices;
};

#if defined(__cplusplus)
extern "C" {
#endif

extern size_t nrncore_netpar_bytes();
extern void nrncore_netpar_cellgroups_helper(CellGroup*);
extern int nrncore_art2index(double* param); // find the Memb_list index

#if defined(__cplusplus)
}
#endif


#endif
