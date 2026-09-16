/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

///
/// THIS FILE IS GENERATED AT BUILD TIME AND SHALL NOT BE EDITED.
///

#include "pybind/pyvisitor.hpp"

#include <memory>
#include <nanobind/nanobind.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/trampoline.h>

#include "ast/all.hpp"
#include "pybind/pybind_utils.hpp"
#include "visitors/constant_folder_visitor.hpp"
#include "visitors/inline_visitor.hpp"
#include "visitors/kinetic_block_visitor.hpp"
#include "visitors/local_var_rename_visitor.hpp"
#include "visitors/lookup_visitor.hpp"
#include "visitors/nmodl_visitor.hpp"
#include "visitors/matexp_visitor.hpp"
#include "visitors/symtab_visitor.hpp"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCDFAInspection"

/**
 * \file
 * \brief Visitor related Python bindings
 */

namespace nmodl {
namespace docstring {

static const char* visitor_class = R"(
    Base Visitor class
)";

static const char* ast_visitor_class = R"(
    AstVisitor class
)";

static const char* ast_lookup_visitor_class = R"(
    AstLookupVisitor class

    Attributes:
        types (list of :class:`AstNodeType`): node types to search in the AST
        nodes (list of AST): matching nodes found in the AST
)";

static const char* nmodl_print_visitor_class = R"(
    NmodlPrintVisitor class
)";

static const char* constant_folder_visitor_class = R"(
    ConstantFolderVisitor class
)";

static const char* inline_visitor_class = R"(
    InlineVisitor class
)";

static const char* kinetic_block_visitor_class = R"(
    KineticBlockVisitor class
)";

static const char* local_var_rename_visitor_class = R"(
    LocalVarRenameVisitor class
)";

static const char* matexp_visitor_class = R"(
    MatexpVisitor class
)";

}  // namespace docstring
}  // namespace nmodl


using namespace nanobind::literals;
namespace nb = nanobind;


{% for node in nodes %
}
void PyVisitor::visit_{{node.class_name | snake_case}}(ast::{{node.class_name}} & node) {
    nanobind::detail::ticket nb_ticket(nb_trampoline,
                                       "visit_{{ node.class_name|snake_case }}",
                                       true);
    nb_trampoline.base().attr(nb_ticket.key)(nanobind::cast(node, nanobind::rv_policy::reference));
}
{ % endfor % }

{% for node in nodes %
}
void PyAstVisitor::visit_{{node.class_name | snake_case}}(ast::{{node.class_name}} & node) {
    nanobind::detail::ticket nb_ticket(nb_trampoline,
                                       "visit_{{ node.class_name|snake_case }}",
                                       false);
    if (nb_ticket.key.is_valid()) {
        nb_trampoline.base().attr(nb_ticket.key)(
            nanobind::cast(node, nanobind::rv_policy::reference));
        return;
    }
    NBBase::visit_{{node.class_name | snake_case}}(node);
}
{ % endfor % }

{% for node in nodes %
}
void PyConstVisitor::visit_{{node.class_name | snake_case}}(const ast::{{node.class_name}} & node) {
    nanobind::detail::ticket nb_ticket(nb_trampoline,
                                       "visit_{{ node.class_name|snake_case }}",
                                       true);
    nb_trampoline.base().attr(nb_ticket.key)(nanobind::cast(node, nanobind::rv_policy::reference));
}
{ % endfor % }

{% for node in nodes %
}
void PyConstAstVisitor::visit_{{node.class_name | snake_case}}(const ast::{{node.class_name}} &
                                                               node) {
    nanobind::detail::ticket nb_ticket(nb_trampoline,
                                       "visit_{{ node.class_name|snake_case }}",
                                       false);
    if (nb_ticket.key.is_valid()) {
        nb_trampoline.base().attr(nb_ticket.key)(
            nanobind::cast(node, nanobind::rv_policy::reference));
        return;
    }
    NBBase::visit_{{node.class_name | snake_case}}(node);
}
{ % endfor % }


/**
 * \brief Class mirroring nmodl::NmodlPrintVisitor for Python bindings
 */
class PyNmodlPrintVisitor: private VisitorOStreamResources, public NmodlPrintVisitor {
  public:
    using NmodlPrintVisitor::NmodlPrintVisitor;
    using VisitorOStreamResources::flush;

    PyNmodlPrintVisitor() = default;

    PyNmodlPrintVisitor(std::string filename)
        : NmodlPrintVisitor(filename){};

    PyNmodlPrintVisitor(nb::object object)
        : VisitorOStreamResources(object)
        , NmodlPrintVisitor(*ostream){};

    {% for node in nodes %
    }
    void visit_{{node.class_name | snake_case}}(const ast::{{node.class_name}} & node) override {
        NmodlPrintVisitor::visit_{{node.class_name | snake_case}}(node);
        flush();
    }
    { % endfor % }
};


