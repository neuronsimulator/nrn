/*
# =============================================================================
# Copyright (c) 2025 Open Brain Institute
# =============================================================================.
*/
#include <catch2/catch_test_macros.hpp>
#include <iostream>

#include "coreneuron/coreneuron.hpp"
#include "coreneuron/mechanism/mech_mapping.hpp"
#include "coreneuron/mechanism/mechanism.hpp"
#include "coreneuron/permute/data_layout.hpp"

using namespace coreneuron;

// Note: Catch2 does not natively support death tests. I cannot test malformed report configurations
// (Katta)
TEST_CASE("register_all_variables_offsets and get_var_location_from_var_name") {
    // Ensure mech_data_layout has enough entries
    corenrn.get_mech_data_layout().resize(43, SOA_LAYOUT);
    const int mech_id = 42;

    const char* names[] = {"var0", "var1", "var2", "var3_mechX", nullptr, nullptr, nullptr};
    SerializedNames serialized_names = names;

    register_all_variables_offsets(mech_id, serialized_names);

    Memb_list ml;
    ml._nodecount_padded = 2;
    const int nvars = 4;
    ml.data = new double[nvars * ml._nodecount_padded];

    for (int i = 0; i < nvars * ml._nodecount_padded; ++i) {
        ml.data[i] = i + 100;
    }

    std::string mech_name = "mechX";

    SECTION("basic variable retrieval") {
        for (int var_idx = 0; var_idx < nvars; ++var_idx) {
            std::string var_name = names[var_idx];

            double* val = get_var_location_from_var_name(mech_id, mech_name, var_name, &ml, 1);

            int expected_ix = var_idx * ml._nodecount_padded + 1;
            REQUIRE(val == &ml.data[expected_ix]);
            REQUIRE(*val == 100 + expected_ix);
        }
    }

    SECTION("fallback variable retrieval (var3 -> var3_mechX)") {
        double* val = get_var_location_from_var_name(mech_id, mech_name, "var3", &ml, 1);

        int expected_ix = 3 * ml._nodecount_padded + 1;
        REQUIRE(val == &ml.data[expected_ix]);
        REQUIRE(*val == 100 + expected_ix);
    }

    delete[] ml.data;
}
