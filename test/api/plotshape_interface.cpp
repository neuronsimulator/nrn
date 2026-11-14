// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
// nrn_get_plotshape_interface and the queries on the interface it returns. A
// PlotShape is a ShapePlot with a GUI and a ShapePlotData without one, and this
// test leaves that to the run: without -nogui it gets a ShapePlot when there is
// a display. Only a ShapePlot can fail, so a run with a display is what guards
// nrn_get_plotshape_interface. Windows has no display check, so it stays out.
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

static Object* object_named(const char* name) {
    return nrn_symbol_object_get(nrn_symbol(name));
}

int main(void) {
    static std::array<const char*, 4> argv = {"plotshape_interface", "-nopython", nullptr, nullptr};
    int argc = 2;
#ifdef _WIN32
    argv[argc++] = "-nogui";
#endif
    nrn_init(argc, argv.data());

    // ps gets values other than PlotShape's defaults, so the reads below show
    // they come from that object; ps_of_list keeps the default scale.
    nrn_hoc_call(
        "create s\n"
        "s insert hh\n"
        "objref ps, sl, ps_of_list\n"
        "ps = new PlotShape(0)\n"
        "ps.variable(\"m_hh\")\n"
        "ps.scale(-65.5, 12.25)\n"
        "sl = new SectionList()\n"
        "s sl.append()\n"
        "ps_of_list = new PlotShape(sl, 0)\n");

    bool ok = true;
    ShapePlotInterface* spi = nrn_get_plotshape_interface(object_named("ps"));
    ok &= check(strcmp(nrn_get_plotshape_varname(spi), "m_hh") == 0, "varname");
    ok &= check(nrn_get_plotshape_low(spi) == -65.5f, "low");
    ok &= check(nrn_get_plotshape_high(spi) == 12.25f, "high");
    ok &= check(nrn_get_plotshape_section_list(spi) == nullptr, "no section list");

    ShapePlotInterface* of_list = nrn_get_plotshape_interface(object_named("ps_of_list"));
    ok &= check(nrn_get_plotshape_section_list(of_list) == object_named("sl"), "section list");
    // Both kinds start at -80 to 40. The variable name does not compare: a
    // ShapePlot starts with v, a ShapePlotData with none until variable().
    ok &= check(nrn_get_plotshape_low(of_list) == -80.f, "default low");
    ok &= check(nrn_get_plotshape_high(of_list) == 40.f, "default high");

    return ok ? 0 : 1;
}
