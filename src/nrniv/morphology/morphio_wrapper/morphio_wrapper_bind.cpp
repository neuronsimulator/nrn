#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/tuple.h>
#include <nanobind/stl/vector.h>
#include "morphio_wrapper.hpp"

namespace nb = nanobind;

namespace neuron::morphology {
extern void morphio_load(PyObject* pyObj, const MorphIOWrapper& morph);
}

void morphio_load_wrapper(nb::handle pyObj, const neuron::morphology::MorphIOWrapper& morph) {
    neuron::morphology::morphio_load(pyObj.ptr(), morph);
}

void morphio_load_file(nb::handle pyObj, const std::string& filename) {
    neuron::morphology::MorphIOWrapper morph(filename);
    neuron::morphology::morphio_load(pyObj.ptr(), morph);
}

NB_MODULE(morphio_api, m) {
    nb::class_<neuron::morphology::MorphIOWrapper>(m, "MorphIOWrapper")
        .def(nb::init<const std::string&>())
        .def("morph_as_hoc", &neuron::morphology::MorphIOWrapper::morph_as_hoc)
        .def_prop_ro("section_index_to_name",
                     &neuron::morphology::MorphIOWrapper::section_index_to_name)
        .def_prop_ro("section_type_distribution",
                     &neuron::morphology::MorphIOWrapper::section_type_distribution);
    m.def("morphio_load", &morphio_load_wrapper);
    m.def("morphio_load", &morphio_load_file);
}