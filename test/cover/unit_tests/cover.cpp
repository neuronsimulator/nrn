#include <../../nrnconf.h>
#include "oc_ansi.h"

extern bool hoc_valid_stmt(const char*, Object*);

#include <catch2/catch_test_macros.hpp>

// Help cover PR's

// #2675 NrnProperty handles extracellular and no longer creates/wraps a Prop structure.
// 8 missing lines in Codecov Report
// Difficult to cover with the interpreter as it requires manual mouse
// interaction with a SymbolChooser or "Plot what?" of Graph.
#include "symdir.h"
TEST_CASE("Cover #2675 ivoc/symdir.cpp", "[SymDirectory]") {
    REQUIRE(hoc_valid_stmt(
                ""
                "// Create a POINT_PROCESS sufficient to cover SymDirectoryImpl::load_mechanism\n"
                "begintemplate Mech1\n"
                "  public a, b, c\n"
                "  double b[3], c[8]\n"
                "  a = 0\n"
                "endtemplate Mech1\n"
                "make_mechanism(\"mech1\", \"Mech1\")\n"
                "create soma\n"
                "insert mech1\n"
                "objref ic\n"
                "ic = new IClamp(.5) // just to cover one line.\n",
                nullptr) == 1);
    std::string parent_path{""};
    auto* somasym = hoc_lookup("soma");
    REQUIRE(somasym);
    auto symdir = new SymDirectory(parent_path, nullptr, somasym, 0, 0);
    for (int i = 0; i < symdir->count(); ++i) {
        printf("%d %s\n", i, symdir->name(i).c_str());
    }
    REQUIRE(symdir->count() == 10);
    REQUIRE(symdir->name(4) == "c_mech1[all]( 0.5 )");
    delete symdir;
    REQUIRE(hoc_valid_stmt("objref ic\nsoma { delete_section() }", nullptr) == 1);
}
