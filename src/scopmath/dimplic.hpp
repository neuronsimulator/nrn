/** @file dimplic.hpp
 *  @copyright (c) 1989-90 Duke University
 */
#pragma once
namespace neuron::scopmath {
template <typename Array, typename IndexArray>
int derivimplicit(int /* _ninits */,
                  int /* n */,
                  IndexArray /* slist */,
                  IndexArray /* dlist */,
                  Array /* p */,
                  double* /* pt */,
                  double /* dt */,
                  int (*fun)(),
                  double** /* ptemp */) {
    fun();
    return 0;
}
template <typename Array, typename Callable, typename IndexArray, typename... Args>
int derivimplicit_thread(int /* n */,
                         IndexArray /* slist */,
                         IndexArray /* dlist */,
                         Array /* p */,
                         Callable fun,
                         Args&&... args) {
    fun(std::forward<Args>(args)...);
    return 0;
}
}  // namespace neuron::scopmath
using neuron::scopmath::derivimplicit;
using neuron::scopmath::derivimplicit_thread;
