// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
// Exercises nrn_rangevar_handle_str: the text Python prints for a reference
// such as seg._ref_v, which names the storage and the row the value is in.
#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include "neuronapi.h"

using std::cerr;
using std::endl;

extern "C" void _ptrtest_reg();
extern "C" void modl_reg() {
    _ptrtest_reg();
}

static bool check(bool cond, const char* msg) {
    if (!cond) {
        cerr << "FAIL: " << msg << endl;
    }
    return cond;
}

// The row in "... row=<row>/<count> ...", or -1.
static int row_of(const std::string& text) {
    auto pos = text.find("row=");
    return pos == std::string::npos ? -1 : std::atoi(text.c_str() + pos + 4);
}

int main(void) {
    static std::array<const char*, 4> argv = {"rangevar_handle_str",
                                              "-nogui",
                                              "-nopython",
                                              nullptr};
    nrn_init(3, argv.data());

    bool ok = true;
    char text[256]{};
    Section* soma = nrn_section_new("soma");
    nrn_nseg_set(soma, 3);
    nrn_mechanism_insert(soma, nrn_symbol("hh"));
    nrn_double_push(-65);
    nrn_function_call(nrn_symbol("finitialize"), 1);
    nrn_double_pop();
    Symbol* v = nrn_symbol("v");

    // Voltage: the node storage, at the row of the node.
    int len = nrn_rangevar_handle_str(v, soma, 0.5, 0, text, sizeof(text));
    std::string described{text};
    ok &= check(len == int(described.size()), "returns the text length");
    ok &= check(described.rfind("data_handle<double>{", 0) == 0, "a double data handle");
    ok &= check(described.find("Voltage") != std::string::npos, "names the voltage storage");
    ok &= check(described.find("val=-65") != std::string::npos, "shows the value");
    for (double x: {1.0 / 6, 0.5, 5.0 / 6}) {
        nrn_rangevar_handle_str(v, soma, x, 0, text, sizeof(text));
        ok &= check(row_of(text) == nrn_segment_node_index(soma, x), "row is the node index");
    }

    // A mechanism variable names its mechanism.
    nrn_rangevar_handle_str(nrn_symbol("gnabar_hh"), soma, 0.5, 0, text, sizeof(text));
    ok &= check(std::strstr(text, "data_handle<double>") && std::strstr(text, "hh"),
                "mechanism variable");

    // A short buffer: truncated, and the full length is returned.
    std::array<char, 8> small{};
    ok &= check(nrn_rangevar_handle_str(v, soma, 0.5, 0, small.data(), small.size()) == len &&
                    std::strlen(small.data()) == small.size() - 1,
                "truncates and returns the full length");

    // Errors, with a message: an uninserted mechanism, an unset POINTER, no
    // section.
    ok &= check(nrn_rangevar_handle_str(nrn_symbol("gl_hh"), soma, 0.5, 0, text, sizeof(text)) >= 0,
                "inserted mechanism");
    Section* dend = nrn_section_new("dend");
    ok &= check(nrn_rangevar_handle_str(nrn_symbol("gl_hh"), dend, 0.5, 0, text, sizeof(text)) ==
                        -1 &&
                    std::strstr(text, "not inserted"),
                "uninserted mechanism is an error");
    nrn_mechanism_insert(dend, nrn_symbol("ptrtest"));
    ok &= check(nrn_rangevar_handle_str(
                    nrn_symbol("src_ptrtest"), dend, 0.5, 0, text, sizeof(text)) == -1 &&
                    std::strstr(text, "POINTER is not set"),
                "unset POINTER is an error");
    ok &= check(nrn_rangevar_handle_str(v, nullptr, 0.5, 0, text, sizeof(text)) == -1,
                "NULL section");
    return ok ? 0 : 1;
}
