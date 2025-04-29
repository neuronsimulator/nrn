/* NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH */
#include "neuronapi.h"
#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <assert.h>

extern "C" void modl_reg(){};

int main(void) {
    static const char* argv[] = {"sections", "-nogui", "-nopython", NULL};
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
    nrn_section_push(dend1);
    nrn_method_call(seclist, nrn_method_symbol(seclist, "subtree"), 0);
    nrn_section_pop();

    /* loop over allsec, print out */
    printf("allsec:\n");
    SectionListIterator* sli = nrn_sectionlist_iterator_new(nrn_allsec());
    while (!nrn_sectionlist_iterator_done(sli)) {
        Section* sec = nrn_sectionlist_iterator_next(sli);
        printf("    %s\n", nrn_secname(sec));
    }
    nrn_sectionlist_iterator_free(sli);

    printf("\ndend1's subtree:\n");
    sli = nrn_sectionlist_iterator_new(nrn_sectionlist_data(seclist));
    while (!nrn_sectionlist_iterator_done(sli)) {
        Section* sec = nrn_sectionlist_iterator_next(sli);
        printf("    %s\n", nrn_secname(sec));
    }
    nrn_sectionlist_iterator_free(sli);
}
