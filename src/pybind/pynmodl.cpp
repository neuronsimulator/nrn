/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/
#include "ast/program.hpp"
#include "config/config.h"
#include "parser/nmodl_driver.hpp"
#include "pybind/pybind_utils.hpp"
#include "visitors/visitor_utils.hpp"

#include <pybind11/iostream.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <memory>
#include <set>

/**
 * \dir
 * \brief Python Interface Implementation
 *
 * \file
 * \brief Top level nmodl Python module implementation
 */

namespace py = pybind11;
using namespace pybind11::literals;

namespace nmodl {

/** \brief docstring of Python exposed API */
namespace docstring {

static const char* const driver = R"(
    This is the NmodlDriver class documentation
)";

static const char* const driver_ast = R"(
    Get ast

    Returns:
        Instance of :py:class:`Program`
)";

static const char* const driver_parse_string = R"(
    Parse NMODL provided as a string

    Args:
        input (str): C code as string
    Returns:
        AST: ast root node if success, throws an exception otherwise

    >>> ast = driver.parse_string("DEFINE NSTEP 6")
)";

static const char* const driver_parse_file = R"(
    Parse NMODL provided as a file

    Args:
        filename (str): name of the C file

    Returns:
        AST: ast root node if success, throws an exception otherwise
)";

static const char* const driver_parse_stream = R"(
    Parse NMODL file provided as istream

    Args:
        in (file): ifstream object

    Returns:
        AST: ast root node if success, throws an exception otherwise
)";

static const char* const to_nmodl = R"(
    Given AST node, return the NMODL string representation

    Args:
        node (AST): AST node
        excludeTypes (set of AstNodeType): Excluded node types

    Returns:
        str: NMODL string representation

    >>> ast = driver.parse_string("NEURON{}")
    >>> nmodl.to_nmodl(ast)
    'NEURON {\n}\n'
)";

static const char* const to_json = R"(
    Given AST node, return the JSON string representation

    Args:
        node (AST): AST node
        compact (bool): Compact node
        expand (bool): Expand node

    Returns:
        str: JSON string representation

    >>> ast = driver.parse_string("NEURON{}")
    >>> nmodl.to_json(ast, True)
    '{"Program":[{"NeuronBlock":[{"StatementBlock":[]}]}]}'
)";

}  // namespace docstring

/**
 * \class PyNmodlDriver
 * \brief Class to bridge C++ NmodlDriver with Python world using pybind11
 */
class PyNmodlDriver: public nmodl::parser::NmodlDriver {
  public:
    std::shared_ptr<nmodl::ast::Program> parse_stream(py::object const& object) {
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

}  // namespace nmodl

// forward declaration of submodule init functions
void init_visitor_module(py::module& m);
void init_ast_module(py::module& m);
void init_symtab_module(py::module& m);

PYBIND11_MODULE(_nmodl, m_nmodl) {
    m_nmodl.doc() = "NMODL : Source-to-Source Code Generation Framework";
    m_nmodl.attr("__version__") = nmodl::Version::NMODL_VERSION;

    py::class_<nmodl::parser::NmodlDriver> _{m_nmodl, "nmodl::parser::NmodlDriver"};
    py::class_<nmodl::PyNmodlDriver, nmodl::parser::NmodlDriver> nmodl_driver(
        m_nmodl, "NmodlDriver", nmodl::docstring::driver);
    nmodl_driver.def(py::init<>())
        .def("parse_string",
             &nmodl::PyNmodlDriver::parse_string,
             "input"_a,
             nmodl::docstring::driver_parse_string)
        .def(
            "parse_file",
            [](nmodl::PyNmodlDriver& driver, const std::string& file) {
                return driver.parse_file(file, nullptr);
            },
            "filename"_a,
            nmodl::docstring::driver_parse_file)
        .def("parse_stream",
             &nmodl::PyNmodlDriver::parse_stream,
             "in"_a,
             nmodl::docstring::driver_parse_stream)
        .def("get_ast", &nmodl::PyNmodlDriver::get_ast, nmodl::docstring::driver_ast);

    m_nmodl.def("to_nmodl",
                static_cast<std::string (*)(const nmodl::ast::Ast&,
                                            const std::set<nmodl::ast::AstNodeType>&)>(
                    nmodl::to_nmodl),
                "node"_a,
                "exclude_types"_a = std::set<nmodl::ast::AstNodeType>(),
                nmodl::docstring::to_nmodl);
    m_nmodl.def("to_json",
                nmodl::to_json,
                "node"_a,
                "compact"_a = false,
                "expand"_a = false,
                "add_nmodl"_a = false,
                nmodl::docstring::to_json);

    init_visitor_module(m_nmodl);
    init_ast_module(m_nmodl);
    init_symtab_module(m_nmodl);
}
