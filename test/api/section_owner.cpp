// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
// A C API client (for example a Python or MATLAB interface) creates sections owned by its own
// objects, the way NEURON's Python module does (nrnpy_newsection): the name
// and cell come from the client's hooks, and the section is found through
// _pysec and section_owner(). The hooks here are plain C functions.
#include <array>
#include <cstdio>
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

// Stands in for the interface's Section object. The hooks reach it from the
// Section* with nrn_section_owner_get, including while it is being created.
struct Owner {
    const char* name;            // nullptr: unnamed
    Object* cell;                // nullptr: no cell
    bool new_cell_each = false;  // the cell hook makes a new object each call, as Python's does
};

static Owner owner_a{"cellA.soma", nullptr};
static int name_calls;
static int foreign_owner_calls;  // a hook saw an owner that is not an Owner of ours
static char name_buffer[64];

static Owner* owner_of(Section* sec) {
    auto* owner = static_cast<Owner*>(nrn_section_owner_get(sec));
    return owner;
}

// Called only for owned sections, including while one is being created and when
// it is released; must not return NULL, and must return the same name each time.
static char* section_name(Section* sec) {
    ++name_calls;
    Owner* owner = owner_of(sec);
    if (!owner) {
        ++foreign_owner_calls;
        std::snprintf(name_buffer, sizeof(name_buffer), "__nrnsec_%p", (void*) sec);
    } else if (owner->name) {
        std::snprintf(name_buffer, sizeof(name_buffer), "%s", owner->name);
    } else {
        // Unnamed: core recognises this form and keeps it out of the _pysec lookup.
        std::snprintf(name_buffer, sizeof(name_buffer), "__nrnsec_%p", (void*) sec);
    }
    return name_buffer;
}

// Called for every section; returns a new reference for an owned one with a cell.
static Object* section_cell(Section* sec) {
    Owner* owner = owner_of(sec);
    if (owner && owner->new_cell_each) {
        Object* cell = nrn_object_new(nrn_symbol("List"), 0);
        nrn_object_ref(cell);
        return cell;
    }
    if (!owner || !owner->cell) {
        return nullptr;
    }
    nrn_object_ref(owner->cell);
    return owner->cell;
}

static int cell_equals_calls;
static int section_cell_equals(Section* sec, Object* obj) {
    ++cell_equals_calls;
    Owner* owner = owner_of(sec);
    return owner && owner->cell && owner->cell == obj;
}

static char other[] = "other.soma";
static char* other_name(Section*) {
    return other;
}

static double call_double(const char* func, bool& ok) {
    char err[256]{};
    ok &= check(nrn_function_call_nothrow(nrn_symbol(func), 0, err, sizeof(err)) == 0, func);
    return nrn_double_pop();
}

static double hoc_value(const char* name) {
    return *nrn_symbol_dataptr(nrn_symbol(name));
}

