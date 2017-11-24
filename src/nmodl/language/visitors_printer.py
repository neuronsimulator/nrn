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
        self.writer.write_line('#include "visitors/astvisitor.hpp"', newline=2)

    def definitions(self):
        for node in self.nodes:
            line = "void " + self.classname + "::visit" + node.class_name + "(" + node.class_name + "* node) {"
            self.writer.write_line(line, post_gutter=1)
            self.writer.write_line("node->visitChildren(this);", post_gutter=-1)
            self.writer.write_line("}", newline=2)