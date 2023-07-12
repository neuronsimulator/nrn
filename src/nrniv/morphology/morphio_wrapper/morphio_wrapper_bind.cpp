#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "morphio_wrapper.hpp"
#include <iostream>

namespace py = pybind11;

namespace neuron::morphology {
extern void morphio_load(PyObject* pyObj, MorphIOWrapper& morph);
}

void morphio_load(const py::object& pyObj, const py::object& pyMorph) {
    // Extract the MorphIOWrapper object from the Python object
    neuron::morphology::MorphIOWrapper& morph = pyMorph.cast<neuron::morphology::MorphIOWrapper&>();

    // call morphio_load
    std::cout << "morphio_load(pyobj, miorwapper) called" << std::endl;
    morphio_load(pyObj.ptr(), morph);
}

PYBIND11_MODULE(morphio_api, m) {
    py::class_<neuron::morphology::MorphIOWrapper>(m, "MorphIOWrapper")
        .def(py::init<const std::string&>())
        .def("morph_as_hoc", &neuron::morphology::MorphIOWrapper::morph_as_hoc)
        .def_property_readonly("sec_idx2names", &neuron::morphology::MorphIOWrapper::sec_idx2names)
        .def_property_readonly("sec_typeid_distrib",
                               &neuron::morphology::MorphIOWrapper::sec_typeid_distrib);
    m.def("morphio_load", &morphio_load);
}