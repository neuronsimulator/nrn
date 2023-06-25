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
    nrn_push_section = reinterpret_cast<decltype(nrn_push_section)>(
        dlsym(handle, "nrn_push_section"));
    nrn_section_new = reinterpret_cast<decltype(nrn_section_new)>(dlsym(handle, "nrn_section_new"));
    nrn_nseg_set = reinterpret_cast<decltype(nrn_nseg_set)>(dlsym(handle, "nrn_nseg_set"));
    nrn_insert_mechanism = reinterpret_cast<decltype(nrn_insert_mechanism)>(
        dlsym(handle, "nrn_insert_mechanism"));
    nrn_double_push = reinterpret_cast<decltype(nrn_double_push)>(dlsym(handle, "nrn_double_push"));
    nrn_object_new = reinterpret_cast<decltype(nrn_object_new)>(dlsym(handle, "nrn_object_new"));
    nrn_rangevar_push = reinterpret_cast<decltype(nrn_rangevar_push)>(
        dlsym(handle, "nrn_rangevar_push"));
    nrn_method_call = reinterpret_cast<decltype(nrn_method_call)>(dlsym(handle, "nrn_method_call"));
    nrn_object_unref = reinterpret_cast<decltype(nrn_object_unref)>(
        dlsym(handle, "nrn_object_unref"));
    nrn_vector_capacity = reinterpret_cast<decltype(nrn_vector_capacity)>(
        dlsym(handle, "nrn_vector_capacity"));
    nrn_vector_data_ptr = reinterpret_cast<decltype(nrn_vector_data_ptr)>(
        dlsym(handle, "nrn_vector_data_ptr"));
    nrn_method_symbol = reinterpret_cast<decltype(nrn_method_symbol)>(
        dlsym(handle, "nrn_method_symbol"));
    nrn_method_call = reinterpret_cast<decltype(nrn_method_call)>(dlsym(handle, "nrn_method_call"));
    nrn_object_pop = reinterpret_cast<decltype(nrn_object_pop)>(dlsym(handle, "nrn_object_pop"));
    nrn_symbol_push = reinterpret_cast<decltype(nrn_symbol_push)>(
        dlsym(handle, "nrn_symbol_push"));
    nrn_pp_property_set = reinterpret_cast<decltype(nrn_pp_property_set)>(
        dlsym(handle, "nrn_pp_property_set"));        
}

int main(void) {
    Section* soma;
    Object* iclamp;
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
    nrn_nseg_set(soma, 3);

    // define soma morphology with two 3d points
    nrn_push_section(soma);
    // (0, 0, 0, 10)
    nrn_double_push(0);
    nrn_double_push(0);
    nrn_double_push(0);
    nrn_double_push(10);
    nrn_function_call(nrn_symbol("pt3dadd"), 4);
    nrn_double_pop();  // pt3dadd returns a number
    // (10, 0, 0, 10)
    nrn_double_push(10);
    nrn_double_push(0);
    nrn_double_push(0);
    nrn_double_push(10);
    nrn_function_call(nrn_symbol("pt3dadd"), 4);
    nrn_double_pop();  // pt3dadd returns a number

    // ion channels
    nrn_insert_mechanism(soma, nrn_symbol("hh"));

    // current clamp at soma(0.5)
    nrn_double_push(0.5);
    iclamp = nrn_object_new(nrn_symbol("IClamp"), 1);
    nrn_pp_property_set(iclamp, "amp", 0.3);
    nrn_pp_property_set(iclamp, "del", 1);
    nrn_pp_property_set(iclamp, "dur", 0.1);

    // setup recording
    v = nrn_object_new(nrn_symbol("Vector"), 0);
    nrn_rangevar_push(nrn_symbol("v"), soma, 0.5);
    nrn_method_call(v, nrn_method_symbol(v, "record"), 1);
    nrn_object_unref(nrn_object_pop());  // record returns the vector
    t = nrn_object_new(nrn_symbol("Vector"), 0);
    assert(nrn_symbol_push);
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

    double* tvec = nrn_vector_data_ptr(t);
    double* vvec = nrn_vector_data_ptr(v);
    ofstream out_file;
    out_file.open("hh_sim.csv");
    for (auto i = 0; i < nrn_vector_capacity(t); i++) {
        out_file << tvec[i] << "," << vvec[i] << endl;
    }
    out_file.close();
    cout << "Results saved to hh_sim.csv" << endl;
}