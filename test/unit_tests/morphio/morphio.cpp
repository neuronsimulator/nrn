#include <catch2/catch_test_macros.hpp>

#include "code.h"
#include "hocdec.h"
#include "ocfunc.h"

#include <sstream>
#include <fstream>
#include <filesystem>
#include <fmt/format.h>
#include "morphio.h"
#include <morphology/morphio_wrapper/morphio_wrapper.hpp>
#include <morphology/morphology.hpp>

namespace {
std::filesystem::path find_file(const std::filesystem::path& path) {
    if (std::filesystem::exists(path)) {
        return path;
    }
    std::filesystem::path current = std::filesystem::current_path();
    while (true) {
        if (auto candidate = current / path; std::filesystem::exists(candidate)) {
            return candidate;
        }
        if (!current.has_parent_path() || current == current.parent_path()) {
            break;
        }
        current = current.parent_path();
    }
    return path;
}

size_t count_soma_points(const std::vector<std::string>& cmds) {
    size_t soma_pts = 0;
    for (const auto& c: cmds) {
        if (c.rfind("soma { pt3dadd(", 0) == 0) {
            ++soma_pts;
        }
    }
    return soma_pts;
}
}  // namespace

TEST_CASE("HocBasicCell", "[NEURON][MorphIO]") {
    WHEN("We load the HocBasicCell template") {
        REQUIRE(hoc_oc(nrn::test::hoc_basic_cell_template) == 0);
    }
}

TEST_CASE("MorphIO Single Point SOMA", "[NEURON][MorphIO][SinglePointSoma]") {
    WHEN("We want to load a single point soma") {
        REQUIRE(hoc_oc("objref cell0, nullcell\n"
                       "cell0 = new HocBasicCell()") == 0);
        REQUIRE(hoc_oc("morphio_load(cell0, \"nonexistent_file.h5\")") == 1);
        REQUIRE(hoc_oc("morphio_load(cell0, "
                       "\"test/morphology/"
                       "soma_single_point.h5\")") == 0);
        REQUIRE(hoc_oc("morphio_load(nullcell, "
                       "\"test/morphology/"
                       "soma_single_point.h5\")") == 1);
        REQUIRE(hoc_oc("topology()") == 0);
    }
}


TEST_CASE("MorphIOWrapper", "[NEURON][MorphIO][MorphIOWrapper][SinglePointSoma]") {
    WHEN("We create a MorphIOWrapper on a non-existent file") {
        REQUIRE_THROWS(neuron::morphology::MorphIOWrapper{"nonexistent_file.h5"});
    }

    WHEN("We create a MorphIOWrapper") {
        neuron::morphology::MorphIOWrapper morph{"test/morphology/soma_single_point.h5"};

        THEN("We check the HOC commands match the expected output") {
            REQUIRE(morph.to_hoc() == nrn::test::soma_single_point_h5_hoc);
        }

        THEN("We check the section names match expected ones") {
            REQUIRE(morph.section_names() == nrn::test::soma_single_point_h5_section_names);
        }

        THEN("We check the section distribution matches expected one") {
            REQUIRE(morph.section_type_distribution() ==
                    nrn::test::soma_single_point_h5_section_type_distribution);
        }
    }
}

