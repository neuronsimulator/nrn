from parser import LanguageParser
from ast_printer import *
from visitors_printer import *


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
