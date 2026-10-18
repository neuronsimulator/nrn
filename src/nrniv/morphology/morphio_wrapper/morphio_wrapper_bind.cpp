#include <nanobind/nanobind.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/tuple.h>
#include <nanobind/stl/vector.h>
#include "morphio_wrapper.hpp"

namespace nb = nanobind;
using namespace nb::literals;

namespace neuron::morphology {
extern void morphio_load(PyObject* pyObj, const MorphIOWrapper& morph);
}

namespace {
std::string get_path_string(nb::handle path_obj) {
    if (nb::isinstance<nb::str>(path_obj)) {
        return nb::cast<std::string>(path_obj);
    }
    if (nb::hasattr(path_obj, "__fspath__")) {
        return nb::cast<std::string>(path_obj.attr("__fspath__")());
    }
    throw nb::type_error("Expected str or os.PathLike object for morphology path");
}
}  // namespace

void morphio_load_wrapper(nb::handle pyObj, const neuron::morphology::MorphIOWrapper& morph) {
    neuron::morphology::morphio_load(pyObj.ptr(), morph);
}

void morphio_load_file(nb::handle pyObj, nb::handle path_obj) {
    neuron::morphology::MorphIOWrapper morph(get_path_string(path_obj));
    neuron::morphology::morphio_load(pyObj.ptr(), morph);
}

NB_MODULE(morphio_api, m) {
    nb::class_<neuron::morphology::MorphIOWrapper>(m, "MorphIOWrapper")
        .def(
            "__init__",
            [](neuron::morphology::MorphIOWrapper* self, nb::handle path_obj) {
                new (self) neuron::morphology::MorphIOWrapper(get_path_string(path_obj));
            },
            "path"_a,
            "Create a MorphIOWrapper from a file path.")
        .def("to_hoc",
             &neuron::morphology::MorphIOWrapper::to_hoc,
             "Return list of HOC statements defining the morphology.")
        .def(
            "instantiate",
            [](const neuron::morphology::MorphIOWrapper& self, nb::handle cell) {
                neuron::morphology::morphio_load(cell.ptr(), self);
            },
            "cell"_a,
            "Instantiate the morphology into a NEURON cell object.")
        .def_prop_ro("filename",
                     &neuron::morphology::MorphIOWrapper::filename,
                     "Path to the loaded morphology file.")
        .def_prop_ro("section_names",
                     &neuron::morphology::MorphIOWrapper::section_names,
                     "List of section names in the morphology.")
        .def_prop_ro("section_counts",
                     &neuron::morphology::MorphIOWrapper::section_counts,
                     "Dictionary mapping section types to their counts.")
        .def_prop_ro("section_type_distribution",
                     &neuron::morphology::MorphIOWrapper::section_type_distribution,
                     "Raw section type distribution as (type_id, start_id, count) tuples.")
        .def("__len__",
             [](const neuron::morphology::MorphIOWrapper& self) {
                 return self.section_names().size();
             })
        .def("__getitem__",
             [](const neuron::morphology::MorphIOWrapper& self, size_t index) {
                 if (index >= self.section_names().size()) {
                     throw nb::index_error("Section index out of range");
                 }
                 return self.section_names()[index];
             })
        .def("__repr__", [](const neuron::morphology::MorphIOWrapper& self) {
            return "<MorphIOWrapper '" + self.filename() +
                   "' (sections=" + std::to_string(self.section_names().size()) + ")>";
        });

    m.def("morphio_load",
          &morphio_load_wrapper,
          "cell"_a,
          "morph"_a,
          "Load morphology from MorphIOWrapper into cell.");
    m.def("morphio_load",
          &morphio_load_file,
          "cell"_a,
          "filename"_a,
          "Load morphology from file into cell.");
    m.def("load",
          &morphio_load_wrapper,
          "cell"_a,
          "morph"_a,
          "Load morphology from MorphIOWrapper into cell.");
    m.def(
        "load", &morphio_load_file, "cell"_a, "filename"_a, "Load morphology from file into cell.");
}