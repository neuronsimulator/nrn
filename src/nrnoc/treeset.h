#pragma once
double* nrn_recalc_ptr(double* old);
void nrn_register_recalc_ptr_callback(void (*f)());
