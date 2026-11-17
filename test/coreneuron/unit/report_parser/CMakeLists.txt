# =============================================================================
# Copyright (c) 2026 Open Brain Institute
# =============================================================================
add_executable(report_parser_test_bin test_report_parser.cpp)
target_link_libraries(report_parser_test_bin coreneuron-unit-test Catch2::Catch2WithMain)
add_test(NAME report_parser_test COMMAND $<TARGET_FILE:report_parser_test_bin>)
cpp_cc_configure_sanitizers(TARGET report_parser_test_bin TEST report_parser_test)
