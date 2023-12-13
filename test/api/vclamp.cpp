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

static const char* argv[] = {"vclamp", "-nogui", "-nopython", nullptr};

extern "C" void modl_reg(){};

int main(void) {
    Section* soma;
    Object* vclamp;
    Object* v;
    Object* t;
    char* temp_str;

    nrn_init(3, argv);

    // load the stdrun library
    temp_str = new char[11];
    strcpy(temp_str, "stdrun.hoc");
    nrn_str_push(&temp_str);
    nrn_function_call(nrn_symbol("load_file"), 1);
    nrn_double_pop();
    delete[] temp_str;


    // topology
    soma = nrn_section_new("soma");

    // define soma morphology with two 3d points
    nrn_section_push(soma);
    assert(soma);
    assert(nrn_section_length_set);
    nrn_section_length_set(soma, 10);
    nrn_segment_diam_set(soma, 0.5, 3);

    // voltage clamp at soma(0.5)
    nrn_double_push(0.5);
    vclamp = nrn_object_new(nrn_symbol("VClamp"), 1);
    // 0 mV for 1 ms; 10 mV for the next 2 ms; 5 mV for the next 3 ms
    nrn_property_array_set(vclamp, "amp", 0, 0);
    nrn_property_array_set(vclamp, "amp", 1, 10);
    nrn_property_array_set(vclamp, "amp", 2, 5);
    nrn_property_array_set(vclamp, "dur", 0, 1);
    nrn_property_array_set(vclamp, "dur", 1, 2);
    nrn_property_array_set(vclamp, "dur", 2, 3);

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

    // continuerun(6)
    nrn_double_push(6);
    nrn_function_call(nrn_symbol("continuerun"), 1);
    nrn_double_pop();

    double* tvec = nrn_vector_data(t);
    double* vvec = nrn_vector_data(v);
    ofstream out_file;
    out_file.open("vclamp.csv");
    for (auto i = 0; i < nrn_vector_capacity(t); i++) {
        out_file << tvec[i] << "," << vvec[i] << endl;
    }
    out_file.close();
    cout << "Results saved to vclamp.csv" << endl;
}