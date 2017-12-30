from printer import *
from utils import *


class AbstractVisitorPrinter(DeclarationPrinter):
    """Prints abstract base class for all visitor implementations"""

    def headers(self):
        pass

    def class_comment(self):
        self.writer.write_line("/* Abstract base class for all visitor implementations */")

    def public_declaration(self):
        self.writer.write_line("public:", post_gutter=1)

        for node in self.nodes:
            line = "virtual void visit_" + to_snake_case(node.class_name) + "(" + node.class_name + "* node) = 0;"
            self.writer.write_line(line)

        self.writer.decrease_gutter()


class AstVisitorDeclarationPrinter(DeclarationPrinter):
    """Prints base visitor class declaration"""

    def headers(self):
        line = '#include "ast/ast.hpp"'
        self.writer.write_line(line)
        line = '#include "visitors/visitor.hpp"'
        self.writer.write_line(line)
        line = "using namespace ast;"
        self.writer.write_line(line, newline=2)

    def class_comment(self):
        self.writer.write_line("/* Basic visitor implementation */")

    def class_name_declaration(self):
        self.writer.write_line("class " + self.classname + " : public Visitor {")

    def public_declaration(self):
        self.writer.write_line("public:", post_gutter=1)

        for node in self.nodes:
            line = "virtual void visit_" + to_snake_case(node.class_name) + "(" + node.class_name + "* node) override;"
            self.writer.write_line(line)

        self.writer.decrease_gutter()


class AstVisitorDefinitionPrinter(DefinitionPrinter):
    """Prints base visitor class method definitions"""

    def headers(self):
        self.writer.write_line('#include "visitors/ast_visitor.hpp"', newline=2)

    def definitions(self):
        for node in self.nodes:
            line = "void " + self.classname + "::visit_" + to_snake_case(node.class_name) + "(" + node.class_name + "* node) {"
            self.writer.write_line(line, post_gutter=1)
            self.writer.write_line("node->visit_children(this);", post_gutter=-1)
            self.writer.write_line("}", newline=2)


class JSONVisitorDeclarationPrinter(DeclarationPrinter):
    """Prints visitor class declaration for printing AST in JSON format"""

    def headers(self):
        line = '#include "visitors/ast_visitor.hpp"'
        self.writer.write_line(line)
        line = '#include "printer/json_printer.hpp"'
        self.writer.write_line(line, newline=2)

    def class_comment(self):
        self.writer.write_line("/* Concrete visitor for printing AST in JSON format */")

    def class_name_declaration(self):
        self.writer.write_line("class " + self.classname + " : public AstVisitor {")

    def private_declaration(self):
        self.writer.write_line("private:", post_gutter=1)
        line = "std::unique_ptr<JSONPrinter> printer;"
        self.writer.write_line(line, newline=2, post_gutter=-1)

    def public_declaration(self):
        self.writer.write_line("public:", post_gutter=1)

        line = self.classname + "() : printer(new JSONPrinter())  {} "
        self.writer.write_line(line)

        line = self.classname + "(std::string filename) : printer(new JSONPrinter(filename)) {}"
        self.writer.write_line(line)

        line = self.classname + "(std::stringstream &ss) : printer(new JSONPrinter(ss)) {}"
        self.writer.write_line(line, newline=2)

        line = "void flush() { printer->flush(); }"
        self.writer.write_line(line)
        line = "void compact_json(bool flag) { printer->compact_json(flag); } "
        self.writer.write_line(line, newline=2)

        for node in self.nodes:
            line = "void visit_" + to_snake_case(node.class_name) + "(" + node.class_name + "* node) override;"
            self.writer.write_line(line)

        self.writer.decrease_gutter()


class JSONVisitorDefinitionPrinter(DefinitionPrinter):
    """Prints visitor class definition for printing AST in JSON format"""

    def headers(self):
        line = '#include "visitors/json_visitor.hpp"'
        self.writer.write_line(line, newline=2)

    def definitions(self):
        for node in self.nodes:
            line = "void " + self.classname + "::visit_" + to_snake_case(node.class_name) + "(" + node.class_name + "* node) {"
            self.writer.write_line(line, post_gutter=1)

            if node.has_children():
                self.writer.write_line("printer->push_block(node->get_type_name());")
                self.writer.write_line("node->visit_children(this);")

                if node.is_data_type_node():
                    if node.class_name == "Integer":
                        self.writer.write_line("if(!node->macroname) {", post_gutter=1)
                        self.writer.write_line("std::stringstream ss;")
                        self.writer.write_line("ss << node->eval();")
                        self.writer.write_line("printer->add_node(ss.str());", post_gutter=-1)
                        self.writer.write_line("}")
                    else:
                        self.writer.write_line("std::stringstream ss;")
                        self.writer.write_line("ss << node->eval();")
                        self.writer.write_line("printer->add_node(ss.str());")

                self.writer.write_line("printer->pop_block();")

                if node.class_name == "Program":
                    self.writer.write_line("flush();")
            else:
                self.writer.write_line("(void)node;")
                self.writer.write_line("printer->add_node(\"" + node.class_name + "\");")

            self.writer.write_line("}", pre_gutter=-1, newline=2)

        self.writer.decrease_gutter()


