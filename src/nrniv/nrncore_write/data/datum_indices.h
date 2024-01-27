#ifndef NRN_DATUM_INDICES_H
#define NRN_DATUM_INDICES_H

// assume all Datum.pval point into this cell. In practice, this holds because
// they point either to the area or an ion property of the given node.
// This is tightly coupled to cache_efficient
// NrnThread.NrnThreadMembList.Memb_List.data and pdata etc.
class DatumIndices {
  public:
    DatumIndices();
    virtual ~DatumIndices();
    int type;
    // ordering as though pdata[i][j] was pdata[0][i*sz+j]
    int* ion_type;   // negative codes semantics, positive codes mechanism type
    int* ion_index;  // index of range variable relative to beginning of that type
};

#endif  // NRN_DATUM_INDICES_H
