// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
// Section owner hooks from the C API and NEURON's own Python module in one
// process. Each reads every owner as its own object, so they must not own
// sections at the same time: no section is created for the module's hooks,
// and they can be replaced only when no section is owned. (Importing the
// module while C API hooks are installed is tested in
// test/unit_tests/hoc_python/test_section_owner_capi.py.)
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

static int owner_token;
static char name[] = "capi_cell.soma";
static int name_calls;
static char* section_name(Section*) {
    ++name_calls;
    return name;
}
static bool cell_is_list;  // the cell hook returns a new HOC List, not a Python object
static Object* section_cell(Section*) {
    if (!cell_is_list) {
        return nullptr;
    }
    Object* cell = nrn_object_new(nrn_symbol("List"), 0);
    nrn_object_ref(cell);
    return cell;
}
static int section_cell_equals(Section*, Object*) {
    return 0;
}

// nrnpython() returns 1 if the statement ran without a Python exception.
static double run_python(const char* statement) {
    char err[256]{};
    char* arg = const_cast<char*>(statement);
    nrn_str_push(&arg);
    if (nrn_function_call_nothrow(nrn_symbol("nrnpython"), 1, err, sizeof(err)) != 0) {
        cerr << "nrnpython: " << err << endl;
        return -1.0;
    }
    return nrn_double_pop();
}

int main(void) {
    // Without -nopython, NEURON starts its Python module, which installs its hooks.
    static std::array<const char*, 3> argv = {"section_owner_python", "-nogui", nullptr};
    nrn_init(2, argv.data());

    bool ok = true;
    char err[256]{};
    ok &= check(run_python("import neuron") == 1.0, "import neuron");
    ok &= check(!nrn_section_owner_hooks_set(
                    section_name, section_cell, section_cell_equals, err, sizeof(err)) &&
                    std::strstr(err, "already installed") != nullptr,
                "hooks refused while the Python module's are installed");
    ok &= check(nrn_section_new_owned(&owner_token) == nullptr,
                "no owned section with the Python module's hooks");

    // With no Python section, the module's hooks can be cleared and replaced,
    // as an interface that replaces the module does under nrniv -python.
    ok &= check(run_python("s = neuron.h.Section(name='pysoma')") == 1.0, "a Python section");
    ok &= check(!nrn_section_owner_hooks_clear(err, sizeof(err)) &&
                    std::strstr(err, "owned sections exist") != nullptr,
                "clear refused while a Python section exists");
    ok &= check(run_python("del s") == 1.0, "delete the Python section");
    ok &= check(nrn_section_owner_hooks_clear(err, sizeof(err)), "clear the module's hooks");
    ok &= check(nrn_section_owner_hooks_set(
                    section_name, section_cell, section_cell_equals, err, sizeof(err)),
                "install hooks after the clear");
    Section* owned = nrn_section_new_owned(&owner_token);
    ok &= check(owned && std::strcmp(nrn_secname(owned), "capi_cell.soma") == 0,
                "owned section after the takeover");

    // The module stays loaded but must not make or wrap owned sections while
    // our hooks are installed: its sections would be named by our hook, and it
    // would read our owner as a Python Section.
    name_calls = 0;
    ok &= check(run_python("s = neuron.h.Section(name='late')") == 0.0,
                "the module refuses to create a section");
    ok &= check(name_calls == 0, "no section named by our hook");
    ok &= check(run_python("[str(x) for x in neuron.h.allsec()]") == 0.0,
                "the module refuses to wrap our section in allsec");
    nrn_section_push(owned);
    ok &= check(run_python("neuron.h.cas()") == 0.0,
                "the module refuses to wrap our section in cas");
    nrn_section_pop();
    ok &= check(std::strcmp(nrn_secname(owned), "capi_cell.soma") == 0, "our section is untouched");

    // With the module still loaded as the Python provider, BBSaveState must not
    // hand it a cell that is a plain HOC object (the module asserts it gets a
    // PythonObject); the save stops with a HOC error instead.
    cell_is_list = true;
    nrn_hoc_call(
        "objref pc_bbss, nc_bbss, nil_bbss\n"
        "pc_bbss = new ParallelContext()\n"
        "pc_bbss.set_gid2node(7, pc_bbss.id())\n"
        "_pysec.capi_cell.soma nc_bbss = new NetCon(&v(0.5), nil_bbss)\n"
        "pc_bbss.cell(7, nc_bbss)\n"
        "finitialize(-65)\n");
    Object* bbss = nrn_object_new(nrn_symbol("BBSaveState"), 0);
    nrn_object_ref(bbss);
    ok &= check(nrn_method_call_nothrow(
                    bbss, nrn_method_symbol(bbss, "save_test"), 0, err, sizeof(err)) != 0 &&
                    std::strstr(err, "only as part of a Python cell") != nullptr,
                "BBSaveState refuses a HOC cell with the Python provider installed");
    nrn_object_unref(bbss);
    nrn_hoc_call("pc_bbss.gid_clear()\nobjref nc_bbss, pc_bbss\n");
    cell_is_list = false;

    nrn_section_owned_release(owned);
    return ok ? 0 : 1;
}
