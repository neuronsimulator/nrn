#include "datum_indices.h"

DatumIndices::DatumIndices() {
    type = -1;
    ion_type = ion_index = 0;
}

DatumIndices::~DatumIndices() {
    if (ion_type)
        delete[] ion_type;
    if (ion_index)
        delete[] ion_index;
}
