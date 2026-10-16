// Verify that a runtime PythonObject provider receives component reads and
// writes even when NEURON itself is compiled without Python support. This test
// supplies minimal callbacks and exact values; it does not link to Python.
#include <array>
#include <cstring>
#include <iostream>

#include "neuronapi.h"

using std::cerr;
using std::endl;

extern "C" void modl_reg() {}

static int read_calls{};
static int write_calls{};
static bool read_frame_matches{};
static bool method_call_matches{};
static bool failing_method_call_matches{};
static bool write_frame_matches{};
static double assigned_value{};
static int read_kind{};
static int write_kind{};
static char provider_text[] = "provider text";
static char* provider_text_ptr = provider_text;
static bool assigned_object_matches{};

static bool check(bool cond, const char* msg) {
    if (!cond) {
        cerr << "FAIL: " << msg << endl;
    }
    return cond;
}

static bool fail_writes{};

static const char* read_component(Object* obj, Symbol* member, int nindex, int isfunc) {
    ++read_calls;
    read_kind = 0;
    const char* name = member ? nrn_symbol_name(member) : nullptr;
    if (name && std::strcmp(name, "missing") == 0) {
        return "provider read failed";
    }
    if (isfunc) {
        const double rhs = nrn_double_pop();
        const double lhs = nrn_double_pop();
        Object* frame_obj = nrn_object_pop();
        method_call_matches = frame_obj == obj && nindex == 2 && (isfunc & 1);
        if (frame_obj) {
            nrn_object_unref(frame_obj);
        }
        if (name && std::strcmp(name, "fail_method") == 0) {
            failing_method_call_matches = method_call_matches && lhs == 12.5 && rhs == 30.25;
            return "provider method failed";
        }
        nrn_double_push(lhs + rhs);
        return nullptr;
    }
    Object* frame_obj = nrn_object_pop();
    read_frame_matches = frame_obj == obj && member && nindex == 0 && isfunc == 0;
    if (name && std::strcmp(name, "text") == 0) {
        read_kind = 1;
    } else if (name && std::strcmp(name, "object") == 0) {
        read_kind = 2;
    } else if (name && std::strcmp(name, "nil") == 0) {
        read_kind = 3;
    }
    if (frame_obj) {
        nrn_object_unref(frame_obj);
    }
    if (read_kind == 1) {
        nrn_str_push(&provider_text_ptr);
    } else if (read_kind == 2) {
        nrn_object_push(obj);
    } else if (read_kind == 3) {
        nrn_object_push(nullptr);
    } else {
        nrn_double_push(42.5);
    }
    return nullptr;
}

static const char* write_component(Object* obj) {
    ++write_calls;
    if (fail_writes) {
        return "provider write failed";
    }
    auto rhs_type = nrn_stack_type();
    if (rhs_type == STACK_IS_NUM) {
        assigned_value = nrn_double_pop();
    } else if (rhs_type == STACK_IS_STR) {
        auto* value = nrn_str_pop();
        write_kind = value && *value && std::strcmp(*value, "assigned text") == 0;
    } else {
        Object* assigned_object = nrn_object_pop();
        assigned_object_matches = assigned_object == obj;
        if (assigned_object) {
            nrn_object_unref(assigned_object);
        }
        write_kind = assigned_object ? 2 : 3;
    }
    Object* frame_obj = nrn_object_pop();
    Symbol* member = nrn_symbol_pop();
    int nindex = nrn_int_pop();
    const char* member_name = member ? nrn_symbol_name(member) : nullptr;
    const bool known_member = member_name && (std::strcmp(member_name, "answer") == 0 ||
                                              std::strcmp(member_name, "text") == 0 ||
                                              std::strcmp(member_name, "object") == 0);
    write_frame_matches = frame_obj == obj && known_member && nindex == 0;
    if (frame_obj) {
        nrn_object_unref(frame_obj);
    }
    return nullptr;
}

