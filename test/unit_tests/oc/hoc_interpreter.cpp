#include <catch2/catch.hpp>

#include <hocdec.h>
#include <ocfunc.h>
#include <code.h>

TEST_CASE("Test hoc interpreter", "[NEURON][hoc_interpreter]") {
    hoc_init_space();
    hoc_pushx(4.0);
    hoc_pushx(5.0);
    hoc_add();
    REQUIRE(hoc_xpop() == 9.0);
}

constexpr static auto check_tempobj_canary = 0xDEADBEEF;

static void istmpobj() {
    auto _listmpobj = hoc_is_tempobj_arg(1) ? check_tempobj_canary : 0;
    hoc_retpushx(_listmpobj);
}

static VoidFunc hoc_intfunc[] = {{"istmpobj", istmpobj}, {0, 0}};


TEST_CASE("Test hoc_register_var", "[NEURON][hoc_interpreter][istmpobj]") {
    hoc_register_var(nullptr, nullptr, hoc_intfunc);
    REQUIRE(hoc_lookup("istmpobj") != nullptr);
    hoc_pushobj(hoc_temp_objptr(nullptr));
    REQUIRE(hoc_call_func(hoc_lookup("istmpobj"), 1) == check_tempobj_canary);
}

SCENARIO("Test for issue #1995", "[NEURON][hoc_interpreter][issue-1995]") {
    // This is targeting AddressSanitizer errors that were seen in #1995
    GIVEN("Test calling a function that returns an Object that lived on the stack") {
        constexpr auto vector_size = 5;
        REQUIRE(hoc_oc(("obfunc foo() {localobj lv1\n"
                        "  lv1 = new Vector(" +
                        std::to_string(vector_size) +
                        ")\n"
                        "  lv1.indgen()\n"
                        "  return lv1\n"
                        "}\n")
                           .c_str()) == 0);
        WHEN("It is called") {
            REQUIRE(hoc_oc("objref v1\n"
                           "v1 = foo()\n") == 0);
            THEN("The returned value should be correct") {
                auto const i = GENERATE_COPY(range(1, vector_size));
                REQUIRE(hoc_oc(("hoc_ac_ = v1.x[" + std::to_string(i) + "]\n").c_str()) == 0);
                REQUIRE(hoc_ac_ == i);
            }
        }
    }
}
#if USE_PYTHON
TEST_CASE("Test hoc_array_access", "[NEURON][hoc_interpreter][nrnpython][array_access]") {
    REQUIRE(hoc_oc("nrnpython(\"avec = [0,1,2]\")\n"
                   "objref po\n"
                   "po = new PythonObject()\n"
                   "po = po.avec\n") == 0);
    THEN("The avec can value should be correct") {
        auto const i = GENERATE_COPY(range(0, 3));
        REQUIRE(hoc_oc(("hoc_ac_ = po._[" + std::to_string(i) + "]\n").c_str()) == 0);
        REQUIRE(hoc_ac_ == i);
    }
}
#endif
