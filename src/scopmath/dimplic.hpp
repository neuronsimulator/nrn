/** @file dimplic.hpp
 *  @copyright (c) 1989-90 Duke University
 */
#pragma once
struct Datum;
struct NrnThread;
namespace neuron::scopmath {
template <typename Array>
int derivimplicit(int /* _ninits */,
                  int /* n */,
                  int* /* slist */,
                  int* /* dlist */,
                  Array /* p */,
                  double* /* pt */,
                  double /* dt */,
                  int (*fun)(),
                  double** /* ptemp */) {
    fun();
    return 0;
}
inline int derivimplicit_thread(int /* n */,
                                int* /* slist */,
                                int* /* dlist */,
                                double* p,
                                int (*fun)(double*, Datum*, Datum*, NrnThread*),
                                Datum* ppvar,
                                Datum* thread,
                                NrnThread* nt) {
    fun(p, ppvar, thread, nt);
    return 0;
}
}  // namespace neuron::scopmath
using neuron::scopmath::derivimplicit;
using neuron::scopmath::derivimplicit_thread;
