// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
// A C API client (for example a Python or MATLAB interface) creates sections owned by its own
// objects, the way NEURON's Python module does (nrnpy_newsection): the name
// and cell come from the client's hooks, and the section is found through
// _pysec and section_owner(). The hooks here are plain C functions.
#include <array>
#include <cstring>
#include <iostream>
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

static int owner_token;  // stands in for the interface's Section object
static Section* owned;   // the section it owns
static Object* cell;     // the cell that owns the section
static char name[] = "cellA.soma";

// Called only for owned sections, including while one is being created;
// must not return NULL.
static char* section_name(Section*) {
    return name;
}

// Called for every section; returns a new reference for the owned one only.
static Object* section_cell(Section* sec) {
    if (sec != owned) {
        return nullptr;
    }
    nrn_object_ref(cell);
    return cell;
}

static int section_cell_equals(Section* sec, Object* obj) {
    return sec == owned && obj == cell;
}

static char other[] = "other";
static char* other_name(Section*) {
    return other;
}

static double section_count(bool& ok) {
    char err[256]{};
    ok &= check(nrn_function_call_nothrow(nrn_symbol("section_count"), 0, err, sizeof(err)) == 0,
                "section_count call succeeds");
    return nrn_double_pop();
}

int main(void) {
    static std::array<const char*, 4> argv = {"section_owner", "-nogui", "-nopython", nullptr};
    nrn_init(3, argv.data());

    bool ok = true;
    char err[256]{};
    nrn_hoc_call(
        "func section_count() { local n  n = 0  forall n += 1  return n }\n"
        "pysec_L = 0\n");
    cell = nrn_object_new(nrn_symbol("List"), 0);

    ok &= check(nrn_section_new_owned(&owner_token) == nullptr,
                "no owned section before the hooks are installed");
    ok &= check(nrn_section_owner_hooks_set(
                    section_name, section_cell, section_cell_equals, err, sizeof(err)),
                "install hooks");
    ok &= check(nrn_section_owner_hooks_set(
                    section_name, section_cell, section_cell_equals, err, sizeof(err)),
                "installing the same hooks again is allowed");
    ok &= check(!nrn_section_owner_hooks_set(
                    other_name, section_cell, section_cell_equals, err, sizeof(err)) &&
                    std::strstr(err, "already installed") != nullptr,
                "another provider's hooks are refused");

    owned = nrn_section_new_owned(&owner_token);
    ok &= check(owned != nullptr, "create an owned section");
    ok &= check(std::strcmp(nrn_secname(owned), "cellA.soma") == 0, "name comes from the hook");
    ok &= check(section_count(ok) == 1.0, "section is in the section list");

    // section_owner() reaches the cell through the cell hook.
    nrn_section_push(owned);
    ok &= check(nrn_function_call_nothrow(nrn_symbol("section_owner"), 0, err, sizeof(err)) == 0,
                "section_owner call succeeds");
    Object* owner = nrn_object_pop();
    ok &= check(owner == cell, "section_owner returns the owning cell");
    nrn_object_unref(owner);
    nrn_section_pop();

    // HOC finds the section by its _pysec name.
    nrn_hoc_call("_pysec.cellA.soma { pysec_L = L }\n");
    ok &= check(*nrn_symbol_dataptr(nrn_symbol("pysec_L")) == 100.0, "_pysec name lookup");

    // The owner going away frees the section.
    nrn_section_owned_release(owned);
    owned = nullptr;
    ok &= check(section_count(ok) == 0.0, "release frees the section");

    // Deleted from HOC first: release only drops the reference HOC kept.
    owned = nrn_section_new_owned(&owner_token);
    nrn_section_push(owned);
    ok &= check(nrn_function_call_nothrow(nrn_symbol("delete_section"), 0, err, sizeof(err)) == 0,
                "delete_section from HOC");
    nrn_double_pop();
    // nrn_section_pop rejects a deleted section; HOC's pop_section does not.
    ok &= check(nrn_function_call_nothrow(nrn_symbol("pop_section"), 0, err, sizeof(err)) == 0,
                "pop the deleted section");
    nrn_double_pop();
    ok &= check(section_count(ok) == 0.0, "HOC delete removes it from the section list");
    ok &= check(!nrn_section_is_active(owned), "deleted section is inactive");
    nrn_section_owned_release(owned);

    nrn_object_unref(cell);
    return ok ? 0 : 1;
}
