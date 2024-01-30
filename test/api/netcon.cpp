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
    0x1.d8340689fafcdp+3,
    -0x1.2e02b18fab641p+6,
    -0x1.0517fe92a58d9p+6,
    -0x1.03e59d79732fcp+6,
    -0x1.03e51f949532bp+6,
#else
    -0x1.04p+6,
    0x1.d9fa4f205318p+3,
    -0x1.2e0327138fc9p+6,
    -0x1.051caef48c1p+6,
    -0x1.03e62a34d83f2p+6,
    -0x1.03e5860b6c0c1p+6,
#endif
};

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
