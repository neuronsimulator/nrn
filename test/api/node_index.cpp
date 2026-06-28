// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
// Exercises nrn_segment_node_index, the C-API equivalent of Python's
// seg.node_index() (index of (sec, x)'s node in NEURON's internal node array).
#include <array>
#include <cstring>
#include <iostream>
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

int main(void) {
    static std::array<const char*, 4> argv = {"node_index", "-nogui", "-nopython", nullptr};
    nrn_init(3, argv.data());

    // soma <- dend (3 segments), so the dend owns three distinct interior nodes.
    Section* soma = nrn_section_new("soma");
    Section* dend = nrn_section_new("dend");
    nrn_section_connect(dend, 0, soma, 1);
    nrn_nseg_set(dend, 3);

    // node indices become canonical once the tree is set up.
    nrn_double_push(-65);
    nrn_function_call(nrn_symbol("finitialize"), 1);
    nrn_double_pop();

    bool ok = true;

    int i_soma = nrn_segment_node_index(soma, 0.5);
    int i_dend = nrn_segment_node_index(dend, 0.5);
    ok &= check(i_soma >= 0, "soma(0.5) index is non-negative");
    ok &= check(i_dend >= 0, "dend(0.5) index is non-negative");
    ok &= check(i_soma != i_dend, "soma and dend nodes are distinct");

    // the call is stable across repeated queries.
    ok &= check(i_soma == nrn_segment_node_index(soma, 0.5), "soma index is stable");

    // dend's three interior segments occupy three distinct nodes.
    int a = nrn_segment_node_index(dend, 1.0 / 6.0);
    int b = nrn_segment_node_index(dend, 3.0 / 6.0);
    int c = nrn_segment_node_index(dend, 5.0 / 6.0);
    ok &= check(a != b && b != c && a != c, "dend interior segments are distinct nodes");

    // contract: a null section yields -1 rather than crashing.
    ok &= check(nrn_segment_node_index(nullptr, 0.5) == -1, "null section returns -1");

    return ok ? 0 : 1;
}
