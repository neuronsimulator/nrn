// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include "neuronapi.h"
#include "./test_common.h"

using std::cout;
using std::endl;
using std::ofstream;

constexpr std::initializer_list<double> EXPECTED_V{
#ifndef CORENEURON_ENABLED
    -0x1.04p+6,
    -0x1.085a63d029bc3p+6,
    -0x1.112a5e95eb67cp+6,
    -0x1.1795abaec26c1p+6,
    -0x1.0422351f3f9dcp+6,
    -0x1.03e5317ac368cp+6,
#else
    -0x1.04p+6,
    -0x1.085a703d657a7p+6,
    -0x1.112d0039e9c38p+6,
    -0x1.17974aa201b7bp+6,
    -0x1.041fdf57a182bp+6,
    -0x1.03e58fad20b92p+6,
#endif
};

extern "C" void modl_reg(){/* No modl_reg */};

int main(void) {
    static std::array<const char*, 4> argv = {"netcon", "-nogui", "-nopython", nullptr};
    nrn_init(3, argv.data());

    // load the stdrun library
    nrn_function_call_s("load_file", "stdrun.hoc");

    // topology
    auto soma = nrn_section_new("soma");

    // ion channels
    nrn_mechanism_insert(soma, nrn_symbol("hh"));

    // NetStim
    auto ns = nrn_object_new_NoArgs("NetStim");
    nrn_property_set(ns, "start", 5.);
    nrn_property_set(ns, "noise", 1.);
    nrn_property_set(ns, "interval", 5.);
    nrn_property_set(ns, "number", 10.);

    nrn_method_call(ns, "noiseFromRandom123", "ddd", 1., 2., 3.);

    // syn = h.ExpSyn(soma(0.5))
    auto syn = nrn_object_new("ExpSyn", NRN_ARG_DOUBLE, 0.5);
    nrn_property_set(syn, "tau", 3.);  // 3 ms timeconstant
    nrn_property_set(syn, "e", 0.);    // 0 mV reversal potential (excitatory synapse)

    // nc = h.NetCon(ns, syn)
    auto nc = nrn_object_new("NetCon", "oo", ns, syn);
    nrn_property_array_set(nc, "weight", 0, 0.5);
    nrn_property_set(nc, "delay", 0);

    // record from the netcon directly
    auto vec = nrn_object_new_NoArgs("Vector");
    nrn_method_call(nc, "record", "o", vec);

    // setup recording
    auto soma_v = nrn_rangevar_new(soma, 0.5, "v");
    Object* v = nrn_object_new_NoArgs("Vector");
    nrn_method_call(v, "record", NRN_ARG_RANGEVAR NRN_ARG_DOUBLE, &soma_v, 20.0);

    nrn_function_call("finitialize", NRN_ARG_DOUBLE, -65.0);
    nrn_function_call("continuerun", NRN_ARG_DOUBLE, 100.5);

    int n_nc_events = nrn_vector_size(vec);
    if (n_nc_events != 10) {
        std::cerr << "Wrong number of Netcon events: " << n_nc_events << std::endl;
        return 1;
    }

    if (!approximate(EXPECTED_V, v)) {
        return 1;
    }
}
