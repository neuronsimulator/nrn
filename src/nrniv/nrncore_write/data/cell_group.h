#ifndef NRN_CELL_GROUP_H
#define NRN_CELL_GROUP_H

#include "datum_indices.h"
#include <vector>
#include <map>
#include <cstddef>
#include <cassert>
#include "section.h"

class PreSyn;
class NetCon;
class NrnThread;

extern "C" int nrn_dblpntr2nrncore(double*, NrnThread&, int& type, int& etype);

typedef std::pair < int, Memb_list* > MlWithArtItem;
typedef std::vector < MlWithArtItem > MlWithArt;
typedef std::map<double*, int> PVoid2Int;
typedef std::vector<std::map<int, Memb_list*> > Deferred_Type2ArtMl;

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
    int ndiam; // > 0 only if diam semantics in use.
    int n_mech;
    int* ml_vdata_offset;
    // following three are parallel arrays
    PreSyn** output_ps; //n_presyn of these, real are first, tml order for acell.
    int* output_gid; // n_presyn of these, -(type + 1000*index) if no gid
    int* output_vindex; // n_presyn of these. >=0 if associated with voltage, -(type + 1000*index) for acell.
    int n_netcon; // all that have targets associated with this threads Point_process.
    NetCon** netcons;
    int* netcon_srcgid; // -(type + 1000*index) refers to acell with no gid
    // -1 means the netcon has no source
    std::vector<int> netcon_negsrcgid_tid; // if some srcgid above are negative,
    // need to know their thread id for Phase1::read_direct. Entries here only
    // for negative srcgid in the order of netcon_srcgid and if nrn_nthread > 1.
    int* netcon_pnttype;
    int* netcon_pntindex;
    // Datum.pval info
    int ntype;
    DatumIndices* datumindices;
    MlWithArt mlwithart;

    static CellGroup* mk_cellgroups(CellGroup*); // gid, PreSyn, NetCon, Point_process relation.
    static void datumtransform(CellGroup*); // Datum.pval to int
    static void datumindex_fill(int, CellGroup&, DatumIndices&, Memb_list*); //helper
    static void mk_cgs_netcon_info(CellGroup* cgs);
    static void mk_tml_with_art(CellGroup*);
    static size_t get_mla_rankbytes(CellGroup*);
    static void clean_art(CellGroup*);

    static void setup_nrn_has_net_event();
    static inline void clear_artdata2index() {
        artdata2index_.clear();
    }

    static inline void clean_deferred_type2artml() {
        for (auto& th: deferred_type2artml_) {
            for (auto& p: th) {
                Memb_list* ml = p.second;
                if (ml->data) { delete [] ml->data; }
                if (ml->pdata) { delete [] ml->pdata; }
                delete ml;
            }
        }
        deferred_type2artml_.clear();
    }

    static Deferred_Type2ArtMl deferred_type2artml_;
    static std::vector<NetCon**> deferred_netcons;
    static void defer_clean_netcons(CellGroup*);
    static void clean_deferred_netcons();    

private:
    static PVoid2Int artdata2index_;

    static int* has_net_event_;

    static inline int nrncore_art2index(double* d) {
        assert(artdata2index_.find(d) != artdata2index_.end());
        return artdata2index_[d];
    }

    static inline int nrn_has_net_event(int type) {
        return has_net_event_[type];
    }

public:
    static inline int nrncore_pntindex_for_queue(double* d, int tid, int type) {
        Memb_list* ml = nrn_threads[tid]._ml_list[type];
        if (ml) {
            assert(d >= ml->data[0] && d < (ml->data[0] + (ml->nodecount*nrn_prop_param_size_[type])));
            return (d - ml->data[0])/nrn_prop_param_size_[type];
        }
        return nrncore_art2index(d);
    }

};

#endif //NRN_CELL_GROUP_H
