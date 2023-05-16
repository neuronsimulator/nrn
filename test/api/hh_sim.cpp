// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
#include "neuronapi.h"
#include <dlfcn.h>
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
        cout << "Couldn't open dylib." << endl << dlerror() << endl;
        exit(-1);
    }
    nrn_init = reinterpret_cast<decltype(nrn_init)>(dlsym(handle, "nrn_init"));
    assert(nrn_init);
    nrn_push_str = reinterpret_cast<decltype(nrn_push_str)>(dlsym(handle, "nrn_push_str"));
    nrn_call_function = reinterpret_cast<decltype(nrn_call_function)>(
        dlsym(handle, "nrn_call_function"));
    nrn_get_symbol = reinterpret_cast<decltype(nrn_get_symbol)>(dlsym(handle, "nrn_get_symbol"));
    nrn_pop_double = reinterpret_cast<decltype(nrn_pop_double)>(dlsym(handle, "nrn_pop_double"));
    nrn_push_section = reinterpret_cast<decltype(nrn_push_section)>(
        dlsym(handle, "nrn_push_section"));
    nrn_new_section = reinterpret_cast<decltype(nrn_new_section)>(dlsym(handle, "nrn_new_section"));
    nrn_set_nseg = reinterpret_cast<decltype(nrn_set_nseg)>(dlsym(handle, "nrn_set_nseg"));
    nrn_insert_mechanism = reinterpret_cast<decltype(nrn_insert_mechanism)>(
        dlsym(handle, "nrn_insert_mechanism"));
    nrn_push_double = reinterpret_cast<decltype(nrn_push_double)>(dlsym(handle, "nrn_push_double"));
    nrn_new_object = reinterpret_cast<decltype(nrn_new_object)>(dlsym(handle, "nrn_new_object"));
    nrn_get_pp_property_ptr = reinterpret_cast<decltype(nrn_get_pp_property_ptr)>(
        dlsym(handle, "nrn_get_pp_property_ptr"));
    nrn_push_double_ptr = reinterpret_cast<decltype(nrn_push_double_ptr)>(
        dlsym(handle, "nrn_push_double_ptr"));
    nrn_get_rangevar_ptr = reinterpret_cast<decltype(nrn_get_rangevar_ptr)>(
        dlsym(handle, "nrn_get_rangevar_ptr"));
    nrn_call_method = reinterpret_cast<decltype(nrn_call_method)>(dlsym(handle, "nrn_call_method"));
    nrn_unref_object = reinterpret_cast<decltype(nrn_unref_object)>(
        dlsym(handle, "nrn_unref_object"));
    nrn_vector_capacity = reinterpret_cast<decltype(nrn_vector_capacity)>(
        dlsym(handle, "nrn_vector_capacity"));
    nrn_vector_data_ptr = reinterpret_cast<decltype(nrn_vector_data_ptr)>(
        dlsym(handle, "nrn_vector_data_ptr"));
    nrn_get_method_symbol = reinterpret_cast<decltype(nrn_get_method_symbol)>(
        dlsym(handle, "nrn_get_method_symbol"));
    nrn_call_method = reinterpret_cast<decltype(nrn_call_method)>(dlsym(handle, "nrn_call_method"));
    nrn_pop_object = reinterpret_cast<decltype(nrn_pop_object)>(dlsym(handle, "nrn_pop_object"));
    nrn_get_symbol_ptr = reinterpret_cast<decltype(nrn_get_symbol_ptr)>(
        dlsym(handle, "nrn_get_symbol_ptr"));
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
    nrn_push_str(&temp_str);
    nrn_call_function(nrn_get_symbol("load_file"), 1);
    nrn_pop_double();
    delete[] temp_str;


    // topology
    soma = nrn_new_section("soma");
    nrn_set_nseg(soma, 3);

    // define soma morphology with two 3d points
    nrn_push_section(soma);
    // (0, 0, 0, 10)
    nrn_push_double(0);
    nrn_push_double(0);
    nrn_push_double(0);
    nrn_push_double(10);
    nrn_call_function(nrn_get_symbol("pt3dadd"), 4);
    nrn_pop_double();  // pt3dadd returns a number
    // (10, 0, 0, 10)
    nrn_push_double(10);
    nrn_push_double(0);
    nrn_push_double(0);
    nrn_push_double(10);
    nrn_call_function(nrn_get_symbol("pt3dadd"), 4);
    nrn_pop_double();  // pt3dadd returns a number

    // ion channels
    nrn_insert_mechanism(soma, nrn_get_symbol("hh"));

    // current clamp at soma(0.5)
    nrn_push_double(0.5);
    iclamp = nrn_new_object(nrn_get_symbol("IClamp"), 1);
    *nrn_get_pp_property_ptr(iclamp, "amp") = 0.3;
    *nrn_get_pp_property_ptr(iclamp, "del") = 1;
    *nrn_get_pp_property_ptr(iclamp, "dur") = 0.1;

    // setup recording
    v = nrn_new_object(nrn_get_symbol("Vector"), 0);
    nrn_push_double_ptr(nrn_get_rangevar_ptr(soma, nrn_get_symbol("v"), 0.5));
    nrn_call_method(v, nrn_get_method_symbol(v, "record"), 1);
    nrn_unref_object(nrn_pop_object());  // record returns the vector
    t = nrn_new_object(nrn_get_symbol("Vector"), 0);
    nrn_push_double_ptr(nrn_get_symbol_ptr(nrn_get_symbol("t")));
    nrn_call_method(t, nrn_get_method_symbol(t, "record"), 1);
    nrn_unref_object(nrn_pop_object());  // record returns the vector

    // finitialize(-65)
    nrn_push_double(-65);
    nrn_call_function(nrn_get_symbol("finitialize"), 1);
    nrn_pop_double();

    // continuerun(10)
    nrn_push_double(10);
    nrn_call_function(nrn_get_symbol("continuerun"), 1);
    nrn_pop_double();

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