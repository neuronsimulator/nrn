#pragma once
#include <InterViews/observe.h>
#include "neuron/container/data_handle.hpp"

#include <cstddef>  // std::size_t

void nrn_notify_freed(void (*pf)(void*, int));
void nrn_notify_when_void_freed(void* p, Observer* ob);
void nrn_notify_when_double_freed(double* p, Observer* ob);
void nrn_notify_pointer_disconnect(Observer* ob);
void notify_pointer_freed(void* pt);
void notify_freed(void* p);
void notify_freed_val_array(double* p, std::size_t);

namespace neuron::container {
void notify_when_handle_dies(data_handle<double>, Observer*);
}
