from printer import *
from utils import *
from node_info import ORDER_VAR_NAME


class AstDeclarationPrinter(DeclarationPrinter):
    """Prints all AST nodes class declarations"""

    def ast_types(self):
        """ print ast type for every ast node """
        self.write_line("enum class Type {", post_gutter=1)
        for node in self.nodes[:-1]:
            name = to_snake_case(node.class_name).upper() + ","
            self.write_line(name)
        name = to_snake_case(self.nodes[-1].class_name).upper()
        self.write_line(name)
        self.write_line("};", pre_gutter=-1)

    def headers(self):
        self.write_line("#include <iostream>")
        self.write_line("#include <memory>")
        self.write_line("#include <string>")
        self.write_line("#include <vector>", newline=2)

        self.write_line('#include "ast/ast_utils.hpp"')
        self.write_line('#include "lexer/modtoken.hpp"')
        self.write_line('#include "utils/common_utils.hpp"', newline=2)

    def class_comment(self):
        self.write_line("/* all classes representing Abstract Syntax Tree (AST) nodes */")

    def forward_declarations(self):
        self.write_line("/* forward declarations of AST classes */")
        for node in self.nodes:
            self.write_line("class " + node.class_name + ";")

        self.write_line()
        self.write_line("/* Type for every ast node */")
        self.ast_types()

        self.write_line()
        self.write_line("/* std::vector for convenience */")

        for node in self.nodes:
            typename = "std::vector<std::shared_ptr<" + node.class_name + ">>"
            self.write_line("using " + node.class_name + "Vector = " + typename + ";")

        created_types = []

        # iterate over all node types
        for node in self.nodes:
            type = (node.get_typename(), node.class_name.lower() + "_ptr")
            if type not in created_types:
                created_types.append(type)

            for child in node.children:
                ctype = child.get_typename()
                cname = child.class_name.lower()

                # base types like int, string are not defined in YYSTYPE
                if child.is_base_type_node():
                    continue

                if child.is_vector:
                    cname = cname + "_list"
                else:
                    cname += "_ptr";

                type = (ctype, cname)

                if type not in created_types:
                    created_types.append(type)

        for tuple in created_types:
            self.write_line("using " + tuple[1] + " = " + tuple[0] + ";")


    def ast_classes_declaration(self):

        # TODO: for demo, removing error messages in get_token method.
        # need to look in detail whether we should abort in this case
        # when ModToken in NULL. This was introduced when added SymtabVisitor
        # pass to get Token information.

        # TODO: remove visitor related method definition from declaration
        # to avoid this include
        self.write_line()
        self.write_line("#include <visitors/visitor.hpp>", newline=2)

        self.write_line("/* Define all AST nodes */", newline=2)

        for node in self.nodes:
            class_decl = "class " + node.class_name + " : public "

            if node.base_class:
                class_decl += node.base_class
            else:
                class_decl += " AST"

            # depending on if node is abstract or not, we have to
            # add virtual or override keyword to methods
            if node.is_abstract:
                virtual = "virtual "
                override = ""
            else:
                virtual = ""
                override = " override"

            self.write_line(class_decl + " {", post_gutter=1)
            self.write_line("public:", post_gutter=1)

            members = []
            members_with_types = []

            for child in node.children:
                members.append(child.get_typename() + " " + child.varname)
                members_with_types.append(child.get_typename_as_member() + " " + child.varname)

            if members:
                self.write_line("/* member variables */")

            # todo : need a way to setup location
            # self.write_line("void setLocation(nmodl::location loc) {}")

            for member in members_with_types:
                self.write_line(member + ";")

            if node.has_token:
                self.write_line("std::shared_ptr<ModToken> token;")


            if node.is_symtab_needed():
                self.write_line("std::shared_ptr<symtab::SymbolTable> symtab;", newline=2)

            if members:
                self.write_line("/* constructors */")
                self.write_line(node.class_name + "(" + (", ".join(members)) + ");")
                self.write_line(node.class_name + "(const " + node.class_name + "& obj);")

            # program node holds statements and blocks and we instantiate it in driver
            # also other nodes which we use as value type, parsor needs to return them
            # as a value. And instantiation of those causes error if default constructor not there.
            if node.is_program_node() or node.is_ptr_excluded_node():
                self.write_line( node.class_name + "() {}", newline=2)

            # Todo : We need virtual destructor otherwise there will be memory leaks.
            #        But we need define which are virtual base classes that needs virtual function.
            self.write_line("virtual ~" + node.class_name + "() {}")
            self.write_line()

            get_method_added = False

            for child in node.children:
                class_name = child.class_name
                varname = child.varname
                if child.add_method:
                    self.write_line("void add" + class_name + "(" + class_name + " *s) {")
                    self.write_line("    " + varname + ".push_back(std::shared_ptr<" + class_name + ">(s));")
                    self.write_line("}")

                if child.getname_method:
                    # string node should be evaluated and hence eval() method
                    if child.is_string_node():
                        method = "eval"
                    else:
                        method = "get_name"

                    self.write_line("virtual std::string get_name() override {")
                    self.write_line("    return " + varname + "->" + method + "();")
                    self.write_line("}")

                    if not node.has_token:
                        self.write_line(virtual + "ModToken* get_token() override {")
                        self.write_line("    return " + varname + "->get_token();")
                        self.write_line("}")

                    get_method_added = True

                if node.is_prime_node() and child.varname == ORDER_VAR_NAME:
                    self.write_line("int get_order() " + " { return " + ORDER_VAR_NAME + "->eval(); }")

            # add method to return typename
            self.write_line("virtual std::string get_type_name() override { return \"" + node.class_name + "\"; }")

            # all member functions
            self.write_line(virtual + "void visit_children (Visitor* v) override;")
            self.write_line(virtual + "void accept(Visitor* v) override { v->visit_" + to_snake_case(node.class_name) + "(this); }")

            # TODO: type should declared as enum class
            typename = to_snake_case(node.class_name).upper()
            self.write_line(virtual + "Type get_type() override { return Type::" + typename + "; }")
            self.write_line("bool is_" + to_snake_case(node.class_name) + " () override { return true; }")
            self.write_line(virtual + node.class_name + "* clone() override { return new " + node.class_name + "(*this); }")

            if node.has_token:
                self.write_line(virtual + "ModToken* get_token() " + override + " { return token.get(); }")
                self.write_line("void set_token(ModToken& tok) " + " { token = std::shared_ptr<ModToken>(new ModToken(tok)); }")

            if node.is_symtab_needed():
                self.write_line("void set_symbol_table(std::shared_ptr<symtab::SymbolTable> newsymtab) " + " { symtab = newsymtab; }")
                self.write_line("std::shared_ptr<symtab::SymbolTable> get_symbol_table() override " + " { return symtab; }")

            if node.is_number_node():
                self.write_line(virtual + "void negate()" + override + " { std::cout << \"ERROR : negate() not implemented! \"; abort(); } ")

            if node.is_base_class_number_node():
                if node.is_boolean_node():
                    self.write_line("void negate() override { value = !value; }")
                else:
                    self.write_line("void negate() override { value = -value; }")

            if node.is_identifier_node():
                self.write_line(virtual + "void set_name(std::string /*name*/)" + override + " { std::cout << \"ERROR : set_name() not implemented! \"; abort(); }")

            if node.is_name_node():
                self.write_line(virtual + "void set_name(std::string name)" + override + " { value->set(name); }")

            # if node is of enum type then return enum value
            # TODO: hardcoded Names
            if node.is_data_type_node():
                data_type = node.get_data_type_name()
                if node.is_enum_node():
                    self.write_line("std::string " + " eval() { return " + data_type + "Names[value]; }")
                # But if basic data type then eval return their value
                # TODO: value member is hardcoded
                else:
                    self.write_line(data_type + " eval() { return value; }")
                    self.write_line("void set(" + data_type + " _value) " + " { value = _value; }")

            self.write_line("};", pre_gutter=-2, newline=2)

    def class_name_declaration(self):
        self.write_line("namespace ast {", newline=2, post_gutter=1)
        self.forward_declarations()
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
        self.write_line('#include "ast/ast.hpp"', newline=2)

    def definitions(self):
        self.write_line("namespace ast {", post_gutter=1)

        for node in self.nodes:
            # first get types of all childrens into members vector
            members = []

            # in order to print initializer list, we need to know which
            # members are no pointer types
            ptr_members = []
            non_ptr_members = []

            for child in node.children:
                members.append((child.get_typename(), child.varname))

                if child.is_pointer_node() and not child.is_vector:
                    ptr_members.append((child.get_typename(), child.varname))
                else:
                    non_ptr_members.append((child.get_typename(), child.varname))

                if child.is_base_type_node():
                    continue

                self.writer.increase_gutter()
                if child.is_vector:
                    # TODO : remove this with C++11 style
                    self.add_line("for (auto& item : this->" + child.varname + ") {")
                    self.add_line("        item->accept(v);")
                    self.add_line("}")
                elif child.optional or child.is_statement_block_node():
                    self.add_line("if (this->" + child.varname + ") {")
                    self.add_line("    this->" + child.varname + "->accept(v);")
                    self.add_line("}")
                elif not child.is_pointer_node():
                    self.add_line(child.varname + ".accept(v);")
                else:
                    self.add_line(child.varname + "->accept(v);")
                self.writer.decrease_gutter()

            if self.writer.num_buffered_lines():
                self.write_line("/* visit method for " + node.class_name + " ast node */")
                self.write_line("void " + node.class_name + "::visit_children(Visitor* v) {", post_gutter=1)
                self.writer.flush_buffered_lines()
                self.write_line("}", pre_gutter=-1, newline=2)
            else:
                self.write_line("void " + node.class_name + "::visit_children(Visitor* /*v*/) {}")

            if members:
                # TODO : constructor definition : remove this with C++11 style
                self.write_line("/* constructor for " + node.class_name + " ast node */")
                arguments = (", ".join(map(lambda x: x[0] + " " + x[1], members)))
                self.write_line(node.class_name + "::" + node.class_name + "(" + arguments + ")")

                if non_ptr_members:
                    self.write_line(":", newline=0)
                    self.write_line(",\n".join(map(lambda x: x[1] + "(" + x[1] + ")", non_ptr_members)))

                self.write_line("{")

                for member in ptr_members:
                    # todo : bit hack here, need to remove pointer because we are creating smart pointer
                    typename = member[0].replace("*", "")
                    self.write_line("    this->" + member[1] + " = std::shared_ptr<" + typename + ">(" + member[1] + ");")

                self.write_line("}", newline=2)

                # copy construcotr definition : remove this with C++11 style
                self.write_line("/* copy constructor for " + node.class_name + " ast node */")
                self.write_line(node.class_name + "::" + node.class_name + "(const " + node.class_name + "& obj) ")

                self.write_line("{", post_gutter=1)

                # TODO : more cleanup
                for child in node.children:
                    if child.is_vector:
                        self.write_line("for (auto& item : obj." + child.varname + ") {")
                        self.write_line("    this->" + child.varname + ".push_back(std::shared_ptr< " + child.class_name + ">(item->clone()));")
                        self.write_line("}")
                    else:
                        if child.is_pointer_node():
                            self.write_line("if (obj." + child.varname + ")")
                            self.write_line("    this->" + child.varname + " = std::shared_ptr<" + child.class_name + ">(obj." + child.varname + "->clone());")
                        else:
                            self.write_line("this->" + child.varname + " = obj." + child.varname + ";")

                if node.has_token:
                    self.write_line("if (obj.token)")
                    self.write_line("    this->token = std::shared_ptr<ModToken>(obj.token->clone());")

                self.write_line("}", pre_gutter=-1, newline=2)

        self.write_line("} // namespace ast", pre_gutter=-1)
