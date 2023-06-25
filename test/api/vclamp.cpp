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

void setup_neuron_api(void) {
    void* handle = dlopen("libnrniv.dylib", RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        handle = dlopen("libnrniv.so", RTLD_NOW | RTLD_LOCAL);
        if (!handle) {
            cout << "Couldn't open NEURON library." << endl << dlerror() << endl;
            exit(-1);
        }
    }
    nrn_init = reinterpret_cast<decltype(nrn_init)>(dlsym(handle, "nrn_init"));
    assert(nrn_init);
    nrn_str_push = reinterpret_cast<decltype(nrn_str_push)>(dlsym(handle, "nrn_str_push"));
    nrn_function_call = reinterpret_cast<decltype(nrn_function_call)>(
        dlsym(handle, "nrn_function_call"));
    nrn_symbol = reinterpret_cast<decltype(nrn_symbol)>(dlsym(handle, "nrn_symbol"));
    nrn_double_pop = reinterpret_cast<decltype(nrn_double_pop)>(dlsym(handle, "nrn_double_pop"));
    nrn_section_push = reinterpret_cast<decltype(nrn_section_push)>(
        dlsym(handle, "nrn_section_push"));
    nrn_section_new = reinterpret_cast<decltype(nrn_section_new)>(dlsym(handle, "nrn_section_new"));
    nrn_double_push = reinterpret_cast<decltype(nrn_double_push)>(dlsym(handle, "nrn_double_push"));
    nrn_object_new = reinterpret_cast<decltype(nrn_object_new)>(dlsym(handle, "nrn_object_new"));
    nrn_pp_property_array_set = reinterpret_cast<decltype(nrn_pp_property_array_set)>(
        dlsym(handle, "nrn_pp_property_array_set"));
    nrn_rangevar_push = reinterpret_cast<decltype(nrn_rangevar_push)>(
        dlsym(handle, "nrn_rangevar_push"));
    nrn_method_call = reinterpret_cast<decltype(nrn_method_call)>(dlsym(handle, "nrn_method_call"));
    nrn_object_unref = reinterpret_cast<decltype(nrn_object_unref)>(
        dlsym(handle, "nrn_object_unref"));
    nrn_vector_capacity = reinterpret_cast<decltype(nrn_vector_capacity)>(
        dlsym(handle, "nrn_vector_capacity"));
    nrn_vector_data = reinterpret_cast<decltype(nrn_vector_data)>(dlsym(handle, "nrn_vector_data"));
    nrn_method_symbol = reinterpret_cast<decltype(nrn_method_symbol)>(
        dlsym(handle, "nrn_method_symbol"));
    nrn_method_call = reinterpret_cast<decltype(nrn_method_call)>(dlsym(handle, "nrn_method_call"));
    nrn_object_pop = reinterpret_cast<decltype(nrn_object_pop)>(dlsym(handle, "nrn_object_pop"));
    nrn_symbol_push = reinterpret_cast<decltype(nrn_symbol_push)>(dlsym(handle, "nrn_symbol_push"));
    nrn_section_length_set = reinterpret_cast<decltype(nrn_section_length_set)>(
        dlsym(handle, "nrn_section_length_set"));
    nrn_segment_diam_set = reinterpret_cast<decltype(nrn_segment_diam_set)>(
        dlsym(handle, "nrn_segment_diam_set"));
}

int main(void) {
    Section* soma;
    Object* vclamp;
    Object* v;
    Object* t;
    char* temp_str;

    setup_neuron_api();
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
    nrn_pp_property_array_set(vclamp, "amp", 0, 0);
    nrn_pp_property_array_set(vclamp, "amp", 1, 10);
    nrn_pp_property_array_set(vclamp, "amp", 2, 5);
    nrn_pp_property_array_set(vclamp, "dur", 0, 1);
    nrn_pp_property_array_set(vclamp, "dur", 1, 2);
    nrn_pp_property_array_set(vclamp, "dur", 2, 3);

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