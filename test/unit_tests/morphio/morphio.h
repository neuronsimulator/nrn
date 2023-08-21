#pragma once
#include <tuple>
namespace nrn::test {

// HOC BasicCell template required for a traditional morphology read from hoc
constexpr auto hoc_basic_cell_template = R"(
begintemplate HocBasicCell
objref this, somatic, all, axonal, basal, apical
proc init() {
        all     = new SectionList()
        somatic = new SectionList()
        basal   = new SectionList()
        apical  = new SectionList()
        axonal  = new SectionList()
}

create soma[1], dend[1], apic[1], axon[1]
endtemplate HocBasicCell
)";

const std::vector<std::string> soma_single_point_h5_hoc{
    "create soma[1]",
    "forsec \"soma\" somatic.append",
    "create axon[3]",
    "forsec \"axon\" axonal.append",
    "create dend[3]",
    "forsec \"dend\" basal.append",
    "forall all.append",
    "soma { pt3dadd(1, 1.7484555e-07, 0, 1) }",
    "soma { pt3dadd(0.94581723, -0.32469952, 0, 1) }",
    "soma { pt3dadd(0.78914064, -0.61421257, 0, 1) }",
    "soma { pt3dadd(0.54694813, -0.83716649, 0, 1) }",
    "soma { pt3dadd(0.24548566, -0.96940023, 0, 1) }",
    "soma { pt3dadd(-0.082579389, -0.99658448, 0, 1) }",
    "soma { pt3dadd(-0.40169525, -0.91577339, 0, 1) }",
    "soma { pt3dadd(-0.67728162, -0.73572391, 0, 1) }",
    "soma { pt3dadd(-0.87947375, -0.47594735, 0, 1) }",
    "soma { pt3dadd(-0.98636132, -0.16459456, 0, 1) }",
    "soma { pt3dadd(-0.98636132, 0.16459462, 0, 1) }",
    "soma { pt3dadd(-0.87947375, 0.47594741, 0, 1) }",
    "soma { pt3dadd(-0.67728156, 0.73572391, 0, 1) }",
    "soma { pt3dadd(-0.4016954, 0.91577333, 0, 1) }",
    "soma { pt3dadd(-0.08257933, 0.99658448, 0, 1) }",
    "soma { pt3dadd(0.2454855, 0.96940029, 0, 1) }",
    "soma { pt3dadd(0.54694819, 0.83716649, 0, 1) }",
    "soma { pt3dadd(0.78914052, 0.61421269, 0, 1) }",
    "soma { pt3dadd(0.94581723, 0.32469946, 0, 1) }",
    "soma { pt3dadd(1, 0, 0, 1) }",
    "soma connect axon[0](0), 0.5",
    "axon[0] { pt3dadd(0, 0, 0, 2) }",
    "axon[0] { pt3dadd(0, -4, 0, 2) }",
    "axon[0] connect axon[1](0), 1",
    "axon[1] { pt3dadd(0, -4, 0, 2) }",
    "axon[1] { pt3dadd(6, -4, 0, 4) }",
    "axon[0] connect axon[2](0), 1",
    "axon[2] { pt3dadd(0, -4, 0, 2) }",
    "axon[2] { pt3dadd(-5, -4, 0, 4) }",
    "soma connect dend[0](0), 0.5",
    "dend[0] { pt3dadd(0, 0, 0, 2) }",
    "dend[0] { pt3dadd(0, 5, 0, 2) }",
    "dend[0] connect dend[1](0), 1",
    "dend[1] { pt3dadd(0, 5, 0, 2) }",
    "dend[1] { pt3dadd(-5, 5, 0, 3) }",
    "dend[0] connect dend[2](0), 1",
    "dend[2] { pt3dadd(0, 5, 0, 2) }",
    "dend[2] { pt3dadd(6, 5, 0, 3) }"};

const std::vector<std::string> soma_single_point_h5_section_index_to_name{"soma",
                                                                  "axon[0]",
                                                                  "axon[1]",
                                                                  "axon[2]",
                                                                  "dend[0]",
                                                                  "dend[1]",
                                                                  "dend[2]"};

const std::vector<std::tuple<int, int, int>> soma_single_point_h5_section_type_distribution{
    {1, -1, 1},
    {2, 1, 3},
    {3, 2, 3},
};

}  // namespace nrn::test