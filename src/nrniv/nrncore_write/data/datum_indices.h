#ifndef NRN_DATUM_INDICES_H
#define NRN_DATUM_INDICES_H

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

#endif //NRN_DATUM_INDICES_H
