// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <dlfcn.h>
#include "neuronapi.h"
#include "./test_common.h"

using std::cout;
using std::endl;
using std::ofstream;

static const char* argv[] = {"netcon", "-nogui", "-nopython", nullptr};

constexpr std::array<double, 6> EXPECTED_V{
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
    Object* v;
    Object* t;
    char* temp_str;

    nrn_init(3, argv);

    // load the stdrun library
    nrn_function_call_s("load_file", "stdrun.hoc");

    // topology
    auto soma = nrn_section_new("soma");

    // ion channels
    nrn_mechanism_insert(soma, nrn_symbol("hh"));

    // NetStim
    auto ns = nrn_object_new(nrn_symbol("NetStim"), 0);
    nrn_property_set(ns, "start", 5);
    nrn_property_set(ns, "noise", 1);
    nrn_property_set(ns, "interval", 5);
    nrn_property_set(ns, "number", 10);

    // syn = h.ExpSyn(soma(0.5))
    nrn_double_push(0.5);
    auto syn = nrn_object_new(nrn_symbol("ExpSyn"), 1);
    nrn_property_set(syn, "tau", 3);  // 3 ms timeconstant
    nrn_property_set(syn, "e", 0);    // 0 mV reversal potential (excitatory synapse)

    // nc = h.NetCon(ns, syn)
    nrn_object_push(ns);
    nrn_object_push(syn);
    auto nc = nrn_object_new(nrn_symbol("NetCon"), 2);
    nrn_property_array_set(nc, "weight", 0, 0.5);
    nrn_property_set(nc, "delay", 0);

    auto vec = nrn_object_new(nrn_symbol("Vector"), 0);

    // nc.record(vec)
    nrn_object_push(vec);
    nrn_method_call(nc, nrn_method_symbol(nc, "record"), 1);
    // TODO: record probably put something on the stack that should be removed

    // setup recording
    v = nrn_object_new(nrn_symbol("Vector"), 0);
    nrn_rangevar_push(nrn_symbol("v"), soma, 0.5);
    nrn_double_push(20);
    nrn_method_call(v, nrn_method_symbol(v, "record"), 2);
    nrn_object_unref(nrn_object_pop());  // record returns the vector

    nrn_function_call_d("finitialize", -65);

    nrn_function_call_d("continuerun", 100.5);

    if (!approximate(EXPECTED_V, v)) {
        return 1;
    }
}