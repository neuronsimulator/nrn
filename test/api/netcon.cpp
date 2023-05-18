// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
#include <cstring>
#include <iostream>
#include <fstream>
#include <dlfcn.h>

using std::cout;
using std::endl;
using std::ofstream;

typedef struct Symbol Symbol;
typedef struct Object Object;
typedef struct Section Section;
typedef struct SectionListIterator SectionListIterator;
typedef struct nrn_Item nrn_Item;
typedef struct SymbolTableIterator SymbolTableIterator;
typedef struct Symlist Symlist;

extern "C" {
int nrn_init(int argc, const char** argv);
void nrn_push_str(char** str);
void nrn_call_function(Symbol* sym, int narg);
double nrn_pop_double(void);
Section* nrn_new_section(char const* const name);
Symbol* nrn_get_symbol(char const* const name);
void nrn_insert_mechanism(Section* sec, Symbol* mechanism);
Object* nrn_new_object(Symbol* sym, int narg);
void nrn_call_method(Object* obj, Symbol* method_sym, int narg);
double* nrn_get_pp_property_ptr(Object* pp, const char* name);
void nrn_push_double(double val);
void nrn_push_object(Object* obj);
Symbol* nrn_get_method_symbol(Object* obj, char const* const name);
Object* nrn_pop_object(void);
void nrn_unref_object(Object* obj);
double* nrn_vector_data_ptr(Object* vec);
int nrn_vector_capacity(Object* vec);
double* nrn_get_steered_property_ptr(Object* obj, const char* name);
double* nrn_get_rangevar_ptr(Section* const sec, Symbol* const sym, double const x);
void nrn_push_double_ptr(double* addr);
double* nrn_get_symbol_ptr(Symbol* sym);
}

static const char* argv[] = {"netcon", "-nogui", "-nopython", nullptr};

extern "C" void modl_reg(){};

int main(void) {
    Object* v;
    Object* t;
    char* temp_str;

    // Note: cannot use RTLD_LOCAL here
    void* handle = dlopen("libnrniv.dylib", RTLD_NOW);
    if (!handle) {
        handle = dlopen("libnrniv.so", RTLD_NOW);
        if (!handle) {
            cout << "Couldn't open NEURON library." << endl << dlerror() << endl;
            exit(-1);
        }
    }

    nrn_init(3, argv);

    // load the stdrun library
    temp_str = new char[11];
    strcpy(temp_str, "stdrun.hoc");
    nrn_push_str(&temp_str);
    nrn_call_function(nrn_get_symbol("load_file"), 1);
    nrn_pop_double();
    delete[] temp_str;


    // topology
    auto soma = nrn_new_section("soma");

    // ion channels
    nrn_insert_mechanism(soma, nrn_get_symbol("hh"));

    // NetStim
    auto ns = nrn_new_object(nrn_get_symbol("NetStim"), 0);
    *nrn_get_pp_property_ptr(ns, "start") = 5;
    *nrn_get_pp_property_ptr(ns, "noise") = 1;
    *nrn_get_pp_property_ptr(ns, "interval") = 5;
    *nrn_get_pp_property_ptr(ns, "number") = 10;

    // syn = h.ExpSyn(soma(0.5))
    nrn_push_double(0.5);
    auto syn = nrn_new_object(nrn_get_symbol("ExpSyn"), 1);
    *nrn_get_pp_property_ptr(syn, "tau") = 3;  // 3 ms timeconstant
    *nrn_get_pp_property_ptr(syn, "e") = 0;    // 0 mV reversal potential (excitatory synapse)

    // nc = h.NetCon(ns, syn)
    nrn_push_object(ns);
    nrn_push_object(syn);
    auto nc = nrn_new_object(nrn_get_symbol("NetCon"), 2);
    nrn_get_steered_property_ptr(nc, "weight")[0] = 0.5;
    *nrn_get_steered_property_ptr(nc, "delay") = 0;

    auto vec = nrn_new_object(nrn_get_symbol("Vector"), 0);

    // nc.record(vec)
    nrn_push_object(vec);
    nrn_call_method(nc, nrn_get_method_symbol(nc, "record"), 1);
    // TODO: record probably put something on the stack that should be removed

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

    // continuerun(100)
    nrn_push_double(100);
    nrn_call_function(nrn_get_symbol("continuerun"), 1);
    nrn_pop_double();

    double* tvec = nrn_vector_data_ptr(t);
    double* vvec = nrn_vector_data_ptr(v);
    ofstream out_file;
    out_file.open("netcon.csv");
    for (auto i = 0; i < nrn_vector_capacity(t); i++) {
        out_file << tvec[i] << "," << vvec[i] << endl;
    }
    out_file.close();
    cout << "Results saved to netcon.csv" << endl;

    dlclose(handle);
}