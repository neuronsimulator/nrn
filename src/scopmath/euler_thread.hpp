#pragma once
#include "errcodes.hpp"

#include <utility>
struct NrnThread;
double _modl_get_dt_thread(NrnThread*);
namespace neuron::scopmath {
template <typename Array, typename Callable, typename... Args>
int euler_thread(int neqn, int* var, int* der, Array p, Callable func, Args&&... args) {
    auto* const nt = get_first<NrnThread*>(std::forward<Args>(args)...);
    auto const dt = _modl_get_dt_thread(nt);

    // Calculate the derivatives
    func(std::forward<Args>(args)...);

    // Update dependent variables
    for (auto i = 0; i < neqn; i++) {
        p[var[i]] += dt * p[der[i]];
    }

    return SUCCESS;
}
}  // namespace neuron::scopmath
using neuron::scopmath::euler_thread;
