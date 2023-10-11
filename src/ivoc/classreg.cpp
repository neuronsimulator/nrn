#include <../../nrnconf.h>
// interface c++ class to oc

#include <InterViews/resource.h>
#include <stdio.h>
#include "hoc_membf.h"
#ifndef OC_CLASSES
#define OC_CLASSES "occlass.h"
#endif

#define EXTERNS 1
extern void
#include OC_CLASSES
    ;

#undef EXTERNS
static void (*register_classes[])() = {
#include OC_CLASSES
    ,
    0};

void hoc_class_registration(void) {
    for (int i = 0; register_classes[i]; i++) {
        (*register_classes[i])();
    }
}
