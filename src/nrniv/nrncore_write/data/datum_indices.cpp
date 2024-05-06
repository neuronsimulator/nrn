#include "datum_indices.h"

DatumIndices::~DatumIndices() {
    delete[] datum_type;
    delete[] datum_index;
}
