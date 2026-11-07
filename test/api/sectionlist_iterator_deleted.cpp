// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
// nrn_sectionlist_iterator_next on a SectionList holding deleted sections: it
// skips them and drops them from the list, wherever in the list they are. The
// list's items come from malloc and each holds a reference on its section, so a
// sanitizer or valgrind run of this test also checks how the item and the
// reference are released.
#include <array>
#include <iostream>
#include <vector>
#include "neuronapi.h"
#include "section.h"

using std::cerr;
using std::endl;

extern "C" void modl_reg() {}

static bool check(bool cond, const char* msg) {
    if (!cond) {
        cerr << "FAIL: " << msg << endl;
    }
    return cond;
}

static std::vector<Section*> iterate(nrn_Item* sl) {
    std::vector<Section*> out;
    SectionListIterator* it = nrn_sectionlist_iterator_new(sl);
    while (!nrn_sectionlist_iterator_done(it)) {
        if (Section* sec = nrn_sectionlist_iterator_next(it)) {
            out.push_back(sec);
        }
    }
    nrn_sectionlist_iterator_free(it);
    return out;
}

int main(void) {
    static std::array<const char*, 4> argv = {"sectionlist_iterator_deleted",
                                              "-nogui",
                                              "-nopython",
                                              nullptr};
    nrn_init(3, argv.data());

    nrn_hoc_call(
        "create a, b, c, d, e\n"
        "objref sl\n"
        "sl = new SectionList()\n"
        "forall sl.append()\n");
    nrn_Item* sl = nrn_sectionlist_data(nrn_symbol_object_get(nrn_symbol("sl")));

    bool ok = true;
    std::vector<Section*> const all = iterate(sl);
    ok &= check(all.size() == 5, "all five live");
    if (!ok) {
        return 1;
    }
    Section* const b = all[1];
    Section* const d = all[3];

    // The first, a middle and the last entry.
    nrn_hoc_call(
        "a delete_section()\n"
        "c delete_section()\n"
        "e delete_section()\n");
    ok &= check(iterate(sl) == std::vector<Section*>{b, d}, "deleted sections are skipped");
    ok &= check(iterate(sl) == std::vector<Section*>{b, d}, "the live ones stay in the list");

    // Nothing live left. The list gives up its reference to each section it drops.
    nrn_section_ref(d);
    nrn_hoc_call(
        "b delete_section()\n"
        "d delete_section()\n");
    int const refcount = d->refcount;
    ok &= check(iterate(sl).empty(), "a list of deleted sections yields none");
    ok &= check(d->refcount == refcount - 1, "the list unrefs a section it drops");
    nrn_section_unref(d);
    ok &= check(iterate(sl).empty(), "and is empty afterwards");

    return ok ? 0 : 1;
}
