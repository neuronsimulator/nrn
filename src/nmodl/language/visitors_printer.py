from printer import *


class AbstractVisitorPrinter(DeclarationPrinter):
    """Prints abstract base class for all visitor implementations"""

    def headers(self):
        pass

    def class_comment(self):
        self.writer.write_line("/* Abstract base class for all visitor implementations */")

    def public_declaration(self):
        self.writer.write_line("public:", post_gutter=1)

        for node in self.nodes:
            line = "virtual void visit" + node.class_name + "(" + node.class_name + "* node) = 0;"
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
            line = "virtual void visit" + node.class_name + "(" + node.class_name + "* node) override;"
            self.writer.write_line(line)

        self.writer.decrease_gutter()


class AstVisitorDefinitionPrinter(DefinitionPrinter):
    """Prints base visitor class method definitions"""

    def headers(self):
        self.writer.write_line('#include "visitors/ast_visitor.hpp"', newline=2)

    def definitions(self):
        for node in self.nodes:
            line = "void " + self.classname + "::visit" + node.class_name + "(" + node.class_name + "* node) {"
            self.writer.write_line(line, post_gutter=1)
            self.writer.write_line("node->visitChildren(this);", post_gutter=-1)
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
            line = "void visit" + node.class_name + "(" + node.class_name + "* node) override;"
            self.writer.write_line(line)

        self.writer.decrease_gutter()


class JSONVisitorDefinitionPrinter(DefinitionPrinter):
    """Prints visitor class definition for printing AST in JSON format"""

    def headers(self):
        line = '#include "visitors/json_visitor.hpp"'
        self.writer.write_line(line, newline=2)

    def definitions(self):
        for node in self.nodes:
            line = "void " + self.classname + "::visit" + node.class_name + "(" + node.class_name + "* node) {"
            self.writer.write_line(line, post_gutter=1)

            if node.has_children():
                self.writer.write_line("printer->pushBlock(node->getTypeName());")
                self.writer.write_line("node->visitChildren(this);")

                if node.is_data_type_node():
                    if node.class_name == "Integer":
                        self.writer.write_line("if(!node->macroname) {", post_gutter=1)
                        self.writer.write_line("std::stringstream ss;")
                        self.writer.write_line("ss << node->eval();")
                        self.writer.write_line("printer->addNode(ss.str());", post_gutter=-1)
                        self.writer.write_line("}")
                    else:
                        self.writer.write_line("std::stringstream ss;")
                        self.writer.write_line("ss << node->eval();")
                        self.writer.write_line("printer->addNode(ss.str());")

                self.writer.write_line("printer->popBlock();")

                if node.class_name == "Program":
                    self.writer.write_line("flush();")
            else:
                self.writer.write_line("printer->addNode(\"" + node.class_name + "\");")

            self.writer.write_line("}", pre_gutter=-1, newline=2)

        self.writer.decrease_gutter()