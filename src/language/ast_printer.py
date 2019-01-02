from printer import *
from utils import *

class AstForwardDeclarationPrinter(DeclarationPrinter):
    """Prints all AST nodes forward declarations"""

    def headers(self):
        self.write_line("#include <memory>")
        self.write_line("#include <string>")
        self.write_line("#include <vector>", newline=2)

    def class_name_declaration(self):
        self.write_line("namespace ast {", newline=2, post_gutter=1)
        self.write_line("/* forward declarations of AST classes */")
        for node in sorted(self.nodes):
            self.write_line("class {};".format(node.class_name))

        self.write_line()
        self.write_line("/* Type for every ast node */")
        self.write_line("enum class AstNodeType {", post_gutter=1)

        for node in sorted(self.nodes):
            self.write_line("{},".format(to_snake_case(node.class_name).upper()))

        self.write_line("};", pre_gutter=-1, newline=2)

        self.write_line("/* std::vector for convenience */")
        for node in sorted(self.nodes):
            typename = "std::vector<std::shared_ptr<{}>>".format(node.class_name)
            self.write_line("using {}Vector = {};".format(node.class_name, typename))

    def declaration_end(self):
        self.write_line(pre_gutter=-1)

    def private_declaration(self):
        pass

    def public_declaration(self):
        pass

    def post_declaration(self):
        self.write_line("} // namespace ast", pre_gutter=-1)


class AstDeclarationPrinter(DeclarationPrinter):
    """Prints all AST nodes class declarations"""

    def headers(self):
        self.write_line("#include <iostream>")
        self.write_line("#include <memory>")
        self.write_line("#include <string>")
        self.write_line("#include <vector>", newline=2)

        self.write_line('#include "ast/ast_decl.hpp"')
        self.write_line('#include "ast/ast_common.hpp"')
        self.write_line('#include "lexer/modtoken.hpp"')
        self.write_line('#include "utils/common_utils.hpp"')
        self.write_line('#include "visitors/visitor.hpp"', newline=2)

    def ast_classes_declaration(self):

        # TODO: for demo, removing error messages in get_token method.
        # need to look in detail whether we should abort in this case
        # when ModToken is NULL. This was introduced when added SymtabVisitor
        # pass to get Token information.

        # todo : need a way to setup location
        # self.write_line("void setLocation(nmodl::location loc) {}")

        self.write_line("/* Define all AST nodes */", newline=2)

        for node in self.nodes:

            self.write_line("class {} : public {} {{".format(node.class_name, node.base_class), post_gutter=1)

            private_members = node.private_members()
            public_members = node.public_members()

            if private_members:
                self.write_line("private:", post_gutter=1)
                for member in private_members:
                    self.write_line("{} {};".format(member[0], member[1]))
                self.write_line(post_gutter=-1)

            self.write_line("public:", post_gutter=1)
            for member in public_members:
                self.write_line("{} {};".format(member[0], member[1]))

            if node.children:
                members = [ child.get_typename() + " " + child.varname for child in node.children]
                self.write_line()
                self.write_line("{}({});".format(node.class_name, (", ".join(members))))
                self.write_line("{0}(const {0}& obj);".format(node.class_name))

            # program node holds statements and blocks and we instantiate it in driver
            # also other nodes which we use as value type, parsor needs to return them
            # as a value. And instantiation of those causes error if default constructor not there.
            if node.is_program_node() or node.is_ptr_excluded_node():
                self.write_line( node.class_name + "() = default;", newline=2)

            # depending on if node is abstract or not, we have to
            # add virtual or override keyword to methods
            virtual, override = ("virtual ", "") if node.is_abstract else ("", " override")

            # Todo : We need virtual destructor otherwise there will be memory leaks.
            #        But we need define which are virtual base classes that needs virtual function.
            self.write_line("virtual ~{}() {{}}".format(node.class_name))
            self.write_line()

            for child in node.children:
                class_name = child.class_name
                varname = child.varname
                if child.add_method:
                    self.write_line("void add{0}({0} *s) {{".format(class_name))
                    self.write_line("    {}.emplace_back(s);".format(varname))
                    self.write_line("}")

                if child.get_node_name:
                    # string node should be evaluated and hence eval() method
                    method = "eval" if child.is_string_node() else "get_node_name"
                    self.write_line("virtual std::string get_node_name() override {")
                    self.write_line("    return {}->{}();".format(varname, method))
                    self.write_line("}")

                getter_method = child.getter_method if child.getter_method else "get_" + to_snake_case(varname)
                getter_override = " override" if child.getter_override else ""
                return_type = child.member_typename
                self.write_line("{} {}(){}{{ return {}; }}".format(return_type, getter_method, getter_override, varname))

                setter_method = "set_" + to_snake_case(varname)
                setter_type = child.member_typename
                reference = "" if child.is_base_type_node() else "&&"
                self.write_line("void {0}({1}{2} {3}) {{ this->{3} = {3}; }}".format(setter_method, setter_type, reference, varname))

            self.write_line()

            # all member functions
            self.write_line("{}void visit_children (Visitor* v) override;".format(virtual))
            self.write_line("{}void accept(Visitor* v) override {{ v->visit_{}(this); }}".format(virtual, to_snake_case(node.class_name)))
            self.write_line("{0}{1}* clone() override {{ return new {1}(*this); }}".format(virtual, node.class_name))

            self.write_line()

            # TODO: type should declared as enum class
            typename = to_snake_case(node.class_name).upper()
            self.write_line("{}AstNodeType get_node_type() override {{ return AstNodeType::{}; }}".format(virtual, typename))
            self.write_line('{}std::string get_node_type_name() override {{ return "{}"; }}'.format(virtual, node.class_name))
            self.write_line("bool is_{} () override {{ return true; }}".format(to_snake_case(node.class_name)))

            if node.has_token:
                self.write_line("{}ModToken* get_token(){} {{ return token.get(); }}".format(virtual, override))
                self.write_line("void set_token(ModToken& tok) { token = std::shared_ptr<ModToken>(new ModToken(tok)); }")

            if node.is_symtab_needed():
                self.write_line("void set_symbol_table(symtab::SymbolTable* newsymtab) override { symtab = newsymtab; }")
                self.write_line("symtab::SymbolTable* get_symbol_table() override { return symtab; }")

            if node.is_program_node():
                self.write_line("symtab::ModelSymbolTable* get_model_symbol_table() { return &model_symtab; }")

            if node.is_base_class_number_node():
                negation = "!" if node.is_boolean_node() else "-";
                self.write_line("void negate() override {{ value = {}value; }}".format(negation))
                self.write_line("double to_double() override { return value; }")

            if node.is_name_node():
                self.write_line("{}void set_name(std::string name){} {{ value->set(name); }}".format(virtual, override))

            if node.is_number_node():
                self.write_line('{}double to_double() {{ throw std::runtime_error("to_double not implemented"); }}'.format(virtual))

            if node.is_base_block_node():
                self.write_line('virtual ArgumentVector get_parameters() { throw std::runtime_error("get_parameters not implemented"); }')

            # if node is of enum type then return enum value
            if node.is_data_type_node():
                data_type = node.get_data_type_name()
                if node.is_enum_node():
                    self.write_line("std::string eval() {{ return {}Names[value]; }}".format(data_type))
                # But if basic data type then eval return their value
                else:
                    self.write_line("{} eval() {{ return value; }}".format(data_type))
                    self.write_line("void set({} _value) {{ value = _value; }}".format(data_type))

            self.write_line("};", pre_gutter=-2, newline=2)

    def class_name_declaration(self):
        self.write_line("namespace ast {", newline=2, post_gutter=1)
        self.ast_classes_declaration()

    def declaration_end(self):
        self.write_line(pre_gutter=-1)

    def private_declaration(self):
        pass

    def public_declaration(self):
        pass

    def post_declaration(self):
        self.write_line("} // namespace ast", pre_gutter=-1)


