/*************************************************************************
 * Copyright (C) 2019-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <catch/catch.hpp>

#include "codegen/codegen_c_visitor.hpp"
#include "codegen/codegen_ispc_visitor.hpp"
#include "codegen/codegen_utils.hpp"

using namespace nmodl;
using namespace visitor;
using namespace codegen;

using input_result_map = std::unordered_map<std::string, std::string>;

SCENARIO("C codegen utility functions", "[codegen][util][c]") {
    GIVEN("Double constant as string") {
        std::string double_constant = "0.012345678901234567";

        THEN("Codegen C Visitor prints double with same precision") {
            auto nmodl_constant_result = codegen::utils::format_double_string<CodegenCVisitor>(
                double_constant);
            REQUIRE(nmodl_constant_result == double_constant);
        }
    }

    GIVEN("Integer constant as string") {
        std::string double_constant = "1";

        std::string codegen_output = "1.0";

        THEN("Codegen C Visitor prints integer as double number") {
            auto nmodl_constant_result = codegen::utils::format_double_string<CodegenCVisitor>(
                double_constant);
            REQUIRE(nmodl_constant_result == codegen_output);
        }
    }

    GIVEN("Double constants in scientific notation as strings") {
        input_result_map tests({{"1e+18", "1e+18"}, {"1e-18", "1e-18"}, {"1E18", "1E18"}});

        THEN("Codegen C Visitor prints doubles with scientific notation") {
            for (const auto& test: tests) {
                REQUIRE(codegen::utils::format_double_string<CodegenCVisitor>(test.first) ==
                        test.second);
            }
        }
    }

    GIVEN("Float constant as string") {
        std::string float_constant = "0.01234567";

        THEN("Codegen C Visitor prints float with same precision") {
            auto nmodl_constant_result = codegen::utils::format_float_string<CodegenCVisitor>(
                float_constant);
            REQUIRE(nmodl_constant_result == float_constant);
        }
    }

    GIVEN("Float constant as string") {
        std::string float_constant = "1";

        std::string codegen_output = "1.0";

        THEN("Codegen C Visitor prints integer as double number") {
            auto nmodl_constant_result = codegen::utils::format_float_string<CodegenCVisitor>(
                float_constant);
            REQUIRE(nmodl_constant_result == codegen_output);
        }
    }

    GIVEN("Float constants in scientific notation as strings") {
        input_result_map tests({{"1e+18", "1e+18"}, {"1e-18", "1e-18"}, {"1E18", "1E18"}});

        THEN("Codegen C Visitor prints doubles with scientific notation") {
            for (const auto& test: tests) {
                REQUIRE(codegen::utils::format_float_string<CodegenCVisitor>(test.first) ==
                        test.second);
            }
        }
    }
}

SCENARIO("ISPC codegen utility functions", "[codegen][util][ispc]") {
    GIVEN("Double constant as string") {
        input_result_map tests({{"0.012345678901234567", "0.012345678901234567d"},
                                {".012345678901234567", "0.012345678901234567d"},
                                {"123.", "123.d"}});

        THEN("Codegen ISPC Visitor prints double with same precision") {
            for (const auto& test: tests) {
                REQUIRE(codegen::utils::format_double_string<CodegenIspcVisitor>(test.first) ==
                        test.second);
            }
        }
    }

    GIVEN("Integer double constant as string") {
        std::string double_constant = "1";

        std::string codegen_output = "1.0d";

        THEN("Codegen ISPC Visitor prints integer as double number") {
            auto nmodl_constant_result = codegen::utils::format_double_string<CodegenIspcVisitor>(
                double_constant);
            REQUIRE(nmodl_constant_result == codegen_output);
        }
    }

    GIVEN("Double constants in scientific notation as strings") {
        input_result_map tests(
            {{"1e+18", "1d+18"}, {"1e-18", "1d-18"}, {".123e18", ".123d18"}, {"1E18", "1d18"}});

        THEN("Codegen ISPC Visitor prints doubles with scientific notation") {
            for (const auto& test: tests) {
                REQUIRE(codegen::utils::format_double_string<CodegenIspcVisitor>(test.first) ==
                        test.second);
            }
        }
    }

    GIVEN("Float constant as string") {
        input_result_map tests(
            {{"0.01234567", "0.01234567f"}, {".01234567", "0.01234567f"}, {"123.", "123.f"}});

        THEN("Codegen ISPC Visitor prints float with same precision") {
            for (const auto& test: tests) {
                REQUIRE(codegen::utils::format_float_string<CodegenIspcVisitor>(test.first) ==
                        test.second);
            }
        }
    }

    GIVEN("Integer float constant as string") {
        std::string float_constant = "1";

        std::string codegen_output = "1.0f";

        THEN("Codegen ISPC Visitor prints integer as double number") {
            auto nmodl_constant_result = codegen::utils::format_float_string<CodegenIspcVisitor>(
                float_constant);
            REQUIRE(nmodl_constant_result == codegen_output);
        }
    }

    GIVEN("Float constants in scientific notation as strings") {
        input_result_map tests(
            {{"1e+18", "1e+18"}, {"1e-18", "1e-18"}, {".123e18", ".123e18"}, {"1E18", "1E18"}});

        THEN("Codegen ISPC Visitor prints doubles with scientific notation") {
            for (const auto& test: tests) {
                REQUIRE(codegen::utils::format_float_string<CodegenIspcVisitor>(test.first) ==
                        test.second);
            }
        }
    }
}