int main(void) {
    static std::array<const char*, 4> argv = {"runtime_pythonobject_marshaling",
                                              "-nogui",
                                              "-nopython",
                                              nullptr};
    nrn_init(3, argv.data());

    bool ok = true;
    Symbol* pyobject = nrn_symbol("PythonObject");
    ok &= check(pyobject != nullptr, "stub PythonObject class is registered");

    char bad_error[128]{};
    nrn_double_push(9182.0);
    ok &= check(!nrn_template_set_component_hooks(
                    nullptr, read_component, write_component, bad_error, sizeof(bad_error)),
                "invalid component registration fails closed");
    ok &= check(bad_error[0] != '\0', "invalid component registration reports an error");
    ok &= check(nrn_double_pop() == 9182.0,
                "invalid component registration leaves the HOC stack untouched");

    // A host provider identifies its PythonObject template and installs the
    // two callbacks through the public opaque registration API.
    char error[128]{};
    ok &= check(nrn_template_set_component_hooks(
                    pyobject, read_component, write_component, error, sizeof(error)),
                "component hooks registered");

    Object* obj = nrn_object_new(pyobject, 0);
    ok &= check(obj != nullptr, "PythonObject constructed");
    ok &= check(nrn_hoc_call("func read_component() { return $o1.answer }") == 0,
                "read helper defined");
    ok &= check(nrn_hoc_call("func call_component() { return $o1.add($2, $3) }") == 0,
                "method-call helper defined");
    ok &= check(nrn_hoc_call("proc write_component() { $o1.answer = $2 }") == 0,
                "write helper defined");

    nrn_double_push(1001.0);
    nrn_object_push(obj);
    nrn_function_call(nrn_symbol("read_component"), 1);
    ok &= check(nrn_double_pop() == 42.5, "component read returns the provider's exact value");
    ok &= check(nrn_double_pop() == 1001.0, "numeric read preserves the lower stack sentinel");
    ok &= check(read_calls == 1, "component read callback runs exactly once");
    ok &= check(read_frame_matches, "component read callback receives the expected frame");

    nrn_double_push(1009.0);
    nrn_object_push(obj);
    nrn_double_push(12.5);
    nrn_double_push(30.25);
    nrn_function_call(nrn_symbol("call_component"), 3);
    ok &= check(nrn_double_pop() == 42.75, "component method call returns the exact sum");
    ok &= check(nrn_double_pop() == 1009.0,
                "component method call preserves the lower stack sentinel");
    ok &= check(method_call_matches, "component method callback receives call metadata");

    nrn_double_push(1002.0);
    nrn_object_push(obj);
    nrn_double_push(73.25);
    nrn_function_call(nrn_symbol("write_component"), 2);
    nrn_double_pop();
    ok &= check(nrn_double_pop() == 1002.0, "numeric write preserves the lower stack sentinel");
    ok &= check(assigned_value == 73.25, "component write receives the exact assigned value");
    ok &= check(write_calls == 1, "component write callback runs exactly once");
    ok &= check(write_frame_matches, "component write callback receives the expected frame");

    ok &= check(nrn_hoc_call("strdef read_text_result") == 0, "string result storage defined");
    ok &= check(nrn_hoc_call("proc read_text() { read_text_result = $o1.text }") == 0,
                "string read helper defined");
    nrn_double_push(1003.0);
    nrn_object_push(obj);
    nrn_function_call(nrn_symbol("read_text"), 1);
    ok &= check(nrn_double_pop() == 0.0, "string read procedure result is balanced");
    const char* text = nrn_symbol_str_get(nrn_symbol("read_text_result"));
    ok &= check(text && std::strcmp(text, "provider text") == 0,
                "component string read returns provider text");
    ok &= check(nrn_double_pop() == 1003.0, "string read preserves the lower stack sentinel");

    ok &= check(nrn_hoc_call("obfunc read_object() { return $o1.object }") == 0,
                "object read helper defined");
    nrn_double_push(1004.0);
    nrn_object_push(obj);
    nrn_function_call(nrn_symbol("read_object"), 1);
    Object* returned = nrn_object_pop();
    ok &= check(returned == obj, "component object read returns provider object");
    if (returned) {
        nrn_object_unref(returned);
    }
    ok &= check(nrn_double_pop() == 1004.0, "object read preserves the lower stack sentinel");

    ok &= check(nrn_hoc_call("obfunc read_nil() { return $o1.nil }") == 0,
                "nil read helper defined");
    nrn_double_push(1005.0);
    nrn_object_push(obj);
    nrn_function_call(nrn_symbol("read_nil"), 1);
    ok &= check(nrn_object_pop() == nullptr, "component nil read returns nil");
    ok &= check(nrn_double_pop() == 1005.0, "nil read preserves the lower stack sentinel");

    ok &= check(nrn_hoc_call("proc write_text() { $o1.text = $s2 }") == 0,
                "string write helper defined");
    nrn_double_push(1006.0);
    nrn_object_push(obj);
    nrn_str_push(&provider_text_ptr);
    static_assert(sizeof(provider_text) >= sizeof("assigned text"),
                  "provider_text must fit the assigned literal");
    std::memcpy(provider_text, "assigned text", sizeof("assigned text"));
    nrn_function_call(nrn_symbol("write_text"), 2);
    ok &= check(write_kind == 1, "component string write receives provider text");
    ok &= check(write_frame_matches, "component string write receives the expected frame");
    ok &= check(nrn_double_pop() == 0.0, "string write procedure result is balanced");
    ok &= check(nrn_double_pop() == 1006.0, "string write preserves the lower stack sentinel");

    ok &= check(nrn_hoc_call("proc write_object() { $o1.object = $o2 }") == 0,
                "object write helper defined");
    nrn_double_push(1007.0);
    nrn_object_push(obj);
    nrn_object_push(obj);
    nrn_function_call(nrn_symbol("write_object"), 2);
    ok &= check(write_kind == 2 && assigned_object_matches,
                "component object write receives provider object");
    ok &= check(write_frame_matches, "component object write receives the expected frame");
    ok &= check(nrn_double_pop() == 0.0, "object write procedure result is balanced");
    ok &= check(nrn_double_pop() == 1007.0, "object write preserves the lower stack sentinel");

    ok &= check(nrn_hoc_call("proc write_nil() { $o1.object = $o2 }") == 0,
                "nil write helper defined");
    nrn_double_push(1008.0);
    nrn_object_push(obj);
    nrn_object_push(nullptr);
    nrn_function_call(nrn_symbol("write_nil"), 2);
    ok &= check(write_kind == 3, "component nil write receives nil");
    ok &= check(write_frame_matches, "component nil write receives the expected frame");
    ok &= check(nrn_double_pop() == 0.0, "nil write procedure result is balanced");
    ok &= check(nrn_double_pop() == 1008.0, "nil write preserves the lower stack sentinel");

    ok &= check(nrn_hoc_call("func read_missing() { return $o1.missing }") == 0,
                "failing read helper defined");
    char call_error[128]{};
    nrn_double_push(1010.0);
    nrn_object_push(obj);
    ok &= check(nrn_function_call_nothrow(
                    nrn_symbol("read_missing"), 1, call_error, sizeof(call_error)) != 0,
                "component read failure reaches the nothrow API");
    ok &= check(std::strstr(call_error, "provider read failed") != nullptr,
                "component read failure preserves the provider message");
    Object* failed_read_arg = nrn_object_pop();
    ok &= check(failed_read_arg == obj, "failed read restores its object argument");
    if (failed_read_arg) {
        nrn_object_unref(failed_read_arg);
    }
    ok &= check(nrn_double_pop() == 1010.0, "failed read restores the pre-call stack");

    call_error[0] = '\0';
    fail_writes = true;
    nrn_double_push(1011.0);
    nrn_object_push(obj);
    nrn_double_push(88.0);
    ok &= check(nrn_function_call_nothrow(
                    nrn_symbol("write_component"), 2, call_error, sizeof(call_error)) != 0,
                "component write failure reaches the nothrow API");
    ok &= check(std::strstr(call_error, "provider write failed") != nullptr,
                "component write failure preserves the provider message");
    ok &= check(nrn_double_pop() == 88.0, "failed write restores its numeric argument");
    Object* failed_write_arg = nrn_object_pop();
    ok &= check(failed_write_arg == obj, "failed write restores its object argument");
    if (failed_write_arg) {
        nrn_object_unref(failed_write_arg);
    }
    ok &= check(nrn_double_pop() == 1011.0, "failed write restores the pre-call stack");
    fail_writes = false;

    ok &= check(nrn_hoc_call("begintemplate ProviderMethodCaller\n"
                             "public call\n"
                             "func call() { return $o1.fail_method($2, $3) }\n"
                             "endtemplate ProviderMethodCaller\n") == 0,
                "failing method caller template defined");
    Object* caller = nrn_object_new(nrn_symbol("ProviderMethodCaller"), 0);
    ok &= check(caller != nullptr, "method caller constructed");
    Symbol* call = nrn_method_symbol(caller, "call");
    const int reads_before_failure = read_calls;
    call_error[0] = '\0';
    nrn_double_push(1012.0);
    nrn_object_push(obj);
    nrn_double_push(12.5);
    nrn_double_push(30.25);
    ok &= check(nrn_method_call_nothrow(caller, call, 3, call_error, sizeof(call_error)) != 0,
                "component method failure reaches the method nothrow API");
    ok &= check(read_calls == reads_before_failure + 1 && failing_method_call_matches,
                "failing method callback consumes the expected isfunc frame exactly once");
    ok &= check(std::strstr(call_error, "provider method failed") != nullptr,
                "component method failure preserves the provider message");
    ok &= check(nrn_double_pop() == 30.25, "failed method restores its last argument");
    ok &= check(nrn_double_pop() == 12.5, "failed method restores its first numeric argument");
    Object* failed_method_arg = nrn_object_pop();
    ok &= check(failed_method_arg == obj, "failed method restores its object argument");
    if (failed_method_arg) {
        nrn_object_unref(failed_method_arg);
    }
    ok &= check(nrn_double_pop() == 1012.0, "failed method restores the pre-call stack");

    // A successful call after the failure also checks the restored interpreter context.
    nrn_double_push(1013.0);
    nrn_object_push(obj);
    nrn_double_push(12.5);
    nrn_double_push(30.25);
    ok &= check(nrn_function_call_nothrow(
                    nrn_symbol("call_component"), 3, call_error, sizeof(call_error)) == 0,
                "successful provider method still works after failure");
    ok &= check(nrn_double_pop() == 42.75, "post-failure method returns the exact sum");
    ok &= check(nrn_double_pop() == 1013.0, "post-failure method preserves the lower sentinel");
    nrn_object_unref(caller);

    nrn_object_unref(obj);
    return ok ? 0 : 1;
}
