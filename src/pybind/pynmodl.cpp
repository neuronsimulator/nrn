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

#include "config/config.h"
#include "parser/nmodl_driver.hpp"
#include "pybind/pybind_utils.hpp"
#include "visitors/visitor_utils.hpp"

namespace py = pybind11;
using pybind11::literals::operator""_a;

/** \brief docstring of Python symbols */
namespace docstring {

static const char* driver = R"(
    This is the NmodlDriver class documentation
)";

static const char* driver_ast = R"(
    Get ast

    Returns:
        Instance of :py:class:`Program`
)";

static const char* driver_parse_string = R"(
    Parse C provided as a string (testing)

    Args:
        input (str): C code as string
    Returns:
        bool: true if success, false otherwise

    >>> driver.parse_string("DEFINE NSTEP 6")
    True
)";

static const char* driver_parse_file = R"(
    Parse C file

    Args:
        filename (str): name of the C file

    Returns:
        bool: true if success, false otherwise
)";

static const char* driver_parse_stream = R"(
    Parse C file provided as istream

    Args:
        in (file): ifstream object

    Returns:
        bool: true if success, false otherwise
)";

static const char* to_nmodl = R"(
    Given AST node, return the JSON string representation

    Args:
        node (AST): AST node
        excludeTypes (set of AstNodeType): Excluded node types

    Returns:
        str: JSON string representation
)";

static const char* to_json = R"(
    Given AST node, return the NMODL string representation

    Args:
        node (AST): AST node
        compact (bool): Compact node
        expand (bool): Expand node

    Returns:
        str: NMODL string representation

    >>> driver.parse_string("NEURON{}")
    True
    >>> ast = driver.ast()
    >>> nmodl.to_json(ast, True)
    '{"Program":[{"NeuronBlock":[{"StatementBlock":[]}]}]}'
)";

}  // namespace docstring

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

    py::class_<PyDriver> nmodl_driver(m_nmodl, "NmodlDriver", docstring::driver);
    nmodl_driver.def(py::init<>())
        .def("parse_string", &PyDriver::parse_string, "input"_a, docstring::driver_parse_string)
        .def("parse_file", &PyDriver::parse_file, "filename"_a, docstring::driver_parse_file)
        .def("parse_stream", &PyDriver::parse_stream, "in"_a, docstring::driver_parse_stream)
        .def("ast", &PyDriver::ast, docstring::driver_ast);

    m_nmodl.def("to_nmodl", nmodl::to_nmodl, "node"_a,
                "exclude_types"_a = std::set<nmodl::ast::AstNodeType>(), docstring::to_nmodl);
    m_nmodl.def("to_json", nmodl::to_json, "node"_a, "compact"_a = false, "expand"_a = false,
                docstring::to_json);

    init_visitor_module(m_nmodl);
    init_ast_module(m_nmodl);
    init_symtab_module(m_nmodl);
}
