// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
// Exercises the section-tree accessors nrn_section_parent, nrn_section_trueparent,
// nrn_section_child and nrn_section_sibling, which read NEURON's connectivity
// directly (what a HOC SectionRef's parent/trueparent/child yields) without
// constructing a SectionRef object. Builds a known tree and checks each edge,
// including a case where the true parent differs from the direct parent.
#include <array>
#include <iostream>
#include <set>
#include "neuronapi.h"

using std::cerr;
using std::endl;

extern "C" void modl_reg() {}

static bool check(bool cond, const char* msg) {
    if (!cond) {
        cerr << "FAIL: " << msg << endl;
    }
    return cond;
}

// Collect a section's children by walking child -> sibling.
static std::set<Section*> children_of(Section* sec) {
    std::set<Section*> out;
    for (Section* c = nrn_section_child(sec); c; c = nrn_section_sibling(c)) {
        out.insert(c);
    }
    return out;
}

int main(void) {
    static std::array<const char*, 4> argv = {"section_tree", "-nogui", "-nopython", nullptr};
    nrn_init(3, argv.data());

    bool ok = true;

    // Topology:
    //   soma (root)
    //   dend1 : 0 -> soma(1)     (connected away from soma's 0 end)
    //   dend2 : 0 -> dend1(1)
    //   dend3 : 0 -> dend1(1)
    //   axon  : 0 -> soma(0)     (connected AT soma's 0 end)
    Section* soma = nrn_section_new("soma");
    Section* dend1 = nrn_section_new("dend1");
    Section* dend2 = nrn_section_new("dend2");
    Section* dend3 = nrn_section_new("dend3");
    Section* axon = nrn_section_new("axon");
    nrn_section_connect(dend1, 0, soma, 1);
    nrn_section_connect(dend2, 0, dend1, 1);
    nrn_section_connect(dend3, 0, dend1, 1);
    nrn_section_connect(axon, 0, soma, 0);

    // parent: the section each is directly connected to; NULL for the root.
    ok &= check(nrn_section_parent(soma) == nullptr, "root has no parent");
    ok &= check(nrn_section_parent(dend1) == soma, "dend1's parent is soma");
    ok &= check(nrn_section_parent(dend2) == dend1, "dend2's parent is dend1");
    ok &= check(nrn_section_parent(dend3) == dend1, "dend3's parent is dend1");
    ok &= check(nrn_section_parent(axon) == soma, "axon's parent is soma");

    // trueparent: normally the parent, but a section joined at its parent's 0
    // end (axon -> soma(0), and soma is the root) has no true parent -- the
    // relationship climbs past soma to the root's absent parent. This is the
    // case where trueparent differs from parent.
    ok &= check(nrn_section_trueparent(soma) == nullptr, "root has no true parent");
    ok &= check(nrn_section_trueparent(dend1) == soma, "dend1's true parent is soma");
    ok &= check(nrn_section_trueparent(dend2) == dend1, "dend2's true parent is dend1");
    ok &= check(nrn_section_parent(axon) == soma && nrn_section_trueparent(axon) == nullptr,
                "axon's parent is soma but it has no true parent (joined at soma's 0 end)");

    // child / sibling: iterating child -> sibling yields exactly the children.
    ok &= check(children_of(soma) == (std::set<Section*>{dend1, axon}),
                "soma's children are dend1 and axon");
    ok &= check(children_of(dend1) == (std::set<Section*>{dend2, dend3}),
                "dend1's children are dend2 and dend3");
    ok &= check(nrn_section_child(dend2) == nullptr, "dend2 is a leaf (no child)");
    ok &= check(nrn_section_child(axon) == nullptr, "axon is a leaf (no child)");

    // Null safety: none of the accessors crash on a null section.
    ok &= check(nrn_section_parent(nullptr) == nullptr, "parent(NULL) is NULL");
    ok &= check(nrn_section_trueparent(nullptr) == nullptr, "trueparent(NULL) is NULL");
    ok &= check(nrn_section_child(nullptr) == nullptr, "child(NULL) is NULL");
    ok &= check(nrn_section_sibling(nullptr) == nullptr, "sibling(NULL) is NULL");

    return ok ? 0 : 1;
}
