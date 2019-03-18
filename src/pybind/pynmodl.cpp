/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <memory>
#include <set>

#include <pybind11/iostream.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "parser/nmodl_driver.hpp"
#include "pybind/pybind_utils.hpp"
#include "version/version.h"
#include "visitors/visitor_utils.hpp"

namespace py = pybind11;


class PyDriver: public nmodl::parser::NmodlDriver {
  public:
    bool parse_stream(py::object object) {
        py::object tiob = py::module::import("io").attr("TextIOBase");
        if (py::isinstance(object, tiob)) {
            py::detail::pythonibuf<py::str> buf(object);
            std::istream istr(&buf);
            return NmodlDriver::parse_stream(istr);
        } else {
            py::detail::pythonibuf<py::bytes> buf(object);
            std::istream istr(&buf);
            return NmodlDriver::parse_stream(istr);
        }
    }
};

// forward declaration of submodule init functions
void init_visitor_module(py::module& m);
void init_ast_module(py::module& m);
void init_symtab_module(py::module& m);

PYBIND11_MODULE(_nmodl, m_nmodl) {
    m_nmodl.doc() = "NMODL : Source-to-Source Code Generation Framework";
    m_nmodl.attr("__version__") = nmodl::version::NMODL_VERSION;

    py::class_<PyDriver> nmodl_driver(m_nmodl, "NmodlDriver");

    // todo : what has changed ? fix this!
    // nmodl_driver.def(py::init<bool, bool>());
    nmodl_driver.def(py::init<>())
        .def("parse_string", &PyDriver::parse_string)
        .def("parse_file", &PyDriver::parse_file)
        .def("parse_stream", &PyDriver::parse_stream)
        .def("ast", &PyDriver::ast);

    m_nmodl.def("to_nmodl", nmodl::to_nmodl, py::arg("node"),
                py::arg("exclude_types") = std::set<nmodl::ast::AstNodeType>());
    m_nmodl.def("to_json", nmodl::to_json, py::arg("node"), py::arg("compact") = false,
                py::arg("expand") = false);

    init_visitor_module(m_nmodl);
    init_ast_module(m_nmodl);
    init_symtab_module(m_nmodl);
}
