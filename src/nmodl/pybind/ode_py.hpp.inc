#include <string>

// This file is generated from `ode.py`.
//
// During code-generation NMODL needs to call SymPy, e.g. to compute
// derivatives symbolically. The code to do this can be found in `ode.py`.
//
// To avoid a dependency of the `nmodl` binary on the Python library `nmodl`.
// We embed `ode.py` like this.
//
// However, because we want to be able to test `ode.py` via pytest we can't
// move it here.

namespace nmodl::pybind_wrappers {
const std::string ode_py = R"jiowi(
@NMODL_ODE_PY@
)jiowi";
#ifdef NRN_ENABLE_COVERAGE
const std::string ode_py_path = "@NMODL_ODE_PY_PATH@";
#endif
}
