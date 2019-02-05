#include <memory>
#include <pybind11/iostream.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "parser/nmodl_driver.hpp"
#include "pybind/pybind_utils.hpp"
#include "visitors/visitor_utils.hpp"

namespace py = pybind11;


class PyDriver: public nmodl::Driver {
  public:
    using nmodl::Driver::Driver;

    bool parse_stream(py::object object) {
        py::object tiob = py::module::import("io").attr("TextIOBase");
        if (py::isinstance(object, tiob)) {
            py::detail::pythonibuf<py::str> buf(object);
            std::istream istr(&buf);
            return nmodl::Driver::parse_stream(istr);
        } else {
            py::detail::pythonibuf<py::bytes> buf(object);
            std::istream istr(&buf);
            return nmodl::Driver::parse_stream(istr);
        }
    }
};

// forward declaration of submodule init functions
void init_visitor_module(py::module& m);
void init_ast_module(py::module& m);
void init_symtab_module(py::module& m);

PYBIND11_MODULE(_nmodl, m_nmodl) {
    py::class_<PyDriver> nmodl_driver(m_nmodl, "Driver");
    nmodl_driver.def(py::init<bool, bool>());
    nmodl_driver.def(py::init<>())
        .def("parse_string", &PyDriver::parse_string)
        .def("parse_file", &PyDriver::parse_file)
        .def("parse_stream", &PyDriver::parse_stream)
        .def("ast", &PyDriver::ast);

    m_nmodl.def("to_nmodl", nmodl::to_nmodl);
    m_nmodl.def("to_json", nmodl::to_json,
            py::arg("node"), py::arg("compact") = false);

    init_visitor_module(m_nmodl);
    init_ast_module(m_nmodl);
    init_symtab_module(m_nmodl);
}