TEST_CASE("MorphIO Contour Soma Geometry", "[NEURON][MorphIO][ContourSoma]") {
    WHEN("We compute contourcenter and contour2centroid on the 59-point soma contour") {
        auto [mean, c_xyz] = neuron::morphology::contourcenter(nrn::test::contour_soma_points);
        REQUIRE(c_xyz.size() == 101);
        auto [new_xyz, new_diams] = neuron::morphology::contour2centroid(mean, c_xyz);
        REQUIRE(new_xyz.size() == 21);
        REQUIRE(new_diams.size() == 21);

        std::vector<std::string> cmds;
        for (size_t i = 0; i < new_xyz.size(); ++i) {
            const auto& p = new_xyz[i];
            const auto& d = new_diams[i];
            cmds.emplace_back("soma { pt3dadd(" + fmt::format("{:.8g}", p[0]) + ", " +
                              fmt::format("{:.8g}", p[1]) + ", " + fmt::format("{:.8g}", p[2]) +
                              ", " + fmt::format("{:.8g}", d) + ") }");
        }
        REQUIRE(cmds == nrn::test::contour_soma_expected_hoc);
    }

    WHEN("We pass degenerate contours to contourcenter and contour2centroid") {
        auto [empty_mean, empty_pts] = neuron::morphology::contourcenter({});
        REQUIRE(empty_pts.empty());
        REQUIRE_THROWS(neuron::morphology::contour2centroid({0.0, 0.0, 0.0}, {}));
        REQUIRE_THROWS(neuron::morphology::contour2centroid(
            {0.0, 0.0, 0.0}, {{{0.0, 0.0, 0.0}}, {{1.0, 0.0, 0.0}}, {{2.0, 0.0, 0.0}}}));
    }

    WHEN("We load test/morphology/test.h5 with a simple contour soma") {
        neuron::morphology::MorphIOWrapper morph{"test/morphology/test.h5"};
        const auto cmds = morph.to_hoc();
        REQUIRE(!cmds.empty());
        REQUIRE(morph.section_names().front() == "soma");
        REQUIRE(hoc_oc("objref cell_contour\n"
                       "cell_contour = new HocBasicCell()\n"
                       "morphio_load(cell_contour, \"test/morphology/test.h5\", 1)") == 0);
        REQUIRE(hoc_oc("morphio_load()") == 1);
        REQUIRE(hoc_oc("morphio_load(cell_contour)") == 1);
        std::filesystem::remove("test/morphology/test.hoc");
    }

    WHEN("We compute contourcenter and contour2centroid on the 26-point soma contour from a.txt") {
        auto [mean, c_xyz] = neuron::morphology::contourcenter(nrn::test::contour_soma_26_points);
        REQUIRE(c_xyz.size() == 101);
        auto [new_xyz, new_diams] = neuron::morphology::contour2centroid(mean, c_xyz);
        REQUIRE(new_xyz.size() == 21);
        REQUIRE(new_diams.size() == 21);
        for (size_t i = 0; i < 21; ++i) {
            REQUIRE(new_diams[i] > 0.0);
        }
    }
}

TEST_CASE("MorphIO Three Point SOMA SWC", "[NEURON][MorphIO][ThreePointSoma]") {
    WHEN("We load a morphology with NeuroMorpho 3-point cylinder soma") {
        const auto c91662_path =
            find_file("share/lib/python/neuron/rxdtests/tests/c91662.swc").string();
        neuron::morphology::MorphIOWrapper morph{c91662_path};
        const auto cmds = morph.to_hoc();
        REQUIRE(!cmds.empty());
        REQUIRE(morph.section_names().front() == "soma");
        // NeuroMorpho 3-point cylinder soma emits 3 points along X with radius = d0/2
        size_t soma_pts = 0;
        for (const auto& c: cmds) {
            if (c.rfind("soma { pt3dadd(", 0) == 0) {
                ++soma_pts;
            }
        }
        REQUIRE(soma_pts == 3);

        const std::string hoc_cmd = fmt::format(
            "objref cell_c91662\ncell_c91662 = new HocBasicCell()\nmorphio_load(cell_c91662, "
            "\"{}\")",
            c91662_path);
        REQUIRE(hoc_oc(hoc_cmd.c_str()) == 0);
    }
}

TEST_CASE("MorphIO Cylinder SOMA and Heterogeneous Neurites", "[NEURON][MorphIO][Cylinders]") {
    WHEN("We load a morphology with cylinder / frustum soma") {
        const auto frustum_swc =
            find_file("external/MorphIO/src/morphio-external/tests/data/soma_single_frustum.swc")
                .string();
        if (std::filesystem::exists(frustum_swc)) {
            neuron::morphology::MorphIOWrapper morph{frustum_swc};
            REQUIRE(count_soma_points(morph.to_hoc()) == 2);
        }
    }

    WHEN("We load a morphology with heterogeneous non-contiguous neurite types") {
        const auto heter_swc =
            find_file(
                "external/MorphIO/src/morphio-external/tests/data/simple-heterogeneous-neurite.swc")
                .string();
        if (std::filesystem::exists(heter_swc)) {
            neuron::morphology::MorphIOWrapper morph{heter_swc};
            const auto& dist = morph.section_type_distribution();
            // Should contain exactly 1 entry for each unique section type (soma, axon, dend)
            REQUIRE(dist.size() == 3);
            const auto& names = morph.section_names();
            // Verify names are unique
            std::vector<std::string> sorted_names = names;
            std::sort(sorted_names.begin(), sorted_names.end());
            REQUIRE(std::adjacent_find(sorted_names.begin(), sorted_names.end()) ==
                    sorted_names.end());
        }
    }
}

