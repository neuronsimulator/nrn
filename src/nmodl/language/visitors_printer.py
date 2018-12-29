from printer import *
from utils import *


class AbstractVisitorPrinter(DeclarationPrinter):
    """Prints abstract base class for all visitor implementations"""

    def headers(self):
        pass

    def class_comment(self):
        self.write_line("/* Abstract base class for all visitor implementations */")

    def public_declaration(self):
        self.write_line("public:", post_gutter=1)

        for node in self.nodes:
            line = "virtual void visit_" + to_snake_case(node.class_name) + "(" + node.class_name + "* node) = 0;"
            self.write_line(line)

        self.writer.decrease_gutter()


class AstVisitorDeclarationPrinter(DeclarationPrinter):
    """Prints base visitor class declaration"""

    def headers(self):
        line = '#include "ast/ast.hpp"'
        self.write_line(line)
        line = '#include "visitors/visitor.hpp"'
        self.write_line(line)
        line = "using namespace ast;"
        self.write_line(line, newline=2)

    def class_comment(self):
        self.write_line("/* Basic visitor implementation */")

    def class_name_declaration(self):
        self.write_line("class " + self.classname + " : public Visitor {")

    def public_declaration(self):
        self.write_line("public:", post_gutter=1)

        for node in self.nodes:
            line = "virtual void visit_" + to_snake_case(node.class_name) + "(" + node.class_name + "* node) override;"
            self.write_line(line)

        self.writer.decrease_gutter()


class AstVisitorDefinitionPrinter(DefinitionPrinter):
    """Prints base visitor class method definitions"""

    def headers(self):
        self.write_line('#include "visitors/ast_visitor.hpp"', newline=2)

    def definitions(self):
        for node in self.nodes:
            line = "void " + self.classname + "::visit_" + to_snake_case(node.class_name) + "(" + node.class_name + "* node) {"
            self.write_line(line, post_gutter=1)
            self.write_line("node->visit_children(this);", post_gutter=-1)
            self.write_line("}", newline=2)


class JSONVisitorDeclarationPrinter(DeclarationPrinter):
    """Prints visitor class declaration for printing AST in JSON format"""

    def headers(self):
        line = '#include "visitors/ast_visitor.hpp"'
        self.write_line(line)
        line = '#include "printer/json_printer.hpp"'
        self.write_line(line, newline=2)

    def class_comment(self):
        self.write_line("/* Concrete visitor for printing AST in JSON format */")

    def class_name_declaration(self):
        self.write_line("class " + self.classname + " : public AstVisitor {")

    def private_declaration(self):
        self.write_line("private:", post_gutter=1)
        line = "std::unique_ptr<JSONPrinter> printer;"
        self.write_line(line, newline=2, post_gutter=-1)

    def public_declaration(self):
        self.write_line("public:", post_gutter=1)

        line = self.classname + "() : printer(new JSONPrinter())  {} "
        self.write_line(line)

        line = self.classname + "(std::string filename) : printer(new JSONPrinter(filename)) {}"
        self.write_line(line)

        line = self.classname + "(std::stringstream &ss) : printer(new JSONPrinter(ss)) {}"
        self.write_line(line, newline=2)

        line = "void flush() { printer->flush(); }"
        self.write_line(line)
        line = "void compact_json(bool flag) { printer->compact_json(flag); } "
        self.write_line(line, newline=2)

        for node in self.nodes:
            line = "void visit_" + to_snake_case(node.class_name) + "(" + node.class_name + "* node) override;"
            self.write_line(line)

        self.writer.decrease_gutter()


class JSONVisitorDefinitionPrinter(DefinitionPrinter):
    """Prints visitor class definition for printing AST in JSON format"""

    def headers(self):
        line = '#include "visitors/json_visitor.hpp"'
        self.write_line(line, newline=2)

    def definitions(self):
        for node in self.nodes:
            line = "void " + self.classname + "::visit_" + to_snake_case(node.class_name) + "(" + node.class_name + "* node) {"
            self.write_line(line, post_gutter=1)

            if node.has_children():
                self.write_line("printer->push_block(node->get_type_name());")
                self.write_line("node->visit_children(this);")

                if node.is_data_type_node():
                    if node.class_name == "Integer":
                        self.write_line("if(!node->macroname) {", post_gutter=1)
                        self.write_line("std::stringstream ss;")
                        self.write_line("ss << node->eval();")
                        self.write_line("printer->add_node(ss.str());", post_gutter=-1)
                        self.write_line("}")
                    else:
                        self.write_line("std::stringstream ss;")
                        self.write_line("ss << node->eval();")
                        self.write_line("printer->add_node(ss.str());")

                self.write_line("printer->pop_block();")

                if node.class_name == "Program":
                    self.write_line("flush();")
            else:
                self.write_line("(void)node;")
                self.write_line("printer->add_node(\"" + node.class_name + "\");")

            self.write_line("}", pre_gutter=-1, newline=2)

        self.writer.decrease_gutter()


