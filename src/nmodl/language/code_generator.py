from parser import LanguageParser
from ast_printer import *
from visitors_printer import *
from nmodl_printer import *


# parse nmodl definition file and get list of abstract nodes
nodes = LanguageParser("nmodl.yaml").parse_file()

# print various header and code files

AstDeclarationPrinter(
    "../ast/ast.hpp",
    "",
    nodes).write()

AstDefinitionPrinter(
    "../ast/ast.cpp",
    "",
    nodes).write()

AbstractVisitorPrinter(
    "../visitors/visitor.hpp",
    "Visitor",
    nodes).write()

AstVisitorDeclarationPrinter(
    "../visitors/ast_visitor.hpp",
    "AstVisitor",
    nodes).write()

AstVisitorDefinitionPrinter(
    "../visitors/ast_visitor.cpp",
    "AstVisitor",
    nodes).write()

JSONVisitorDeclarationPrinter(
    "../visitors/json_visitor.hpp",
    "JSONVisitor",
    nodes).write()

JSONVisitorDefinitionPrinter(
    "../visitors/json_visitor.cpp",
    "JSONVisitor",
    nodes).write()

SymtabVisitorDeclarationPrinter(
    "../visitors/symtab_visitor.hpp",
    "SymtabVisitor",
    nodes).write()

SymtabVisitorDefinitionPrinter(
    "../visitors/symtab_visitor.cpp",
    "SymtabVisitor",
    nodes).write()

NmodlVisitorDeclarationPrinter(
    "../visitors/nmodl_visitor.hpp",
    "NmodlPrintVisitor",
    nodes).write()

NmodlVisitorDefinitionPrinter(
    "../visitors/nmodl_visitor.cpp",
    "NmodlPrintVisitor",
    nodes).write()