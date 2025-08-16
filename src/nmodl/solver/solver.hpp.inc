#include <string>

// This file is generated from `crout/crout.hpp` and `newton/newton.hpp`.
//
// To avoid a dependency of the `nmodl` binary on a specific header include directory and
// to avoid mod file compilation using wrong headers, we include them like this.
//
// However, because we want to be able to test the headers separately we can't
// move them here.

namespace nmodl::solvers {
const std::string crout_hpp = R"jiowi(
@NMODL_CROUT_HPP@
)jiowi";
const std::string newton_hpp = R"jiowi(
@NMODL_CROUT_HPP@
@NMODL_NEWTON_HPP@
)jiowi";
const std::string matexp_h = R"jiowi(
@NMODL_MATEXP_H@
)jiowi";

}
