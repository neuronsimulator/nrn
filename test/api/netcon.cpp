// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
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

extern "C" void modl_reg(){};

int main(void) {
    Object* v;
    Object* t;
    char* temp_str;

    nrn_init(3, argv);

    // load the stdrun library
    temp_str = strdup("stdrun.hoc");
    nrn_str_push(&temp_str);
    nrn_function_call(nrn_symbol("load_file"), 1);
    nrn_double_pop();
    delete[] temp_str;

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
    nrn_method_call(v, nrn_method_symbol(v, "record"), 1);
    nrn_object_unref(nrn_object_pop());  // record returns the vector
    t = nrn_object_new(nrn_symbol("Vector"), 0);
    nrn_symbol_push(nrn_symbol("t"));
    nrn_method_call(t, nrn_method_symbol(t, "record"), 1);
    nrn_object_unref(nrn_object_pop());  // record returns the vector

    // finitialize(-65)
    nrn_double_push(-65);
    nrn_function_call(nrn_symbol("finitialize"), 1);
    nrn_double_pop();

    // continuerun(100)
    nrn_double_push(100);
    nrn_function_call(nrn_symbol("continuerun"), 1);
    nrn_double_pop();

    long n_voltages = nrn_vector_capacity(t);
    auto ref_file = std::string(std::getenv("CURRENT_SOURCE_DIR")) + "/ref/netcon.csv";
    if (compare_spikes(ref_file.c_str(), nrn_vector_data(t), nrn_vector_data(v), n_voltages)) {
        return 0;
    }
    return 1;
}