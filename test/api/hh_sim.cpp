// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
#include "neuronapi.h"
#include <dlfcn.h>

#include <cassert>
#include <cstring>
#include <iostream>
#include <fstream>

using std::cout;
using std::endl;
using std::ofstream;

static const char* argv[] = {"hh_sim", "-nogui", "-nopython", nullptr};

extern "C" void modl_reg(){};

int main(void) {
    Section* soma;
    Object* iclamp;
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
    soma = nrn_section_new("soma");
    nrn_nseg_set(soma, 3);

    // define soma morphology with two 3d points
    nrn_section_push(soma);
    for (double x: {0, 0, 0, 10}) {
        nrn_double_push(x);
    }
    nrn_function_call(nrn_symbol("pt3dadd"), 4);
    nrn_double_pop();  // pt3dadd returns a number
    for (double x: {10, 0, 0, 10}) {
        nrn_double_push(x);
    }
    nrn_function_call(nrn_symbol("pt3dadd"), 4);
    nrn_double_pop();  // pt3dadd returns a number

    // ion channels
    nrn_mechanism_insert(soma, nrn_symbol("hh"));

    // current clamp at soma(0.5)
    nrn_double_push(0.5);
    iclamp = nrn_object_new(nrn_symbol("IClamp"), 1);
    nrn_property_set(iclamp, "amp", 0.3);
    nrn_property_set(iclamp, "del", 1);
    nrn_property_set(iclamp, "dur", 0.1);

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

    // continuerun(10)
    nrn_double_push(10);
    nrn_function_call(nrn_symbol("continuerun"), 1);
    nrn_double_pop();

    double* tvec = nrn_vector_data(t);
    double* vvec = nrn_vector_data(v);
    ofstream out_file;
    out_file.open("hh_sim.csv");
    for (auto i = 0; i < nrn_vector_capacity(t); i++) {
        out_file << tvec[i] << "," << vvec[i] << endl;
    }
    out_file.close();
    cout << "Results saved to hh_sim.csv" << endl;
}