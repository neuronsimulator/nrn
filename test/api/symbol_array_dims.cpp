// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
// nrn_symbol_array_dims reports every dimension of an array symbol;
// nrn_symbol_array_length gives only the first.
#include <array>
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

int main(void) {
    static std::array<const char*, 4> argv = {"symbol_array_dims", "-nogui", "-nopython", nullptr};
    nrn_init(3, argv.data());

    bool ok = true;
    nrn_hoc_call(
        "double grid[3][4]\n"
        "objref cells[2][5][7]\n"
        "scalar = 1\n");

    std::array<int, 4> dims{};
    Symbol* grid = nrn_symbol("grid");
    ok &= check(nrn_symbol_array_dims(grid, dims.data(), 4) == 2, "2-D double array");
    ok &= check(dims[0] == 3 && dims[1] == 4, "2-D sizes");
    ok &= check(nrn_symbol_array_length(grid) == 3, "array_length still gives the first");

    dims = {};
    Symbol* cells = nrn_symbol("cells");
    ok &= check(nrn_symbol_array_dims(cells, dims.data(), 4) == 3, "3-D objref array");
    ok &= check(dims[0] == 2 && dims[1] == 5 && dims[2] == 7, "3-D sizes");

    // A smaller buffer gets the leading sizes; the count is still the total.
    dims = {};
    ok &= check(nrn_symbol_array_dims(cells, dims.data(), 2) == 3, "count with short buffer");
    ok &= check(dims[0] == 2 && dims[1] == 5 && dims[2] == 0, "short buffer is not overrun");
    ok &= check(nrn_symbol_array_dims(cells, nullptr, 0) == 3, "count only");

    ok &= check(nrn_symbol_array_dims(nrn_symbol("scalar"), dims.data(), 4) == 0, "scalar");
    ok &= check(nrn_symbol_array_dims(nullptr, dims.data(), 4) == 0, "null symbol");

    // A mechanism range array: extracellular's xraxial has nlayer elements.
    dims = {};
    ok &= check(nrn_symbol_array_dims(nrn_symbol("xraxial"), dims.data(), 4) == 1,
                "range-variable array");
    ok &= check(dims[0] == 2, "xraxial has the default 2 layers");

    // Redeclaring an array updates its dimensions.
    nrn_hoc_call("double grid[6]\n");
    dims = {};
    ok &= check(nrn_symbol_array_dims(grid, dims.data(), 4) == 1 && dims[0] == 6,
                "redeclared array");

    return ok ? 0 : 1;
}
