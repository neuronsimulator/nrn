// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
// Exercises nrn_segment_mechanisms: a node's mechanisms in the order of its
// property list, the order Python's `for mech in seg` follows.
#include <array>
#include <cmath>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include "neuronapi.h"

using std::cerr;
using std::endl;

extern "C" void modl_reg(){/* No modl_reg */};

static bool check(bool cond, const char* msg) {
    if (!cond) {
        cerr << "FAIL: " << msg << endl;
    }
    return cond;
}

static std::vector<std::string> mechanisms(Section* sec, double x) {
    std::array<Symbol*, 16> syms{};
    int n = nrn_segment_mechanisms(sec, x, syms.data(), syms.size());
    std::vector<std::string> names;
    for (int i = 0; i < n && i < int(syms.size()); ++i) {
        names.emplace_back(nrn_symbol_name(syms[i]));
    }
    return names;
}

static std::string join(const std::vector<std::string>& names) {
    std::string out;
    for (const auto& name: names) {
        out += name + " ";
    }
    return out;
}

static bool check_order(Section* sec, double x, const std::vector<std::string>& expected) {
    auto got = mechanisms(sec, x);
    if (got != expected) {
        cerr << "FAIL: x=" << x << " got " << join(got) << "expected " << join(expected) << endl;
        return false;
    }
    return true;
}

int main(void) {
    static std::array<const char*, 4> argv = {"segment_mechanisms", "-nogui", "-nopython", nullptr};
    nrn_init(3, argv.data());

    bool ok = true;
    Section* soma = nrn_section_new("soma");

    // Each insert puts the mechanism at the head of every node's list; inserting
    // hh then inserts the ions it uses, which land in front of it.
    nrn_mechanism_insert(soma, nrn_symbol("pas"));
    nrn_mechanism_insert(soma, nrn_symbol("hh"));
    const std::vector<std::string> order = {
        "k_ion", "na_ion", "hh", "pas", "capacitance", "morphology"};
    ok &= check_order(soma, 0.5, order);

    // Count only, and a buffer smaller than the list.
    ok &= check(nrn_segment_mechanisms(soma, 0.5, nullptr, 0) == int(order.size()),
                "count with a NULL buffer");
    std::array<Symbol*, 2> two{};
    ok &= check(nrn_segment_mechanisms(soma, 0.5, two.data(), two.size()) == int(order.size()),
                "count past max");
    ok &= check(std::strcmp(nrn_symbol_name(two[1]), "na_ion") == 0, "first max entries filled");

    // After an nseg change the reused center node keeps its order, while new
    // nodes get the section's mechanisms in another order, as in Python.
    nrn_nseg_set(soma, 3);
    ok &= check_order(soma, 0.5, order);
    auto first = mechanisms(soma, 1.0 / 6);
    ok &= check(first.size() == order.size() && first != order,
                "a new node lists the same mechanisms in another order");

    ok &= check(nrn_segment_mechanisms(nullptr, 0.5, nullptr, 0) == -1, "NULL section");

    // x outside [0, 1], including NaN, is rejected as nrn_segment_nmodlrandom_get does;
    // it is not clamped to an end node (NaN used to index past the node array).
    for (double bad: std::array<double, 6>{-0.5, 1.5, 1e300, INFINITY, -INFINITY, NAN}) {
        ok &= check(nrn_segment_mechanisms(soma, bad, nullptr, 0) == -1, "x out of range");
    }

    // The end nodes x=0 and x=1 of a root section carry no mechanisms.
    ok &= check(nrn_segment_mechanisms(soma, 0.0, nullptr, 0) == 0, "x=0 has no mechanisms");
    ok &= check(nrn_segment_mechanisms(soma, 1.0, nullptr, 0) == 0, "x=1 has no mechanisms");

    // Point processes are in the node's list, at the head if inserted last.
    nrn_hoc_call(
        "create dend\n"
        "dend { insert pas nseg = 3 }\n"
        "objref ic\n"
        "dend ic = new IClamp(0.5)\n"
        "access dend\n");
    Section* dend = nrn_cas();
    ok &= check_order(dend, 0.5, {"IClamp", "pas", "capacitance", "morphology"});
    ok &= check_order(dend, 1.0 / 6, {"morphology", "capacitance", "pas"});

    // x=0 of a child section is the node of its parent, with that node's mechanisms.
    nrn_hoc_call(
        "create kid\n"
        "connect kid(0), dend(0.5)\n"
        "access kid\n");
    Section* kid = nrn_cas();
    ok &= check_order(kid, 0.0, {"IClamp", "pas", "capacitance", "morphology"});
    ok &= check(nrn_segment_mechanisms(kid, 1.0, nullptr, 0) == 0, "x=1 of a leaf has none");

    // A deleted section.
    nrn_hoc_call(
        "create gone\n"
        "access gone\n");
    Section* gone = nrn_cas();
    nrn_hoc_call("gone { delete_section() }\n");
    ok &= check(nrn_segment_mechanisms(gone, 0.5, nullptr, 0) == -1, "deleted section");
    return ok ? 0 : 1;
}
