// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
// Exercises nrn_rangevar_handle_str: the text Python prints for a reference
// such as seg._ref_v, which names the storage and the row the value is in.
#include <algorithm>
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
    ok &= check(std::string(text) == "data_handle<double>{cont=hh gnabar row=1/3 val=0.12}",
                "mechanism variable text");

    // Buffer edges. The text is `len` characters; the return is always `len`, the
    // buffer gets at most size - 1 characters and a terminator, and a byte past
    // `size` is never touched.
    ok &= check(nrn_rangevar_handle_str(v, soma, 0.5, 0, nullptr, 0) == len, "NULL buffer counts");
    ok &= check(nrn_rangevar_handle_str(v, soma, 0.5, 0, nullptr, 64) == len,
                "NULL buffer with a size counts");
    for (int size: {0, 1, 8, len, len + 1, len + 2}) {
        std::array<char, 128> big{};
        big.fill('Z');
        ok &= check(nrn_rangevar_handle_str(v, soma, 0.5, 0, big.data(), size) == len,
                    "returns the full length for every size");
        ok &= check(big[size] == 'Z', "nothing written past size");
        if (size == 0) {
            ok &= check(big[0] == 'Z', "size 0 writes nothing");
        } else {
            std::string const expected = described.substr(0, std::min(len, size - 1));
            ok &= check(std::string(big.data()) == expected, "text truncated to size - 1");
        }
    }
    // The same for the error message.
    ok &= check(nrn_rangevar_handle_str(
                    nrn_symbol("gl_hh"), nrn_section_new("bare"), 0.5, 0, nullptr, 0) == -1,
                "error with a NULL buffer");
    std::array<char, 6> tiny{};
    ok &= check(nrn_rangevar_handle_str(nrn_symbol("gl_hh"),
                                        nrn_section_new("bare2"),
                                        0.5,
                                        0,
                                        tiny.data(),
                                        tiny.size()) == -1 &&
                    std::string(tiny.data()) == "gl_hh",
                "error message truncated to size - 1");

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

    // A POINTER that is set describes the handle it holds: the voltage of the
    // node it was pointed at.
    nrn_double_push(-1234);  // sentinel below the source
    nrn_rangevar_push(v, soma, 0.5);
    char err[256];
    ok &= check(nrn_setpointer_pop(nrn_symbol("src_ptrtest"), dend, 0.5, err, sizeof(err)) == 0,
                "setpointer");
    ok &= check(nrn_double_pop() == -1234, "setpointer consumed only its source");
    char voltage[256];
    int const voltage_len = nrn_rangevar_handle_str(v, soma, 0.5, 0, voltage, sizeof(voltage));
    ok &= check(nrn_rangevar_handle_str(
                    nrn_symbol("src_ptrtest"), dend, 0.5, 0, text, sizeof(text)) == voltage_len &&
                    std::string(text) == voltage,
                "a set POINTER describes the handle it holds");
    return ok ? 0 : 1;
}
