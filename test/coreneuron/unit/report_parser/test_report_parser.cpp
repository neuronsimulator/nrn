/*
# =============================================================================
# Copyright (c) 2026 Open Brain Institute
# =============================================================================.
*/
#include <catch2/catch_test_macros.hpp>

#include <csignal>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

#include "coreneuron/io/reports/nrnreport.hpp"

using namespace coreneuron;

namespace {
const std::vector<int> gids{1, 2, 3};

/// Writes a report.conf of one report, as neurodamus writes it.
void write_report_conf(const std::string& file_name, const std::string& metadata_line) {
    std::ofstream conf(file_name, std::ios::binary);
    conf << "1\n" << metadata_line << '\n';
    conf.write(reinterpret_cast<const char*>(gids.data()), gids.size() * sizeof(int));
    conf << "\n1\nAll 0\nout.dat\n";
}

ReportConfiguration parse_report(const std::string& file_name, const std::string& metadata_line) {
    write_report_conf(file_name, metadata_line);
    SpikesInfo spikes_info;
    auto reports = create_report_configurations(file_name, ".", spikes_info);
    REQUIRE(reports.size() == 1);
    REQUIRE(spikes_info.file_name == "out.dat");
    return reports[0];
}
}  // namespace

TEST_CASE("A report line with no method is by_cell") {
    // as written before the method existed: the line ends with the scaling
    const auto report = parse_report("no_method.conf",
                                     "lfp.h5 Mosaic lfp i_membrane mV SONATA All All 0.1 0.0 "
                                     "10.0 3 8 Area");
    REQUIRE(report.type == ReportType::LFP);
    REQUIRE(report.scaling == Scaling::Area);
    REQUIRE(report.lfp_method == LFPReportMethod::ByCell);
    REQUIRE(report.target == gids);
}

TEST_CASE("sum is accepted on an lfp line") {
    const auto report = parse_report("lfp_sum.conf",
                                     "lfp_sum.h5 Mosaic lfp i_membrane mV SONATA All All 0.1 0.0 "
                                     "10.0 3 8 None sum");
    REQUIRE(report.type == ReportType::LFP);
    REQUIRE(report.scaling == Scaling::None);
    REQUIRE(report.lfp_method == LFPReportMethod::Sum);
    REQUIRE(report.target == gids);
}

TEST_CASE("sum is rejected on a non-lfp line") {
    const std::string line =
        "soma.h5 Mosaic compartment v mV SONATA Soma Center 0.1 0.0 10.0 3 8 None";
    // neurodamus writes by_cell on every line
    REQUIRE(parse_report("compartment_by_cell.conf", line + " by_cell").lfp_method ==
            LFPReportMethod::ByCell);

    // The parser aborts, which Catch2 cannot catch: it parses in a child process.
    write_report_conf("compartment_sum.conf", line + " sum");
    const pid_t child = fork();
    REQUIRE(child != -1);
    if (child == 0) {
        std::signal(SIGABRT, SIG_DFL);  // rather than Catch2's handler, which reports a failure
        SpikesInfo spikes_info;
        create_report_configurations("compartment_sum.conf", ".", spikes_info);
        std::_Exit(0);
    }
    int status = 0;
    REQUIRE(waitpid(child, &status, 0) == child);
    REQUIRE(WIFSIGNALED(status));
    REQUIRE(WTERMSIG(status) == SIGABRT);
}