TEST_CASE("MorphIO Nomenclature Mapping", "[NEURON][MorphIO][Nomenclature]") {
    using namespace neuron::morphology;

    SECTION("type2name maps standard and custom types") {
        REQUIRE(type2name(1) == "soma");
        REQUIRE(type2name(2) == "axon");
        REQUIRE(type2name(3) == "dend");
        REQUIRE(type2name(4) == "apic");
        REQUIRE(type2name(5) == "dend_5");
        REQUIRE(type2name(6) == "dend_6");
        REQUIRE(type2name(10) == "dend_10");
        REQUIRE(type2name(-1) == "minus_1");
        REQUIRE(type2name(-2) == "minus_2");
    }

    SECTION("mksubset maps subsets to SectionLists") {
        REQUIRE(mksubset(1, 1, "soma") == "forsec \"soma\" somatic.append");
        REQUIRE(mksubset(2, 5, "axon") == "forsec \"axon\" axonal.append");
        REQUIRE(mksubset(3, 10, "dend") == "forsec \"dend\" basal.append");
        REQUIRE(mksubset(4, 2, "apic") == "forsec \"apic\" apical.append");
        REQUIRE(mksubset(5, 3, "dend_5") == "forsec \"dend_5\" dendritic_5.append");
        REQUIRE(mksubset(-1, 1, "minus_1") == "forsec \"minus_1\" minus_1set.append");
    }

    SECTION("name formats indexed section names") {
        REQUIRE(name(1, 0) == "soma[0]");
        REQUIRE(name(2, 4) == "axon[4]");
        REQUIRE(name(3, 1) == "dend[1]");
        REQUIRE(name(4, 0) == "apic[0]");
        REQUIRE(name(5, 2) == "dend_5[2]");
        REQUIRE(name(-1, 0) == "minus_1[0]");
    }
}

TEST_CASE("MorphIO Four Standard Section Types SWC", "[NEURON][MorphIO][SWC]") {
    const auto swc_path =
        find_file("external/MorphIO/src/morphio-external/tests/data/three_point_soma.swc").string();
    if (std::filesystem::exists(swc_path)) {
        neuron::morphology::MorphIOWrapper morph{swc_path};
        const auto& names = morph.section_names();
        REQUIRE(names.size() == 5);
        REQUIRE(names[0] == "soma");
        REQUIRE(names[1] == "axon[0]");
        REQUIRE(names[2] == "axon[1]");
        REQUIRE(names[3] == "dend[0]");
        REQUIRE(names[4] == "apic[0]");

        const std::string hoc_cmd = fmt::format(
            "objref cell_3pt\ncell_3pt = new HocBasicCell()\nmorphio_load(cell_3pt, \"{}\")",
            swc_path);
        REQUIRE(hoc_oc(hoc_cmd.c_str()) == 0);
    }
}

TEST_CASE("MorphIO Pia ASC Contour Soma and Axon", "[NEURON][MorphIO][ASC]") {
    if (const auto pia_path = find_file("external/MorphIO/src/morphio-external/tests/data/pia.asc");
        std::filesystem::exists(pia_path)) {
        neuron::morphology::MorphIOWrapper morph{pia_path.string()};
        const auto& names = morph.section_names();
        REQUIRE(names.size() == 2);
        REQUIRE(names[0] == "soma");
        REQUIRE(names[1] == "axon[0]");

        const std::string hoc_cmd = fmt::format(
            "objref cell_pia\ncell_pia = new HocBasicCell()\nmorphio_load(cell_pia, \"{}\")",
            pia_path.string());
        REQUIRE(hoc_oc(hoc_cmd.c_str()) == 0);
    }
}

TEST_CASE("MorphIO HDF5 Large Morphologies", "[NEURON][MorphIO][H5]") {
    if (const auto neuron_h5 = find_file(
            "external/MorphIO/src/morphio-external/tests/data/h5/v1/Neuron.h5");
        std::filesystem::exists(neuron_h5)) {
        neuron::morphology::MorphIOWrapper morph{neuron_h5.string()};
        REQUIRE(morph.section_names().size() == 85);
    }

    if (const auto astrocyte_h5 = find_file(
            "external/MorphIO/src/morphio-external/tests/data/astrocyte.h5");
        std::filesystem::exists(astrocyte_h5)) {
        neuron::morphology::MorphIOWrapper morph{astrocyte_h5.string()};
        REQUIRE(morph.section_names().size() == 1316);
    }
}

TEST_CASE("MorphIO Multi-Contour SOMA Rejection", "[NEURON][MorphIO][ErrorHandling]") {
    if (const auto stack_asc = find_file("test/hoctests/tests/test_pt3dstyle_stack.asc");
        std::filesystem::exists(stack_asc)) {
        REQUIRE_THROWS(neuron::morphology::MorphIOWrapper{stack_asc.string()});
    }
}