int main(void) {
    static std::array<const char*, 4> argv = {"section_owner", "-nogui", "-nopython", nullptr};
    nrn_init(3, argv.data());

    bool ok = true;
    char err[256]{};
    nrn_hoc_call(
        "func section_count() { local n  n = 0  forall n += 1  return n }\n"
        "pysec_L = 0\n"
        "objref nil, nc, ncl\n");
    owner_a.cell = nrn_object_new(nrn_symbol("List"), 0);

    ok &= check(nrn_section_new_owned(&owner_a) == nullptr,
                "no owned section before the hooks are installed");
    ok &= check(!nrn_section_owner_hooks_set(
                    section_name, nullptr, section_cell_equals, err, sizeof(err)) &&
                    std::strstr(err, "required") != nullptr,
                "a NULL hook is refused");
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
    ok &= check(nrn_section_new_owned(nullptr) == nullptr, "a NULL owner is refused");

    Section* owned = nrn_section_new_owned(&owner_a);
    ok &= check(owned != nullptr, "create an owned section");
    ok &= check(nrn_section_owner_get(owned) == &owner_a, "nrn_section_owner_get");
    ok &= check(std::strcmp(nrn_secname(owned), "cellA.soma") == 0, "name comes from the hook");
    ok &= check(call_double("section_count", ok) == 1.0, "section is in the section list");

    // Sections NEURON owns itself have no owner.
    Section* plain = nrn_section_new("plain");
    ok &= check(nrn_section_owner_get(plain) == nullptr, "a HOC section has no owner");
    ok &= check(nrn_section_owner_get(nullptr) == nullptr, "NULL section has no owner");

    // section_owner() reaches the cell through the cell hook.
    nrn_section_push(owned);
    ok &= check(nrn_function_call_nothrow(nrn_symbol("section_owner"), 0, err, sizeof(err)) == 0,
                "section_owner call succeeds");
    Object* cell = nrn_object_pop();
    ok &= check(cell == owner_a.cell, "section_owner returns the owning cell");
    nrn_object_unref(cell);
    nrn_section_pop();

    // HOC finds the section by its _pysec name.
    nrn_hoc_call("_pysec.cellA.soma { pysec_L = L }\n");
    ok &= check(hoc_value("pysec_L") == 100.0, "_pysec name lookup");

    // NetCon.precell uses the cell hook, precelllist the cell_equals hook.
    cell_equals_calls = 0;
    nrn_hoc_call(
        "objref precell\n"
        "_pysec.cellA.soma nc = new NetCon(&v(.5), nil)\n"
        "precell = nc.precell()\n"
        "ncl = nc.precelllist()\n"
        "ncl_count = ncl.count()\n"
        "objref nc, ncl\n");
    ok &= check(nrn_symbol_object_get(nrn_symbol("precell")) == owner_a.cell,
                "NetCon.precell is the cell");
    nrn_hoc_call("objref precell\n");
    ok &= check(hoc_value("ncl_count") == 1.0, "NetCon.precelllist finds the NetCon");
    ok &= check(cell_equals_calls == 1, "precelllist asks cell_equals once");

    // cvode.statename finds the state through the hook-provided name.
    nrn_hoc_call(
        "objref cv\ncv = new CVode()\n_pysec.cellA.soma insert hh\n"
        "cv.active(1)\nfinitialize(-65)\nstrdef sname\ncv.statename(0, sname, 0)\n"
        "cv.active(0)\n");
    ok &= check(std::strcmp(nrn_symbol_str_get(nrn_symbol("sname")), "cellA.soma.v(0.5)") == 0,
                "cvode.statename names the owned section");

    // The clear refuses while an owned section exists.
    ok &= check(!nrn_section_owner_hooks_clear(err, sizeof(err)) &&
                    std::strstr(err, "owned sections exist") != nullptr,
                "clear is refused while an owned section exists");

    // The owner going away frees the section; release asks the name hook once more.
    name_calls = 0;
    nrn_section_owned_release(owned);
    ok &= check(name_calls == 1, "release asks the name hook for the _pysec entry");
    owned = nullptr;
    ok &= check(call_double("section_count", ok) == 1.0, "release frees the section");
    nrn_hoc_call("pysec_L = 0\n_pysec.cellA.soma { pysec_L = L }\n");
    ok &= check(hoc_value("pysec_L") == 0.0, "release removes the _pysec name");

    // A section without a name or a cell.
    Owner unnamed{nullptr, nullptr};
    owned = nrn_section_new_owned(&unnamed);
    ok &= check(std::strncmp(nrn_secname(owned), "__nrnsec_0x", 11) == 0, "unnamed section name");
    nrn_section_push(owned);
    nrn_function_call_nothrow(nrn_symbol("section_owner"), 0, err, sizeof(err));
    cell = nrn_object_pop();
    ok &= check(cell == nullptr, "section_owner of a section without a cell is nil");
    nrn_section_pop();
    nrn_section_owned_release(owned);

    // Once HOC has used the _pysec lookup, the name hook runs inside
    // nrn_section_new_owned, before the caller has the Section*; it finds its
    // owner with nrn_section_owner_get.
    name_calls = 0;
    owned = nrn_section_new_owned(&owner_a);
    ok &= check(name_calls == 1, "the name hook runs during creation");
    ok &= check(foreign_owner_calls == 0, "the name hook found its owner during creation");

    // Deleted from HOC first: release only drops the reference HOC kept.
    nrn_section_push(owned);
    ok &= check(nrn_function_call_nothrow(nrn_symbol("delete_section"), 0, err, sizeof(err)) == 0,
                "delete_section from HOC");
    nrn_double_pop();
    // nrn_section_pop rejects a deleted section; HOC's pop_section does not.
    ok &= check(nrn_function_call_nothrow(nrn_symbol("pop_section"), 0, err, sizeof(err)) == 0,
                "pop the deleted section");
    nrn_double_pop();
    ok &= check(call_double("section_count", ok) == 1.0,
                "HOC delete removes it from the section list");
    ok &= check(!nrn_section_is_active(owned), "deleted section is inactive");
    ok &= check(nrn_section_owner_get(owned) == nullptr, "a deleted section has no owner");
    // Its _pysec name went with it: the lookup must not reach a freed section.
    nrn_hoc_call("pysec_L = 0\n_pysec.cellA.soma { pysec_L = L }\n");
    ok &= check(hoc_value("pysec_L") == 0.0, "_pysec name removed by delete_section");
    nrn_section_owned_release(owned);

    // BBSaveState saves an owned section only as part of a Python cell; a cell
    // that is a plain HOC object stops the save with an error. BBSaveState drops
    // a reference to what the cell hook returns, so that hook makes a new object
    // each time.
    Owner owner_b{"cellB.soma", nullptr, true};
    owned = nrn_section_new_owned(&owner_b);
    nrn_hoc_call(
        "objref pc_bbss, nc_bbss\n"
        "pc_bbss = new ParallelContext()\n"
        "pc_bbss.set_gid2node(7, pc_bbss.id())\n"
        "_pysec.cellB.soma nc_bbss = new NetCon(&v(0.5), nil)\n"
        "pc_bbss.cell(7, nc_bbss)\n"
        "finitialize(-65)\n");
    // nrn_object_new returns a reference this test owns.
    Object* bbss = nrn_object_new(nrn_symbol("BBSaveState"), 0);
    ok &= check(nrn_method_call_nothrow(
                    bbss, nrn_method_symbol(bbss, "save_test"), 0, err, sizeof(err)) != 0 &&
                    std::strstr(err, "only as part of a Python cell") != nullptr,
                "BBSaveState refuses a cell that is not a Python object");
    nrn_object_unref(bbss);
    nrn_hoc_call("pc_bbss.gid_clear()\nobjref nc_bbss, pc_bbss\n");
    nrn_section_owned_release(owned);

    // With no owned section left the hooks can be cleared, and then replaced.
    ok &= check(nrn_section_owner_hooks_clear(err, sizeof(err)), "clear the hooks");
    ok &= check(nrn_section_new_owned(&owner_a) == nullptr, "no owned section after the clear");
    ok &= check(nrn_section_owner_hooks_set(
                    other_name, section_cell, section_cell_equals, err, sizeof(err)),
                "install other hooks after the clear");
    owned = nrn_section_new_owned(&owner_a);
    ok &= check(owned && std::strcmp(nrn_secname(owned), "other.soma") == 0,
                "the new hooks name the section");
    nrn_section_owned_release(owned);

    nrn_object_unref(owner_a.cell);
    return ok ? 0 : 1;
}
