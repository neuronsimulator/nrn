#include <catch2/catch_test_macros.hpp>

#include "ast/program.hpp"
#include "codegen/codegen_info.hpp"
#include "parser/nmodl_driver.hpp"

using namespace nmodl;
using namespace visitor;
using namespace codegen;

using nmodl::parser::NmodlDriver;

TEST_CASE("Check ion variable names") {
    auto ion = Ion("na");

    REQUIRE(ion.intra_conc_name() == "nai");
    REQUIRE(ion.intra_conc_pointer_name() == "ion_nai");

    REQUIRE(ion.extra_conc_name() == "nao");
    REQUIRE(ion.extra_conc_pointer_name() == "ion_nao");

    REQUIRE(ion.rev_potential_name() == "ena");
    REQUIRE(ion.rev_potential_pointer_name() == "ion_na_erev");

    REQUIRE(ion.ionic_current_name() == "ina");
    REQUIRE(ion.ionic_current_pointer_name() == "ion_ina");

    REQUIRE(ion.current_derivative_name() == "dinadv");
    REQUIRE(ion.current_derivative_pointer_name() == "ion_dinadv");
}