class SymtabVisitorDeclarationPrinter(DeclarationPrinter):
    """Prints visitor class declaration for printing Symbol table in JSON format"""

    def headers(self):
        line = '#include "visitors/json_visitor.hpp"'
        self.writer.write_line(line)
        line = '#include "visitors/ast_visitor.hpp"'
        self.writer.write_line(line)
        line = '#include "symtab/symbol_table.hpp"'
        self.writer.write_line(line, newline=2)

    def class_comment(self):
        self.writer.write_line("/* Concrete visitor for constructing symbol table from AST */")

    def class_name_declaration(self):

        self.writer.write_line("using namespace symtab;")
        self.writer.write_line("class " + self.classname + " : public AstVisitor {")

    def private_declaration(self):
        self.writer.write_line("private:", post_gutter=1)
        self.writer.write_line("ModelSymbolTable* modsymtab;", newline=2, post_gutter=-1)
        self.writer.write_line("std::unique_ptr<JSONPrinter> printer;")

    def public_declaration(self):
        self.writer.write_line("public:", post_gutter=1)

        line = self.classname + "(ModelSymbolTable* symtab) : modsymtab(symtab), printer(new JSONPrinter()) {} "
        self.writer.write_line(line)

        line = self.classname + "( ModelSymbolTable* symtab, std::stringstream &ss) : modsymtab(symtab), printer(new JSONPrinter(ss)) {}"
        self.writer.write_line(line)

        line = self.classname + "( ModelSymbolTable* symtab, std::string filename) : modsymtab(symtab), printer(new JSONPrinter(filename)) {}"
        self.writer.write_line(line, newline=2)

        # helper function for creating symbol for variables
        self.writer.write_line("template<typename T>")
        self.writer.write_line("void setup_symbol(T* node, SymbolInfo property, int order = 0);", newline=2)

        # helper function for creating symbol table for blocks
        # without name (e.g. parameter, unit, breakpoint)
        self.writer.write_line("template<typename T>")
        line = "void setup_symbol_table(T *node, std::string name, bool is_global);"
        self.writer.write_line(line)

        # helper function for creating symbol table for blocks
        # with name (e.g. procedure, function, derivative)
        self.writer.write_line("template<typename T>")
        line = "void setup_symbol_table(T *node, std::string name, SymbolInfo property, bool is_global);"
        self.writer.write_line(line, newline=2)

        # we have to override visitor methods for the nodes
        # which goes into symbol table
        for node in self.nodes:
            if node.is_symtab_method_required():
                line = "void visit_" + to_snake_case(node.class_name) + "(" + node.class_name + "* node) override;"
                self.writer.write_line(line)

        self.writer.decrease_gutter()

    def post_declaration(self):
        # helper function definitions
        self.writer.write_line()
        self.writer.write_line('#include "visitors/symtab_visitor_helper.hpp"')


class SymtabVisitorDefinitionPrinter(DefinitionPrinter):
    """Prints visitor class definition for printing Symbol table in JSON format"""

    def headers(self):
        line = '#include "symtab/symbol_table.hpp"'
        self.writer.write_line(line)
        line = '#include "visitors/symtab_visitor.hpp"'
        self.writer.write_line(line, newline=2)

    def definitions(self):
        for node in self.nodes:
            if node.is_symtab_method_required():

                line = "void " + self.classname + "::visit_" + to_snake_case(node.class_name) + "(" + node.class_name + "* node) {"
                self.writer.write_line(line, post_gutter=1)

                type_name = to_snake_case(node.class_name)
                property_name = "symtab::details::NmodlInfo::" + type_name

                if node.is_symbol_var_node() or node.is_prime_node():
                    is_prime = ", node->get_order()" if node.is_prime_node() else "";
                    self.writer.write_line("setup_symbol(node, " + property_name + is_prime + ");")

                else:
                    """ setupBlock has node*, properties, global_block"""
                    if node.is_symbol_block_node():
                        fun_call = "setup_symbol_table(node, node->get_name(), " + property_name + ", false);"

                    elif node.is_program_node() or node.is_global_block_node():
                        fun_call = "setup_symbol_table(node, node->get_type_name(), true);"

                    else:
                        """this is for nodes which has parent class as Block node"""
                        fun_call = "setup_symbol_table(node, node->get_type_name(), false);"

                    self.writer.write_line(fun_call)
                self.writer.write_line("}", pre_gutter=-1, newline=2)