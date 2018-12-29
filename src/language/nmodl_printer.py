from printer import *
from utils import *
from node_info import ORDER_VAR_NAME, BINARY_OPERATOR_NAME, BINARY_EXPRESSION_NODE

class NmodlVisitorDeclarationPrinter(DeclarationPrinter):
    """Visitor class declaration for printing AST back to NMODL"""

    def headers(self):
        self.write_line('#include "ast/ast.hpp"')
        self.write_line('#include "printer/nmodl_printer.hpp"', newline=2)

    def class_comment(self):
        self.write_line("/* Visitor for printing AST back to NMODL */")

    def class_name_declaration(self):
        self.write_line("class " + self.classname + " : public Visitor {")

    def private_declaration(self):
        self.write_line("private:")
        self.write_line("    std::unique_ptr<NMODLPrinter> printer;")

    def public_declaration(self):
        self.write_line("public:", post_gutter=1)
        line = self.classname + "() : printer(new NMODLPrinter()) {}"
        self.write_line(line)

        line = self.classname + "(std::string filename) : printer(new NMODLPrinter(filename)) {}"
        self.write_line(line)

        line = self.classname + "(std::stringstream& stream) : printer(new NMODLPrinter(stream)) {}"
        self.write_line(line, newline=2)

        for node in self.nodes:
            line = "virtual void visit_" + to_snake_case(node.class_name) + "(ast::" + node.class_name + "* node) override;"
            self.write_line(line)

        line = "template<typename T>"
        self.write_line(line)
        line = "void visit_element(const std::vector<T>& elements, std::string separator, bool program, bool statement);"
        self.write_line(line)


class NmodlVisitorDefinitionPrinter(DefinitionPrinter):
    """Print visitor class definitions for printing AST back to NMODL"""

    def headers(self):
        self.write_line('#include "visitors/nmodl_visitor.hpp"')
        self.write_line('#include "visitors/nmodl_visitor_helper.hpp"', newline=2)
        self.write_line("using namespace ast;", newline=2)

    def add_element(self, name):
        if name:
            name = '"' + name + '"'
            self.write_line("printer->add_element(" + name + ");")

    def definitions(self):
        for node in self.nodes:

            self.write_line("/* concrete visit function for " + node.class_name + " nodes */")
            self.write_line("void NmodlPrintVisitor::visit_" + to_snake_case(node.class_name) + "(" + node.class_name + "* node) {", post_gutter=1)

            if node.nmodl_name:
                name = '"' + node.nmodl_name + '"'
                self.write_line("printer->add_element(" + name + ");")

            self.add_element(node.prefix)

            if node.is_block_node():
                self.write_line("printer->push_level();")

            # for basic data types we just have to eval and they will return their value
            # but for integer node we have to check if it is represented as macro
            if node.is_data_type_node():
                if node.is_integer_node():
                    self.write_line("if(node->macroname == nullptr) {")
                    self.write_line("    printer->add_element(std::to_string(node->eval()));")
                    self.write_line("}")
                else:
                    self.write_line("std::stringstream ss;")
                    self.write_line("ss << node->eval();")
                    self.write_line("printer->add_element(ss.str());")

            for child in node.children:
                self.add_element(child.force_prefix)

                is_program = "false"
                is_statement = "false"

                # unit block has expressions as children and hence need to be considered as statements
                if child.is_statement_node() or node.is_unit_block():
                    is_statement = "true"

                if node.is_program_node():
                    is_program = "true"

                # In the current implementation all childrens are non-base-data-type nodes
                # i.e. int, double are wrapped into Integer, Double classes. Hence it is
                # not tested in non data types nodes. This restriction could be easily avoided
                # if required.
                if child.is_base_type_node():
                    if not node.is_data_type_node():
                        raise "Base data type members not in non data type nodes handled/tested!"
                else:
                    self.write_line("// Processing " + child.class_name + " " + child.varname)

                    # optional members or statement blocks are in brace (could be nullptr)
                    # start of a brace
                    if child.optional or child.is_statement_block_node():
                        line = 'if(node->' + child.varname + ') {'
                        self.write_line(line, post_gutter=1)

                    # todo : assugming member with nmodl of type Boolean. currently there
                    # are only two use cases: SWEEP and "->""
                    if child.nmodl_name:
                        self.write_line('if(' + 'node->' + child.varname + '->eval()) {')
                        name = '"' + child.nmodl_name + '"'
                        self.write_line("    printer->add_element(" + name + ");")
                        self.write_line('}')

                    # vector members could be empty and pre-defined method to iterate/visit
                    elif child.is_vector:
                        if child.prefix or child.suffix:
                            self.write_line("if (!node->" + child.varname + ".empty()) {", post_gutter=1)
                        self.add_element(child.prefix)
                        separator = '"' + child.separator + '"'
                        self.write_line("visit_element(node->" + child.varname + "," + separator + "," + is_program + "," + is_statement + ");");
                        self.add_element(child.suffix)
                        if child.prefix or child.suffix:
                            self.write_line("}", pre_gutter=-1)

                    # prime need to be printed with "'" as suffix
                    elif node.is_prime_node() and child.varname == ORDER_VAR_NAME:
                        self.write_line("auto order = node->" + child.varname + "->eval();")
                        self.write_line("auto symbol = std::string(order, '\\'');")
                        self.write_line("printer->add_element(symbol);");

                    # add space surrounding certain binary operators for readability
                    elif node.class_name == BINARY_EXPRESSION_NODE and child.varname == BINARY_OPERATOR_NAME:
                        self.write_line('std::string op = node->op.eval();')
                        self.write_line('if(op == "=" || op == "&&" || op == "||" || op == "==")')
                        self.write_line('    op = " " + op + " ";')
                        self.write_line('printer->add_element(op);')

                    else:
                        self.add_element(child.prefix)
                        if child.is_pointer_node():
                            self.write_line("node->" + child.varname + "->accept(this);")
                        else:
                            self.write_line("node->" + child.varname + ".accept(this);")
                        self.add_element(child.suffix)

                    # end of a brace
                    if child.optional or child.is_statement_block_node():
                        self.write_line("}", pre_gutter=-1)

                self.add_element(child.force_suffix)

            self.add_element(node.suffix)

            if node.is_block_node():
                self.write_line("printer->pop_level();")

            self.write_line("}", pre_gutter=-1, newline=2)
