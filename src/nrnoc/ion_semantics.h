#pragma once

inline int nrn_semantics_from_ion(int type, int i) {
    return 2 * type + i;
}
inline bool nrn_semantics_is_ion(int i) {
    return i >= 0 && (i & 1) == 0;
}
inline bool nrn_semantics_is_ionstyle(int i) {
    return i >= 0 && (i & 1) == 1;
}
inline int nrn_semantics_ion_type(int i) {
    return i / 2;
}