TEST_CASE("MorphIO Edge Cases and Error Handling", "[NEURON][MorphIO][EdgeCases]") {
    SECTION("type2name and mksubset with custom or negative type IDs") {
        CHECK(neuron::morphology::type2name(-1) == "minus_1");
        CHECK(neuron::morphology::type2name(-2) == "minus_2");
        CHECK(neuron::morphology::type2name(5) == "dend_5");
        CHECK(neuron::morphology::type2name(10) == "dend_10");

        CHECK(neuron::morphology::mksubset(-1, 0, "minus_1") ==
              "forsec \"minus_1\" minus_1set.append");
        CHECK(neuron::morphology::mksubset(5, 0, "dend_5") ==
              "forsec \"dend_5\" dendritic_5.append");
    }

    SECTION("contourcenter empty input") {
        auto [mean, pts] = neuron::morphology::contourcenter({});
        CHECK(pts.empty());
        CHECK(mean[0] == 0.0);
        CHECK(mean[1] == 0.0);
        CHECK(mean[2] == 0.0);
    }

    SECTION("contour2centroid degenerate / collinear contours throw") {
        using neuron::morphology::Point3D;
        Point3D mean{0.0, 0.0, 0.0};
        // Too few points (< 3)
        CHECK_THROWS_AS(neuron::morphology::contour2centroid(mean, {}), std::runtime_error);
        CHECK_THROWS_AS(neuron::morphology::contour2centroid(mean,
                                                             {Point3D{1, 0, 0}, Point3D{2, 0, 0}}),
                        std::runtime_error);

        // Collinear points along X axis (minor axis is zero -> ratio < 1e-6)
        std::vector<Point3D> collinear = {Point3D{-1, 0, 0}, Point3D{0, 0, 0}, Point3D{1, 0, 0}};
        CHECK_THROWS_AS(neuron::morphology::contour2centroid(mean, collinear), std::runtime_error);

        // Three non-collinear points (< 4 points in convex hull)
        std::vector<Point3D> three_pts = {Point3D{0, 0, 0}, Point3D{1, 0, 0}, Point3D{0, 1, 0}};
        CHECK_THROWS_AS(neuron::morphology::contour2centroid(mean, three_pts), std::runtime_error);
    }

    SECTION("MorphIOWrapper move constructor and assignment") {
        auto fname = find_file("test/morphology/soma_single_point.h5").string();
        neuron::morphology::MorphIOWrapper m1(fname);
        CHECK(!m1.to_hoc().empty());
        neuron::morphology::MorphIOWrapper m2(std::move(m1));
        CHECK(!m2.to_hoc().empty());
        neuron::morphology::MorphIOWrapper m3(fname);
        m3 = std::move(m2);
        CHECK(!m3.to_hoc().empty());
    }

    SECTION("find_file direct and non-existent search paths") {
        auto current = std::filesystem::current_path();
        CHECK(find_file(current) == current);
        std::filesystem::path non_existent("/path_that_does_not_exist/test.h5");
        CHECK(find_file(non_existent) == non_existent);
    }

    SECTION("HOC morphio_load invalid argument counts") {
        REQUIRE(hoc_oc("morphio_load()") == 1);
        REQUIRE(hoc_oc("objref c_err\n c_err = new HocBasicCell()\n morphio_load(c_err)") == 1);
    }

    SECTION("HOC morphio_load failure in cell context") {
        REQUIRE(hoc_oc("begintemplate BrokenHocCell\nendtemplate BrokenHocCell\n"
                       "objref broken_cell\nbroken_cell = new BrokenHocCell()") == 0);
        auto fname = find_file("test/morphology/soma_single_point.h5").string();
        std::string cmd = fmt::format("morphio_load(broken_cell, \"{}\")", fname);
        REQUIRE(hoc_oc(cmd.c_str()) == 1);
    }

    SECTION("HOC morphio_load with optional 3rd argument debug export") {
        std::filesystem::path tmp_swc = std::filesystem::current_path() / "test_debug_dump.swc";
        std::filesystem::path tmp_hoc = std::filesystem::current_path() / "test_debug_dump.hoc";
        {
            std::ofstream ofs(tmp_swc);
            ofs << "1 1 0 0 0 1 -1\n";
        }
        std::string cmd = fmt::format(
            "objref cell_dump\ncell_dump = new HocBasicCell()\nmorphio_load(cell_dump, \"{}\", 1)",
            tmp_swc.string());
        REQUIRE(hoc_oc(cmd.c_str()) == 0);
        CHECK(std::filesystem::exists(tmp_hoc));
        std::filesystem::remove(tmp_swc);
        std::filesystem::remove(tmp_hoc);
    }
}
