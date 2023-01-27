#include <catch2/catch.hpp>

#include "classreg.h"
#include "code.h"
#include "hocdec.h"
#include "hoc_membf.h"
#include "ocfunc.h"

#include <sstream>

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

namespace {
struct UtilityThatLikesThrowing {
    UtilityThatLikesThrowing(int data)
        : m_data{data} {}
    [[nodiscard]] bool hoc_destructor_should_throw() const {
        return m_data == 1.0;
    }
    [[noreturn]] void throw_error() const {
        throw std::runtime_error("throwing from throw_error");
    }
    [[noreturn]] void call_execerror() const {
        hoc_execerror("throwing a tantrum", nullptr);
    }

  private:
    int m_data{};
};
double throwing_util_member_call_execerror(void* ob_void) {
    auto* const ob = static_cast<UtilityThatLikesThrowing*>(ob_void);
    ob->call_execerror();
}
double throwing_util_member_throw_error(void* ob_void) {
    auto* const ob = static_cast<UtilityThatLikesThrowing*>(ob_void);
    ob->throw_error();
}
Object** throwing_util_member_call_execerror_obj_func(void* ob_void) {
    auto* const ob = static_cast<UtilityThatLikesThrowing*>(ob_void);
    ob->call_execerror();
}
Object** throwing_util_member_throw_error_obj_func(void* ob_void) {
    auto* const ob = static_cast<UtilityThatLikesThrowing*>(ob_void);
    ob->throw_error();
}
const char** throwing_util_member_call_execerror_str_func(void* ob_void) {
    auto* const ob = static_cast<UtilityThatLikesThrowing*>(ob_void);
    ob->call_execerror();
}
const char** throwing_util_member_throw_error_str_func(void* ob_void) {
    auto* const ob = static_cast<UtilityThatLikesThrowing*>(ob_void);
    ob->throw_error();
}
static Member_func throwing_util_members[] = {{"call_execerror",
                                               throwing_util_member_call_execerror},
                                              {"throw_error", throwing_util_member_throw_error},
                                              {nullptr, nullptr}};
static Member_ret_obj_func throwing_util_ret_obj_members[] = {
    {"call_execerror_obj_func", throwing_util_member_call_execerror_obj_func},
    {"throw_error_obj_func", throwing_util_member_throw_error_obj_func},
    {nullptr, nullptr}};
static Member_ret_str_func throwing_util_ret_str_members[] = {
    {"call_execerror_str_func", throwing_util_member_call_execerror_str_func},
    {"throw_error_str_func", throwing_util_member_throw_error_str_func},
    {nullptr, nullptr}};
void* throwing_util_constructor(Object*) {
    if (!ifarg(1)) {
        throw std::runtime_error("need at least one argument");
    }
    return new UtilityThatLikesThrowing{static_cast<int>(*hoc_getarg(1))};
}
void throwing_util_destructor(void* ob_void) {
    auto* const ob = static_cast<UtilityThatLikesThrowing*>(ob_void);
    bool const should_throw = ob->hoc_destructor_should_throw();
    delete ob;
    if (should_throw) {
        throw std::runtime_error("throwing from HOC destructor");
    }
}
}  // namespace

std::string hoc_oc_require_error(const char* buf) {
    std::ostringstream oss;
    auto const code = hoc_oc(buf, oss);
    REQUIRE(code != 0);
    auto const output = std::move(oss).str();
    REQUIRE(!output.empty());
    return output;
}

using Catch::Matchers::Contains;  // ContainsSubstring in newer Catch2
SCENARIO("Test calling code from HOC that throws exceptions", "[NEURON][hoc_interpreter]") {
    static bool registered = false;
    if (!registered) {
        class2oc("UtilityThatLikesThrowing",
                 throwing_util_constructor,
                 throwing_util_destructor,
                 throwing_util_members,
                 nullptr /* checkpoint */,
                 throwing_util_ret_obj_members,
                 throwing_util_ret_str_members);
        registered = true;
        // declare these test variables exactly once
        REQUIRE(hoc_oc("objref nil, util\n") == 0);
    }
    GIVEN("A HOC object constructor that throws") {
        REQUIRE_THAT(hoc_oc_require_error("util = new UtilityThatLikesThrowing()\n"),
                     Contains("hoc_execerror: UtilityThatLikesThrowing[0] constructor: need at "
                              "least one argument"));
    }
    GIVEN("A HOC object destructor that throws") {
        REQUIRE_THAT(hoc_oc_require_error("util = nil\n"
                                          "util = new UtilityThatLikesThrowing(1)\n"
                                          "util = nil\n"),
                     Contains("hoc_execerror: UtilityThatLikesThrowing[0] destructor: throwing "
                              "from HOC destructor"));
    }
    GIVEN("A HOC object whose constructor and destructor succeed") {
        // Make sure there are not any instances alive from previous tests, otherwise we can't
        // hardcode the zero in "UtilityThatLikesThrowing[0]"
        REQUIRE(hoc_oc("util = nil\n") == 0);
        // Make an instance and tell it not to throw from its destructor (with the 2)
        REQUIRE(hoc_oc("util = new UtilityThatLikesThrowing(2)\n") == 0);
        // Generate tests for the HOC member functions returning doubles, objects and strings
        auto const method_suffix = GENERATE(as<std::string>{}, "", "_obj_func", "_str_func");
        WHEN("A member function that throws is called") {
            REQUIRE_THAT(
                hoc_oc_require_error(("util.throw_error" + method_suffix + "()\n").c_str()),
                Contains("hoc_execerror: UtilityThatLikesThrowing[0]::throw_error" + method_suffix +
                         ": throwing "
                         "from throw_error"));
        }
        WHEN("A member function that calls hoc_execerror is called") {
            REQUIRE_THAT(hoc_oc_require_error(
                             ("util.call_execerror" + method_suffix + "()\n").c_str()),
                         Contains("hoc_execerror: UtilityThatLikesThrowing[0]::call_execerror" +
                                  method_suffix +
                                  ": "
                                  "hoc_execerror: throwing a tantrum"));
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