class SymtabVisitorDeclarationPrinter(DeclarationPrinter):
    """Prints visitor class declaration for printing Symbol table in JSON format"""

    def headers(self):
        line = '#include "visitors/json_visitor.hpp"'
        self.write_line(line)
        line = '#include "visitors/ast_visitor.hpp"'
        self.write_line(line)
        line = '#include "symtab/symbol_table.hpp"'
        self.write_line(line, newline=2)

    def class_comment(self):
        self.write_line("/* Concrete visitor for constructing symbol table from AST */")

    def class_name_declaration(self):

        self.write_line("using namespace symtab;")
        self.write_line("class " + self.classname + " : public AstVisitor {")

    def private_declaration(self):
        self.write_line("private:", post_gutter=1)
        self.write_line("ModelSymbolTable* modsymtab;", newline=2, post_gutter=-1)
        self.write_line("std::unique_ptr<JSONPrinter> printer;")
        self.write_line("std::string block_to_solve;")
        self.write_line("bool update = false;")
        self.write_line("bool under_state_block = false;", newline=2)

    def public_declaration(self):
        self.write_line("public:", post_gutter=1)

        line = "explicit " + self.classname + "(bool update = false) : printer(new JSONPrinter()), update(update) {} "
        self.write_line(line)

        line = self.classname + "(std::stringstream &ss, bool update = false) : printer(new JSONPrinter(ss)), update(update) {}"
        self.write_line(line)

        line = self.classname + "(std::string filename, bool update = false) : printer(new JSONPrinter(filename)), update(update) {}"
        self.write_line(line, newline=2)

        # helper function for setting up symbol for variable
        self.write_line("void setup_symbol(ast::Node* node, SymbolInfo property);", newline=2)

        # add symbol with given property to model symbol table
        line = "void add_model_symbol_with_property(ast::Node* node, SymbolInfo property);"
        self.write_line(line, newline=2)

        # helper function for creating symbol table for blocks
        line = "void setup_symbol_table(ast::AST *node, const std::string& name, bool is_global);"
        self.write_line(line, newline=2)

        # helper function for creating symbol table for global blocks of mod file
        line = "void setup_symbol_table_for_global_block(ast::Node *node);"
        self.write_line(line, newline=2)

        # helper function for creating symbol table for non-global blocks (e.g. function, procedures)
        line = "void setup_symbol_table_for_scoped_block(ast::Node *node, const std::string& name);"
        self.write_line(line, newline=2)

        # helper function to setup program symbol table
        line = "void setup_symbol_table_for_program_block(ast::Program *node);"
        self.write_line(line, newline=2)

        # we have to override visitor methods for the nodes
        # which goes into symbol table
        for node in self.nodes:
            if node.is_symtab_method_required():
                line = "void visit_" + to_snake_case(node.class_name) + "(" + node.class_name + "* node) override;"
                self.write_line(line)

        self.writer.decrease_gutter()


class SymtabVisitorDefinitionPrinter(DefinitionPrinter):
    """Prints visitor class definition for printing Symbol table in JSON format"""

    def headers(self):
        line = '#include "symtab/symbol_table.hpp"'
        self.write_line(line)
        line = '#include "visitors/symtab_visitor.hpp"'
        self.write_line(line, newline=2)

    def definitions(self):
        for node in self.nodes:

            # for helper nodes definition needs to be diectly implemented in symtab_visitor_helper.cpp
            if node.is_symbol_helper_node():
                continue

            if node.is_symtab_method_required():

                line = "void " + self.classname + "::visit_" + to_snake_case(node.class_name) + "(" + node.class_name + "* node) {"
                self.write_line(line, post_gutter=1)

                type_name = to_snake_case(node.class_name)
                property_name = "NmodlInfo::" + type_name

                if node.is_symbol_var_node():
                    self.write_line("setup_symbol(node, " + property_name + ");")

                else:

                    if node.is_program_node():
                        self.write_line("setup_symbol_table_for_program_block(node);")

                    elif node.is_global_block_node():
                        self.write_line("setup_symbol_table_for_global_block(node);")

                    else:
                        """this is for nodes which has parent class as Block node"""
                        if node.is_symbol_block_node():
                            self.write_line("add_model_symbol_with_property(node, %s);" % property_name)
                            self.write_line("setup_symbol_table_for_scoped_block(node, node->get_name());")
                        else:
                            self.write_line("setup_symbol_table_for_scoped_block(node, node->get_type_name());")

                self.write_line("}", pre_gutter=-1, newline=2)