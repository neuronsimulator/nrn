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

    // Setting a member to an object kept alive only by the member's old value.
    // HOC assignment retains the new object before releasing the old one; the
    // setter must too, or the new object is freed before it is bound.
    nrn_hoc_call(
        "begintemplate Holder\n"
        "public m\n"
        "objref m\n"
        "proc init() { m = new Vector(6) }\n"
        "endtemplate Holder\n"
        "func vector_count() { localobj l  l = new List(\"Vector\")  return l.count() }\n"
        "func items_size() { return cell.items.size() }\n"
        "cell.items = new Vector(4)\n");
    const double vectors = hoc_value("vector_count", ok);
    Object* sole = nrn_property_object_get(cell, "items");
    ok &= check(nrn_property_object_set(cell, "items", sole), "set objref to its own value");
    ok &= check(nrn_property_object_get(cell, "items") == sole, "self-assignment keeps the member");
    ok &= check(hoc_value("vector_count", ok) == vectors, "self-assignment frees no Vector");
    ok &= check(hoc_value("items_size", ok) == 4.0, "self-assigned Vector is intact");

    // The new object is a member of the object being replaced.
    nrn_hoc_call("cell.items = new Holder()\n");
    Object* holder = nrn_property_object_get(cell, "items");
    Object* member = nrn_property_object_get(holder, "m");  // Holder has the only reference
    ok &= check(nrn_property_object_set(cell, "items", member),
                "set objref to the old value's member");
    ok &= check(nrn_property_object_get(cell, "items") == member, "member is bound");
    ok &= check(hoc_value("vector_count", ok) == vectors, "the old value's member is not freed");
    ok &= check(hoc_value("items_size", ok) == 6.0, "the old value's member is intact");
    ok &= check(nrn_property_object_set(cell, "items", nullptr), "clear objref again");

    // The member's old value holds the last reference to the object being
    // assigned to (obj.m = obj, no other reference). The object must stay alive
    // until the new value is stored, then be freed along with it, as HOC does for
    // `mk().m = x`.
    nrn_hoc_call(
        "begintemplate SelfRef\n"
        "public m\n"
        "objref m\n"
        "endtemplate SelfRef\n"
        "func selfref_count() { localobj l  l = new List(\"SelfRef\")  return l.count() }\n");
    Object* self_ref = nrn_object_new(nrn_symbol("SelfRef"), 0);
    ok &= check(nrn_property_object_set(self_ref, "m", self_ref), "set a member to its owner");
    nrn_object_unref(self_ref);  // only its own member keeps it alive now
    ok &= check(hoc_value("selfref_count", ok) == 1.0, "the cycle keeps the object alive");
    const double vectors_before = hoc_value("vector_count", ok);
    nrn_double_push(8);
    Object* replacement = nrn_object_new(nrn_symbol("Vector"), 1);
    ok &= check(nrn_property_object_set(self_ref, "m", replacement),
                "set the member that holds its owner's last reference");
    ok &= check(hoc_value("selfref_count", ok) == 0.0, "the owner is freed after the assignment");
    nrn_object_unref(replacement);  // the freed owner released its reference
    ok &= check(hoc_value("vector_count", ok) == vectors_before,
                "the owner's new member is released");

    // A template's unref() callback sees the same calls and counts as for the
    // HOC assignment: assigning a member does not ref or unref the owner.
    nrn_hoc_call(
        "objref ulog, nil, hoc_side, api_side\n"
        "ulog = new Vector()\n"
        "begintemplate Logged\n"
        "public m\n"
        "objref m\n"
        "external ulog\n"
        "proc unref() { ulog.append($1) }\n"
        "endtemplate Logged\n"
        "func logged_count() { localobj l  l = new List(\"Logged\")  return l.count() }\n"
        "func ulog_size() { return ulog.size() }\n"
        "func ulog_0() { return ulog.x[0] }\n"
        "hoc_side = new Logged()\n"
        "api_side = new Logged()\n");
    Object* api_side = nrn_symbol_object_get(nrn_symbol("api_side"));
    nrn_hoc_call("{ ulog.resize(0) }\nhoc_side.m = new Vector()\n");
    ok &= check(hoc_value("ulog_size", ok) == 0.0, "HOC member assignment: no unref() call");
    nrn_hoc_call("{ ulog.resize(0) }\n");
    nrn_double_push(1);
    Object* fresh = nrn_object_new(nrn_symbol("Vector"), 1);
    ok &= check(nrn_property_object_set(api_side, "m", fresh), "set a member of a Logged");
    nrn_object_unref(fresh);
    ok &= check(hoc_value("ulog_size", ok) == 0.0, "API member assignment: no unref() call");

    // Assigning a member its current value is a no-op, as `a.m = a.m` is.
    nrn_hoc_call(
        "{ ulog.resize(0) }\nhoc_side.m = api_side\n{ ulog.resize(0) }\nhoc_side.m = hoc_side.m\n");
    ok &= check(hoc_value("ulog_size", ok) == 0.0, "HOC a.m = a.m: no unref() call");
    nrn_property_object_set(api_side, "m", api_side);
    nrn_hoc_call("{ ulog.resize(0) }\n");
    ok &= check(nrn_property_object_set(api_side, "m", nrn_property_object_get(api_side, "m")),
                "set a member to its current value");
    ok &= check(hoc_value("ulog_size", ok) == 0.0, "API self-assignment: no unref() call");

    // The member holds its owner (a.m = a): releasing it calls unref(1) once.
    nrn_hoc_call("hoc_side.m = hoc_side\n{ ulog.resize(0) }\nhoc_side.m = nil\n");
    ok &= check(hoc_value("ulog_size", ok) == 1.0 && hoc_value("ulog_0", ok) == 1.0,
                "HOC releasing a.m == a calls unref(1)");
    nrn_hoc_call("{ ulog.resize(0) }\n");
    ok &= check(nrn_property_object_set(api_side, "m", nullptr),
                "clear a member holding its owner");
    ok &= check(hoc_value("ulog_size", ok) == 1.0 && hoc_value("ulog_0", ok) == 1.0,
                "API releasing a.m == a calls unref(1)");

    // The member holds its owner's last reference: one unref(0), then the owner
    // is freed.
    nrn_property_object_set(api_side, "m", api_side);
    nrn_hoc_call("api_side = nil\n");  // only the cycle is left
    ok &= check(hoc_value("logged_count", ok) == 2.0, "the cycle keeps the Logged alive");
    nrn_hoc_call("{ ulog.resize(0) }\n");
    ok &= check(nrn_property_object_set(api_side, "m", nullptr), "release the last reference");
    ok &= check(hoc_value("ulog_size", ok) == 1.0 && hoc_value("ulog_0", ok) == 0.0,
                "releasing the last reference calls unref(0)");
    ok &= check(hoc_value("logged_count", ok) == 1.0, "the Logged is freed");

    // Built-in classes have no HOC dataspace and no objref members.
    nrn_double_push(2);
    Object* builtin = nrn_object_new(nrn_symbol("Vector"), 1);
    ok &= check(nrn_property_object_get(builtin, "x") == nullptr, "built-in has no objref member");
    ok &= check(!nrn_property_object_set(builtin, "x", nullptr), "cannot set built-in objref");
    nrn_object_unref(builtin);

    return ok ? 0 : 1;
}
