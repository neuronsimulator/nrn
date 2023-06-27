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
void nrn_str_push(char** str);
void nrn_function_call(Symbol* sym, int narg);
double nrn_double_pop(void);
Section* nrn_section_new(char const* const name);
Symbol* nrn_symbol(char const* const name);
void nrn_mechanism_insert(Section* sec, Symbol* mechanism);
Object* nrn_object_new(Symbol* sym, int narg);
void nrn_method_call(Object* obj, Symbol* method_sym, int narg);
void nrn_property_set(Object* obj, const char* name, double value);
void nrn_property_array_set(Object* obj, const char* name, int i, double value);
void nrn_double_push(double val);
void nrn_object_push(Object* obj);
Symbol* nrn_method_symbol(Object* obj, char const* const name);
Object* nrn_object_pop(void);
void nrn_object_unref(Object* obj);
double* nrn_vector_data(Object* vec);
int nrn_vector_capacity(Object* vec);
double* nrn_symbol_ptr(Symbol* sym);
void nrn_rangevar_push(Symbol* const sym, Section* const sec, double x);
void nrn_symbol_push(Symbol* sym);
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

    double* tvec = nrn_vector_data(t);
    double* vvec = nrn_vector_data(v);
    ofstream out_file;
    out_file.open("netcon.csv");
    for (auto i = 0; i < nrn_vector_capacity(t); i++) {
        out_file << tvec[i] << "," << vvec[i] << endl;
    }
    out_file.close();
    cout << "Results saved to netcon.csv" << endl;

    dlclose(handle);
}