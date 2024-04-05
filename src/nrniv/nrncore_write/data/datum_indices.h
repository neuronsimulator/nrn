#pragma once

// assume all Datum.pval point into this cell. In practice, this holds because
// they point either to the area or an ion property of the given node.
// This is tightly coupled to cache_efficient
// NrnThread.NrnThreadMembList.Memb_List.data and pdata etc.
class DatumIndices {
  public:
    DatumIndices() = default;
    virtual ~DatumIndices();

    // These are the datum of mechanism `type`.
    int type = -1;

    // `datum_index[i]` is the index of datum `i` inside the mechanism
    // `datum_type[i]`.
    //
    // ordering as though pdata[i][j] was pdata[0][i*sz+j]
    int* datum_type = nullptr;   // negative codes semantics, positive codes mechanism type
    int* datum_index = nullptr;  // index of range variable relative to beginning of that type
};
