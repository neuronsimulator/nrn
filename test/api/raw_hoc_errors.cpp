#include "neuronapi.h"
#include <array>
#include <iostream>

extern "C" void modl_reg() {}

static const char* read_failure(Object*, Symbol*, int, int) {
    return "component read failed";
}

static const char* write_failure(Object*) {
    return "component write failed";
}

static bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
    }
    return condition;
}

int main() {
    std::array<const char*, 4> args = {"raw_hoc_errors", "-nogui", "-nopython", nullptr};
    nrn_init(3, args.data());
    char error[1024]{};
    if (!check(nrn_template_set_component_hooks(
                   nrn_symbol("PythonObject"), read_failure, write_failure, error, sizeof(error)),
               "register failing provider") ||
        !check(nrn_hoc_call("objref raw_provider, raw_vectors\n"
                            "raw_provider = new PythonObject()\n"
                            "raw_vectors = new List(\"Vector\")\n"
                            "raw_marks = 0\n"
                            "proc raw_fail_object() { execerror(\"object failure\") }") == 0,
               "initialize HOC test state")) {
        return 1;
    }
    // Each path exceeds the old ~1000-failure stack overflow threshold. The
    // provider leaves its complete frame for the interpreter to recover.
    const std::array<const char*, 7> commands = {
        "{ execerror(\"raw boom\") raw_marks += 1 }",
        "{ raw_x = raw_provider.bad raw_marks += 1 }",
        "{ raw_provider.fail() raw_marks += 1 }",
        "{ raw_provider.bad = 3 raw_marks += 1 }",
        "{ raw_x = raw_provider._[2] raw_marks += 1 }",
        "{ raw_fail_object(new Vector(3)) raw_marks += 1 }",
        "{ execute(\"execerror(\\\"nested boom\\\")\") raw_marks += 1 }"};
    for (auto command: commands) {
        nrn_double_push(89.25);
        nrn_int_push(37);
        for (int i = 0; i < 1200; ++i) {
            if (!check(nrn_hoc_call(command) != 0, "raw command reports failure")) {
                return 1;
            }
        }
        if (!check(nrn_stack_type() == STACK_IS_INT, "no operands remain above caller sentinels") ||
            !check(nrn_int_pop() == 37, "integer sentinel preserved") ||
            !check(nrn_double_pop() == 89.25, "numeric sentinel preserved") ||
            !check(nrn_hoc_call("hoc_ac_ = raw_marks") == 0, "command after errors succeeds") ||
            !check(*nrn_symbol_dataptr(nrn_symbol("hoc_ac_")) == 0.0,
                   "no continuation after failure") ||
            !check(nrn_hoc_call("hoc_ac_ = raw_vectors.count()") == 0,
                   "query temporary object ownership") ||
            !check(*nrn_symbol_dataptr(nrn_symbol("hoc_ac_")) == 0.0,
                   "temporary Vector released after failure")) {
            return 1;
        }
    }
    if (!check(nrn_hoc_call("hoc_ac_ = 23") == 0, "subsequent assignment succeeds") ||
        !check(*nrn_symbol_dataptr(nrn_symbol("hoc_ac_")) == 23.0, "exact result after errors")) {
        return 1;
    }
}
