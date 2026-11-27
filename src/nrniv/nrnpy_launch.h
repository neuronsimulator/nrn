#pragma once
namespace neuron::python {
/// Let NEURON start Python on its first use.
void register_launcher();
/// A dlopen handle of the libpython to run, null if libnrniv is linked to it. Throws if there is
/// none.
void* nrnpy_open_libpython();
}  // namespace neuron::python