class AstDefinitionPrinter(DefinitionPrinter):
    """Prints AST class definitions"""

    def headers(self):
        self.write_line('#include "ast/ast.hpp"')
        self.write_line('#include "symtab/symbol_table.hpp"', newline=2)

    def definitions(self):
        self.write_line("namespace ast {", post_gutter=1)

        for node in self.nodes:
            # first get types of all childrens into members vector
            members = [(child.get_typename(), child.varname) for child in node.children]
            non_base_members = [child for child in node.children if not child.is_base_type_node()]

            self.write_line("/* visit method for {} ast node */".format(node.class_name))
            self.write_line("void {}::visit_children(Visitor* v) {{".format(node.class_name), post_gutter=1)

            for child in non_base_members:
                if child.is_vector:
                    self.write_line("for (auto& item : this->{}) {{".format(child.varname))
                    self.write_line("        item->accept(v);")
                    self.write_line("}")
                elif child.optional:
                    self.write_line("if (this->{}) {{".format(child.varname))
                    self.write_line("    this->{}->accept(v);".format(child.varname))
                    self.write_line("}")
                elif child.is_pointer_node():
                    self.write_line("{}->accept(v);".format(child.varname))
                else:
                    self.write_line("{}.accept(v);".format(child.varname))

            if not non_base_members:
                self.write_line("(void)v;")

            self.write_line("}", pre_gutter=-1, newline=2)

            if members:
                arguments = (", ".join(map(lambda x: x[0] + " " + x[1], members)))
                initializer = ", ".join(map(lambda x: x[1] + "(" + x[1] + ")", members))

                self.write_line("/* constructor for {} ast node */".format(node.class_name))
                self.write_line("{0}::{0}({1})".format(node.class_name, arguments))
                self.write_line("    :  {}".format(initializer))
                self.write_line("{}", newline=2)

                self.write_line("/* copy constructor for {} ast node */".format(node.class_name))
                self.write_line("{0}::{0}(const {0}& obj) {{".format(node.class_name), post_gutter=1)

                for child in node.children:
                    if child.is_vector:
                        self.write_line("for (auto& item : obj.{}) {{".format(child.varname))
                        self.write_line("    this->{}.emplace_back(item->clone());".format(child.varname))
                        self.write_line("}")
                    elif child.is_pointer_node() or child.optional:
                        self.write_line("if (obj.{}) {{".format(child.varname))
                        self.write_line("    this->{0}.reset(obj.{0}->clone());".format(child.varname))
                        self.write_line("}")
                    else:
                        self.write_line("this->{0} = obj.{0};".format(child.varname))

                if node.has_token:
                    self.write_line("if (obj.token) {")
                    self.write_line("    this->token = std::shared_ptr<ModToken>(obj.token->clone());")
                    self.write_line("}")

                self.write_line("}", pre_gutter=-1, newline=2)

        self.write_line("} // namespace ast", pre_gutter=-1)
