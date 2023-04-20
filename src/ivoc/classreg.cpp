#include <../../nrnconf.h>
// interface c++ class to oc

#include <InterViews/resource.h>
#include <stdio.h>
#include "classreg.h"
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

/*-----------------------------------------------------*/
#if 0
//example

//the class

/*static*/ class A {
public:
	A();
	~A();
	void f1(char* s);
	double f2(double x);
private:
	double x_;
};

A::A() { printf("A::A\n"); x_=1; }
A::~A() { printf("A::~A\n"); }
void A::f1(char* s) { printf("A::f1(\"%s\")\n", s);}
double A::f2(double x) { double a = x_; x_=x; return a; }

//the interface

static void* A_constructor(Object*) {
	printf("A::constructor\n");
	return (void*) new A;
}

static void A_destructor(void* v) {
	delete (A*)v;
}

static double A_f1(void* v) {
	((A*)v)->f1(gargstr(1));
	return(0.);
}

static double A_f2(void* v) {
	double x = ((A*)v)->f2(*getarg(1));
	return(x);
}

// class registration to oc

static Member_func A_member_func[] = {
	"f1", A_f1,
	"f2", A_f2,
	0, 0
};
	
void A_reg() {
	class2oc("A", A_constructor, A_destructor, A_member_func);
}

#endif
