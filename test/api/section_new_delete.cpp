// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
// Deleting a section made by nrn_section_new frees what nrn_section_new
// allocated for it: its symbol, the symbol's name, its section item and its
// slot in HOC's top-level data. Creating and deleting sections then uses no
// memory, as HOC's create and delete_section of one name do.
#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
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

// As Python's delete_section(sec=sec): push, delete_section, then HOC's
// pop_section, which (unlike nrn_section_pop) accepts a deleted section.
static void delete_section(Section* sec) {
    nrn_section_push(sec);
    nrn_function_call(nrn_symbol("delete_section"), 0);
    nrn_double_pop();
    nrn_function_call(nrn_symbol("pop_section"), 0);
    nrn_double_pop();
}

// Two sections, deleted in the order they were made.
static void cycle() {
    static std::string const a(40, 'a'), b(40, 'b');
    Section* sa = nrn_section_new(a.c_str());
    Section* sb = nrn_section_new(b.c_str());
    delete_section(sa);
    delete_section(sb);
}

// nrn_mallinfo(0) is the heap in use where malloc has mallinfo. It is 0 under
// ASan and without mallinfo, and the resident set size on macOS.
static double heap_in_use() {
    nrn_double_push(0);
    nrn_function_call(nrn_symbol("nrn_mallinfo"), 1);
    return nrn_double_pop();
}

// Whether heap_in_use counts bytes: it grows by at least a 4 KB block while the
// block is allocated, and is back where it was once the block is freed.
static bool heap_in_use_is_exact() {
    static void* volatile block;
    double const before = heap_in_use();
    block = std::malloc(4096);
    double const with_block = heap_in_use();
    std::free(block);
    return with_block - before >= 4096 && heap_in_use() == before;
}

int main(void) {
    static std::array<const char*, 4> argv = {"section_new_delete", "-nogui", "-nopython", nullptr};
    nrn_init(3, argv.data());

    bool ok = true;

    // A section deleted while something holds a reference to it, then a new one.
    Section* held = nrn_section_new("held");
    nrn_section_ref(held);
    delete_section(held);
    Section* later = nrn_section_new("later");
    ok &= check(!nrn_section_is_active(held), "the deleted section is not active");
    ok &= check(std::strcmp(nrn_secname(held), "") == 0, "the deleted section has no name");
    ok &= check(std::strcmp(nrn_secname(later), "later") == 0, "the new section has its name");
    nrn_section_unref(held);
    delete_section(later);

    // HOC keeps the symbol of a section it declared, so the name can be created again.
    ok &= check(nrn_hoc_call("create hocsec\n"
                             "hocsec delete_section()\n"
                             "create hocsec\n"
                             "hocsec.L = 7\n"
                             "hoc_ac_ = hocsec.L\n") == 0,
                "HOC creates a section again after deleting it");
    ok &= check(*nrn_symbol_dataptr(nrn_symbol("hoc_ac_")) == 7, "and can use it");

    bool const exact = heap_in_use_is_exact();
    // NEURON's containers settle in the first cycle (on glibc); 50 is margin.
    for (int i = 0; i < 50; ++i) {
        cycle();
    }
    double const before = heap_in_use();
    // 500 cycles x 2 sections.
    for (int i = 0; i < 500; ++i) {
        cycle();
    }
    double const growth = heap_in_use() - before;
    if (!exact) {
        cerr << "SKIP: nrn_mallinfo gives no heap size on this platform/runtime" << endl;
    } else {
        ok &= check(growth == 0, "1000 sections created and deleted use no memory");
        if (growth != 0) {
            cerr << "the heap grew by " << growth << " bytes" << endl;
        }
    }

    return ok ? 0 : 1;
}
