// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
// nrn_property_* on objects of a HOC template (begintemplate), including
// non-public members, and nrn_property_object_get/set for objref members.
#include <array>
#include <cmath>
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

// Read a HOC expression through a nothrow call, so the test sees what HOC sees.
static double hoc_value(const char* func, bool& ok) {
    char err[256]{};
    ok &= check(nrn_function_call_nothrow(nrn_symbol(func), 0, err, sizeof(err)) == 0, func);
    return nrn_double_pop();
}

int main(void) {
    static std::array<const char*, 4> argv = {"template_property", "-nogui", "-nopython", nullptr};
    nrn_init(3, argv.data());

    bool ok = true;
    nrn_hoc_call(
        "begintemplate Cell\n"
        "public x, arr, items\n"
        "objref items, hidden_ref\n"
        "double arr[3]\n"
        "proc init() { x = 4  arr[2] = 7  hidden = 9  items = new List()  hidden_ref = new "
        "Vector(2) }\n"
        "endtemplate Cell\n"
        "objref cell\n"
        "cell = new Cell()\n"
        "func cell_x() { return cell.x }\n"
        "func cell_arr1() { return cell.arr[1] }\n"
        "func items_is_nil() { return object_id(cell.items) == 0 }\n");
    Object* cell = nrn_symbol_object_get(nrn_symbol("cell"));
    ok &= check(cell != nullptr, "cell exists");

    // Doubles: scalar, array element, non-public member.
    ok &= check(nrn_property_get(cell, "x") == 4.0, "get public scalar");
    nrn_property_set(cell, "x", 5.0);
    ok &= check(hoc_value("cell_x", ok) == 5.0, "set public scalar is seen by HOC");
    ok &= check(nrn_property_array_get(cell, "arr", 2) == 7.0, "get array element");
    nrn_property_array_set(cell, "arr", 1, 3.0);
    ok &= check(hoc_value("cell_arr1", ok) == 3.0, "set array element is seen by HOC");
    ok &= check(nrn_property_get(cell, "hidden") == 9.0, "get non-public scalar");

    // Pointers to members.
    nrn_property_push(cell, "x");
    ok &= check(*nrn_double_ptr_pop() == 5.0, "push scalar pointer");
    nrn_property_array_push(cell, "arr", 2);
    ok &= check(*nrn_double_ptr_pop() == 7.0, "push array element pointer");
    ok &= check(nrn_property_data_handle_is_valid(cell, "arr", 2), "valid in-range element");
    ok &= check(!nrn_property_data_handle_is_valid(cell, "arr", 3), "invalid out-of-range element");

    // Names that are not double members read NaN and are not written.
    ok &= check(std::isnan(nrn_property_get(cell, "missing")), "missing member reads NaN");
    ok &= check(std::isnan(nrn_property_get(cell, "items")), "objref member reads NaN as a double");
    ok &= check(std::isnan(nrn_property_array_get(cell, "arr", 3)),
                "out-of-range element reads NaN");
    nrn_property_set(cell, "missing", 1.0);
    ok &= check(!nrn_property_data_handle_is_valid(cell, "missing", 0),
                "missing member is invalid");

    // Objref members, public or not.
    Object* items = nrn_property_object_get(cell, "items");
    ok &= check(items && std::strcmp(nrn_class_name(items), "List") == 0, "get public objref");
    Object* hidden = nrn_property_object_get(cell, "hidden_ref");
    ok &= check(hidden && std::strcmp(nrn_class_name(hidden), "Vector") == 0,
                "get non-public objref");
    ok &= check(nrn_property_object_get(cell, "x") == nullptr, "double member is not an objref");

    nrn_double_push(3);
    Object* vec = nrn_object_new(nrn_symbol("Vector"), 1);
    ok &= check(nrn_property_object_set(cell, "items", vec), "set objref");
    nrn_object_unref(vec);  // the member now holds the only reference
    Object* got = nrn_property_object_get(cell, "items");
    ok &= check(got == vec && std::strcmp(nrn_class_name(got), "Vector") == 0,
                "set objref is read back");
    ok &= check(nrn_property_object_set(cell, "items", nullptr), "clear objref");
    ok &= check(nrn_property_object_get(cell, "items") == nullptr, "cleared objref reads NULL");
    ok &= check(hoc_value("items_is_nil", ok) == 1.0, "cleared objref is nil in HOC");
    ok &= check(!nrn_property_object_set(cell, "x", nullptr), "cannot set a double as an objref");

    // Built-in classes have no HOC dataspace and no objref members.
    nrn_double_push(2);
    Object* builtin = nrn_object_new(nrn_symbol("Vector"), 1);
    ok &= check(nrn_property_object_get(builtin, "x") == nullptr, "built-in has no objref member");
    ok &= check(!nrn_property_object_set(builtin, "x", nullptr), "cannot set built-in objref");
    nrn_object_unref(builtin);

    return ok ? 0 : 1;
}