void init_visitor_module(nb::module_& m) {
    nb::module_ m_visitor = m.def_submodule("visitor");

    nb::class_<Visitor, PyVisitor> visitor(m_visitor, "Visitor", docstring::visitor_class);
    visitor.def(nb::init<>()) {% for node in nodes %
    }
    .def("visit_{{ node.class_name | snake_case }}",
         &Visitor::visit_{{node.class_name | snake_case}}) {
        % if loop.last - %
    };
    { % endif % }
    { % endfor % }

    nb::class_<ConstVisitor, PyConstVisitor> const_visitor(m_visitor,
                                                           "ConstVisitor",
                                                           docstring::visitor_class);
    const_visitor.def(nb::init<>()) {% for node in nodes %
    }
    .def("visit_{{ node.class_name | snake_case }}",
         &ConstVisitor::visit_{{node.class_name | snake_case}}) {
        % if loop.last - %
    };
    { % endif % }
    { % endfor % }

    nb::class_<ConstAstVisitor, ConstVisitor, PyConstAstVisitor> const_ast_visitor(
        m_visitor, "ConstAstVisitor", docstring::ast_visitor_class);
    const_ast_visitor.def(nb::init<>()) {% for node in nodes %
    }
    .def("visit_{{ node.class_name | snake_case }}",
         &ConstAstVisitor::visit_{{node.class_name | snake_case}}) {
        % if loop.last - %
    };
    { % endif % }
    { % endfor % }

    nb::class_<AstVisitor, Visitor, PyAstVisitor> ast_visitor(m_visitor,
                                                              "AstVisitor",
                                                              docstring::ast_visitor_class);
    ast_visitor.def(nb::init<>()) {% for node in nodes %
    }
    .def("visit_{{ node.class_name | snake_case }}",
         &AstVisitor::visit_{{node.class_name | snake_case}}) {
        % if loop.last - %
    };
    { % endif % }
    { % endfor % }

    nb::class_<PyNmodlPrintVisitor, ConstVisitor> nmodl_visitor(
        m_visitor, "NmodlPrintVisitor", docstring::nmodl_print_visitor_class);
    nmodl_visitor.def(nb::init<std::string>());
    nmodl_visitor.def(nb::init<nb::object>());
    nmodl_visitor.def(nb::init<>()) {% for node in nodes %
    }
    .def("visit_{{ node.class_name | snake_case }}",
         &PyNmodlPrintVisitor::visit_{{node.class_name | snake_case}}) {
        % if loop.last - %
    };
    { % endif % }
    { % endfor % }

    nb::class_<AstLookupVisitor, Visitor> lookup_visitor(m_visitor,
                                                         "AstLookupVisitor",
                                                         docstring::ast_lookup_visitor_class);
    lookup_visitor.def(nb::init<>())
        .def(nb::init<ast::AstNodeType>())
        .def("get_nodes", &AstLookupVisitor::get_nodes)
        .def("clear", &AstLookupVisitor::clear)
        .def("lookup",
             [](AstLookupVisitor& v, std::shared_ptr<ast::Ast> n)
                 -> const std::vector<std::shared_ptr<ast::Ast>>& { return v.lookup(*n); })
        .def("lookup",
             [](AstLookupVisitor& v, std::shared_ptr<ast::Ast> n, ast::AstNodeType t)
                 -> const std::vector<std::shared_ptr<ast::Ast>>& { return v.lookup(*n, t); })
        .def("lookup",
             [](AstLookupVisitor& v,
                std::shared_ptr<ast::Ast> n,
                const std::vector<ast::AstNodeType>& types)
                 -> const std::vector<std::shared_ptr<ast::Ast>>& { return v.lookup(*n, types); });

    nb::class_<ConstantFolderVisitor, AstVisitor> constant_folder_visitor(
        m_visitor, "ConstantFolderVisitor", docstring::constant_folder_visitor_class);
    constant_folder_visitor.def(nb::init<>())
        .def("visit_program", &ConstantFolderVisitor::visit_program);

    nb::class_<InlineVisitor, AstVisitor> inline_visitor(m_visitor,
                                                         "InlineVisitor",
                                                         docstring::inline_visitor_class);
    inline_visitor.def(nb::init<>()).def("visit_program", &InlineVisitor::visit_program);

    nb::class_<KineticBlockVisitor, AstVisitor> kinetic_block_visitor(
        m_visitor, "KineticBlockVisitor", docstring::kinetic_block_visitor_class);
    kinetic_block_visitor.def(nb::init<>())
        .def("visit_program", &KineticBlockVisitor::visit_program);

    nb::class_<LocalVarRenameVisitor, AstVisitor> local_var_rename_visitor(
        m_visitor, "LocalVarRenameVisitor", docstring::local_var_rename_visitor_class);
    local_var_rename_visitor.def(nb::init<>())
        .def("visit_program", &LocalVarRenameVisitor::visit_program);

    nb::class_<MatexpVisitor, AstVisitor> matexp_visitor(m_visitor,
                                                         "MatexpVisitor",
                                                         docstring::matexp_visitor_class);
    matexp_visitor.def(nb::init<>()).def("visit_program", &MatexpVisitor::visit_program);
}

#pragma clang diagnostic pop
