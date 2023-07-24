#include <catch2/catch.hpp>

#include "code.h"
#include "hocdec.h"
#include "hoc_membf.h"
#include "ocfunc.h"

#include <sstream>
#include "morphio.h"
#include <morphology/morphio_wrapper/morphio_wrapper.hpp>
#include <morphology/morphology.hpp>

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
            REQUIRE(morph.morph_as_hoc() == nrn::test::soma_single_point_h5_hoc);
        }

        THEN("We check the section index to names match expected ones") {
            REQUIRE(morph.sec_idx2names() == nrn::test::soma_single_point_h5_sec_idx2names);
        }

        THEN("We check the section distribution matches expected one") {
            REQUIRE(morph.sec_typeid_distrib() ==
                    nrn::test::soma_single_point_h5_sec_typeid_distrib);
        }
    }
}