// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
// Exercises nrn_sectionlist_to_array, the batched snapshot of a section list.
// It walks a section list (nrn_allsec() or nrn_sectionlist_data(obj)) in a
// single crossing, skipping semi-deleted sections, where the older iterator
// crosses once per section. The array must agree with the iterator
// element-for-element; a NULL/zero-length call returns the total (the count
// pass); and a buffer shorter than the list must truncate while still reporting
// the true total.
#include <array>
#include <iostream>
#include <vector>
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

// Gather a section list with the existing one-at-a-time iterator, for comparison
// against the batched array.
static std::vector<Section*> gather_with_iterator(nrn_Item* sl) {
    std::vector<Section*> out;
    SectionListIterator* it = nrn_sectionlist_iterator_new(sl);
    while (!nrn_sectionlist_iterator_done(it)) {
        Section* sec = nrn_sectionlist_iterator_next(it);
        if (sec) {
            out.push_back(sec);
        }
    }
    nrn_sectionlist_iterator_free(it);
    return out;
}

// Compare the batched array against the iterator's result for one list.
static bool array_matches_iterator(nrn_Item* sl, const char* what) {
    bool ok = true;
    std::vector<Section*> expected = gather_with_iterator(sl);
    // The count pass: buf = NULL, maxlen = 0 returns the total without writing.
    int count = nrn_sectionlist_to_array(sl, nullptr, 0);
    ok &= check(count == static_cast<int>(expected.size()), what);
    if (count != static_cast<int>(expected.size())) {
        cerr << "  " << what << ": count " << count << " != iterator " << expected.size() << endl;
    }

    std::vector<Section*> buf(count ? count : 1, nullptr);
    int total = nrn_sectionlist_to_array(sl, buf.data(), count);
    ok &= check(total == count,
                "to_array total equals the count pass when the buffer is large enough");
    for (int i = 0; i < count; ++i) {
        if (buf[i] != expected[i]) {
            ok = false;
            cerr << "  " << what << ": array[" << i << "] != iterator[" << i << "]" << endl;
        }
    }
    return ok;
}

int main(void) {
    static std::array<const char*, 4> argv = {"sectionlist_to_array",
                                              "-nogui",
                                              "-nopython",
                                              nullptr};
    nrn_init(3, argv.data());

    bool ok = true;

    // topology: five sections in a small tree
    Section* soma = nrn_section_new("soma");
    Section* dend1 = nrn_section_new("dend1");
    Section* dend2 = nrn_section_new("dend2");
    Section* dend3 = nrn_section_new("dend3");
    Section* axon = nrn_section_new("axon");
    nrn_section_connect(dend1, 0, soma, 1);
    nrn_section_connect(dend2, 0, dend1, 1);
    nrn_section_connect(dend3, 0, dend1, 1);
    nrn_section_connect(axon, 0, soma, 0);

    // allsec: every section created above. The array must agree with the
    // iterator, and the count pass must report 5.
    ok &= check(nrn_sectionlist_to_array(nrn_allsec(), nullptr, 0) == 5,
                "allsec count pass reports all five sections");
    ok &= array_matches_iterator(nrn_allsec(), "allsec array agrees with allsec iterator");

    // A SectionList object: dend1's subtree (dend1, dend2, dend3).
    Object* seclist = nrn_object_new(nrn_symbol("SectionList"), 0);
    nrn_section_push(dend1);
    nrn_method_call(seclist, nrn_method_symbol(seclist, "subtree"), 0);
    nrn_section_pop();
    ok &= check(nrn_sectionlist_to_array(nrn_sectionlist_data(seclist), nullptr, 0) == 3,
                "subtree SectionList count pass reports three sections");
    ok &= array_matches_iterator(nrn_sectionlist_data(seclist),
                                 "SectionList array agrees with SectionList iterator");

    // Truncation: a buffer shorter than the list fills only maxlen entries but
    // still returns the true total, so a caller can detect it undersized.
    std::array<Section*, 5> tbuf;
    tbuf.fill(reinterpret_cast<Section*>(-1));  // sentinel to detect writes
    int total = nrn_sectionlist_to_array(nrn_allsec(), tbuf.data(), 2);
    ok &= check(total == 5, "to_array returns the full total even when the buffer is too small");
    ok &= check(tbuf[0] != reinterpret_cast<Section*>(-1) &&
                    tbuf[1] != reinterpret_cast<Section*>(-1),
                "to_array fills the first maxlen entries");
    ok &= check(tbuf[2] == reinterpret_cast<Section*>(-1), "to_array does not write past maxlen");

    // Null safety: the call does not crash or miscount on a null list.
    ok &= check(nrn_sectionlist_to_array(nullptr, nullptr, 0) == 0,
                "count pass of a null list is zero");
    ok &= check(nrn_sectionlist_to_array(nullptr, tbuf.data(), 5) == 0,
                "to_array of a null list is zero");

    return ok ? 0 : 1;
}
