/* NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH */
#include "neuronapi.h"
#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <assert.h>

static const char* argv[] = {"sections", "-nogui", "-nopython", NULL};

void modl_reg(){};

void setup_neuron_api(void) {
    void* handle = dlopen("libnrniv.dylib", RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        handle = dlopen("libnrniv.so", RTLD_NOW | RTLD_LOCAL);
        if (!handle) {
            printf("Couldn't open NEURON library.\n%s\n", dlerror());
            exit(-1);
        }
    }
    nrn_init = (int (*)(int, const char**))(dlsym(handle, "nrn_init"));
    assert(nrn_init); /* NOTE: this function only exists in versions of NEURON with the API defined
                       */
    nrn_section_new = (Section * (*) (char const*) )(dlsym(handle, "nrn_section_new"));
    nrn_section_connect = (void (*)(Section*, double, Section*, double))(
        dlsym(handle, "nrn_section_connect"));
    nrn_nseg_set = (void (*)(Section*, int))(dlsym(handle, "nrn_nseg_set"));
    nrn_function_call = (void (*)(Symbol*, int))(dlsym(handle, "nrn_function_call"));
    nrn_symbol = (Symbol * (*) (char const*) )(dlsym(handle, "nrn_symbol"));
    nrn_object_new = (Object * (*) (Symbol*, int) )(dlsym(handle, "nrn_object_new"));
    nrn_push_section = (void (*)(Section*))(dlsym(handle, "nrn_push_section"));
    nrn_pop_section = (void (*)(void))(dlsym(handle, "nrn_pop_section"));
    nrn_method_symbol = (Symbol * (*) (Object*, char const*) )(dlsym(handle, "nrn_method_symbol"));
    nrn_method_call = (void (*)(Object*, Symbol*, int))(dlsym(handle, "nrn_method_call"));
    nrn_new_sectionlist_iterator = (SectionListIterator * (*) (nrn_Item*) )(
        dlsym(handle, "nrn_new_sectionlist_iterator"));
    nrn_get_allsec = (nrn_Item * (*) (void) )(dlsym(handle, "nrn_get_allsec"));
    nrn_sectionlist_iterator_done = (int (*)(SectionListIterator*))(
        dlsym(handle, "nrn_sectionlist_iterator_done"));
    nrn_sectionlist_iterator_next = (Section * (*) (SectionListIterator*) )(
        dlsym(handle, "nrn_sectionlist_iterator_next"));
    nrn_free_sectionlist_iterator = (void (*)(SectionListIterator*))(
        dlsym(handle, "nrn_free_sectionlist_iterator"));
    nrn_secname = (char const* (*) (Section*) )(dlsym(handle, "nrn_secname"));
    nrn_sectionlist_data_get = (nrn_Item *
                                (*) (Object*) )(dlsym(handle, "nrn_sectionlist_data_get"));
}

int main(void) {
    setup_neuron_api();
    nrn_init(3, argv);

    // topology
    Section* soma = nrn_section_new("soma");
    Section* dend1 = nrn_section_new("dend1");
    Section* dend2 = nrn_section_new("dend2");
    Section* dend3 = nrn_section_new("dend3");
    Section* axon = nrn_section_new("axon");
    nrn_section_connect(dend1, 0, soma, 1);
    nrn_section_connect(dend2, 0, dend1, 1);
    nrn_section_connect(dend3, 0, dend1, 1);
    nrn_section_connect(axon, 0, soma, 0);
    nrn_nseg_set(axon, 5);

    // print out the morphology
    nrn_function_call(nrn_symbol("topology"), 0);

    /* create a SectionList that is dend1 and its children */
    Object* seclist = nrn_object_new(nrn_symbol("SectionList"), 0);
    nrn_push_section(dend1);
    nrn_method_call(seclist, nrn_method_symbol(seclist, "subtree"), 0);
    nrn_pop_section();

    /* loop over allsec, print out */
    printf("allsec:\n");
    SectionListIterator* sli = nrn_new_sectionlist_iterator(nrn_get_allsec());
    for (; !nrn_sectionlist_iterator_done(sli);) {
        Section* sec = nrn_sectionlist_iterator_next(sli);
        printf("    %s\n", nrn_secname(sec));
    }
    nrn_free_sectionlist_iterator(sli);

    printf("\ndend1's subtree:\n");
    sli = nrn_new_sectionlist_iterator(nrn_sectionlist_data_get(seclist));
    for (; !nrn_sectionlist_iterator_done(sli);) {
        Section* sec = nrn_sectionlist_iterator_next(sli);
        printf("    %s\n", nrn_secname(sec));
    }
    nrn_free_sectionlist_iterator(sli);
}