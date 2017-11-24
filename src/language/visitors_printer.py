from base_printer import *


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
