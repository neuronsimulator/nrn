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
        printf("Couldn't open dylib.\n%s\n", dlerror());
        exit(-1);
    }
    nrn_init = (int (*)(int, const char**))(dlsym(handle, "nrn_init"));
    assert(nrn_init); /* NOTE: this function only exists in versions of NEURON with the API defined
                       */
    nrn_new_section = (Section * (*) (char const*) )(dlsym(handle, "nrn_new_section"));
    nrn_connect_sections = (void (*)(Section*, double, Section*, double))(
        dlsym(handle, "nrn_connect_sections"));
    nrn_set_nseg = (void (*)(Section*, int))(dlsym(handle, "nrn_set_nseg"));
    nrn_call_function = (void (*)(Symbol*, int))(dlsym(handle, "nrn_call_function"));
    nrn_get_symbol = (Symbol * (*) (char const*) )(dlsym(handle, "nrn_get_symbol"));
    nrn_new_object = (Object * (*) (Symbol*, int) )(dlsym(handle, "nrn_new_object"));
    nrn_push_section = (void (*)(Section*))(dlsym(handle, "nrn_push_section"));
    nrn_pop_section = (void (*)(void))(dlsym(handle, "nrn_pop_section"));
    nrn_get_method_symbol = (Symbol *
                             (*) (Object*, char const*) )(dlsym(handle, "nrn_get_method_symbol"));
    nrn_call_method = (void (*)(Object*, Symbol*, int))(dlsym(handle, "nrn_call_method"));
    nrn_new_sectionlist_iterator = (SectionListIterator * (*) (hoc_Item*) )(
        dlsym(handle, "nrn_new_sectionlist_iterator"));
    nrn_get_allsec = (hoc_Item * (*) (void) )(dlsym(handle, "nrn_get_allsec"));
    nrn_sectionlist_iterator_done = (int (*)(SectionListIterator*))(
        dlsym(handle, "nrn_sectionlist_iterator_done"));
    nrn_sectionlist_iterator_next = (Section * (*) (SectionListIterator*) )(
        dlsym(handle, "nrn_sectionlist_iterator_next"));
    nrn_free_sectionlist_iterator = (void (*)(SectionListIterator*))(
        dlsym(handle, "nrn_free_sectionlist_iterator"));
    nrn_secname = (char const* (*) (Section*) )(dlsym(handle, "nrn_secname"));
    nrn_get_sectionlist_data = (hoc_Item *
                                (*) (Object*) )(dlsym(handle, "nrn_get_sectionlist_data"));
}

int main(void) {
    setup_neuron_api();
    nrn_init(3, argv);

    // topology
    Section* soma = nrn_new_section("soma");
    Section* dend1 = nrn_new_section("dend1");
    Section* dend2 = nrn_new_section("dend2");
    Section* dend3 = nrn_new_section("dend3");
    Section* axon = nrn_new_section("axon");
    nrn_connect_sections(dend1, 0, soma, 1);
    nrn_connect_sections(dend2, 0, dend1, 1);
    nrn_connect_sections(dend3, 0, dend1, 1);
    nrn_connect_sections(axon, 0, soma, 0);
    nrn_set_nseg(axon, 5);

    // print out the morphology
    nrn_call_function(nrn_get_symbol("topology"), 0);

    /* create a SectionList that is dend1 and its children */
    Object* seclist = nrn_new_object(nrn_get_symbol("SectionList"), 0);
    nrn_push_section(dend1);
    nrn_call_method(seclist, nrn_get_method_symbol(seclist, "subtree"), 0);
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
    sli = nrn_new_sectionlist_iterator(nrn_get_sectionlist_data(seclist));
    for (; !nrn_sectionlist_iterator_done(sli);) {
        Section* sec = nrn_sectionlist_iterator_next(sli);
        printf("    %s\n", nrn_secname(sec));
    }
    nrn_free_sectionlist_iterator(sli);
}