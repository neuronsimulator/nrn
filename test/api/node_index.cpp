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

    // Exact node indices for this topology, matching Python's seg.node_index().
    // soma's nodes are 0 (0-end), 1 (center), 2 (1-end, shared with dend's
    // 0-end at the connection); dend's three interior segments are nodes 3-5.
    ok &= check(nrn_segment_node_index(soma, 0.5) == 1, "soma(0.5) is node 1");
    ok &= check(nrn_segment_node_index(dend, 1.0 / 6.0) == 3, "dend(1/6) is node 3");
    ok &= check(nrn_segment_node_index(dend, 3.0 / 6.0) == 4, "dend(3/6) is node 4");
    ok &= check(nrn_segment_node_index(dend, 5.0 / 6.0) == 5, "dend(5/6) is node 5");

    // contract: a null section yields -1 rather than crashing.
    ok &= check(nrn_segment_node_index(nullptr, 0.5) == -1, "null section returns -1");

    return ok ? 0 : 1;
}
