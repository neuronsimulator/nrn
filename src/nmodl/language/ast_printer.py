from printer import *
from utils import *
from node_info import ORDER_VAR_NAME


class AstDeclarationPrinter(DeclarationPrinter):
    """Prints all AST nodes class declarations"""

    def ast_types(self):
        """ print ast type for every ast node """
        self.writer.write_line("enum class Type {", post_gutter=1)
        for node in self.nodes[:-1]:
            name = node_ast_type(node.class_name) + ","
            self.writer.write_line(name)
        name = node_ast_type(self.nodes[-1].class_name)
        self.writer.write_line(name)
        self.writer.write_line("};", pre_gutter=-1)

    def headers(self):
        self.writer.write_line("#include <iostream>")
        self.writer.write_line("#include <memory>")
        self.writer.write_line("#include <string>")
        self.writer.write_line("#include <vector>", newline=2)

        self.writer.write_line('#include "ast/ast_utils.hpp"')
        self.writer.write_line('#include "lexer/modtoken.hpp"')
        self.writer.write_line('#include "utils/common_utils.hpp"')

    def class_comment(self):
        self.writer.write_line("/* all classes representing Abstract Syntax Tree (AST) nodes */")

    def forward_declarations(self):
        self.writer.write_line("/* forward declarations of AST classes */")
        for node in self.nodes:
            self.writer.write_line("class " + node.class_name + ";")

        self.writer.write_line()
        self.writer.write_line("/* Type for every ast node */")
        self.ast_types()

        self.writer.write_line()
        self.writer.write_line("/* std::vector for convenience */")

        for node in self.nodes:
            typename = "std::vector<std::shared_ptr<" + node.class_name + ">>"
            self.writer.write_line("using " + node.class_name + "Vector = " + typename + ";")

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
            self.writer.write_line("using " + tuple[1] + " = " + tuple[0] + ";")


    def ast_classes_declaration(self):

        # TODO: for demo, removing error messages in getToken method.
        # need to look in detail whether we should abort in this case
        # when ModToken in NULL. This was introduced when added SymtabVisitor
        # pass to get Token information.

        # TODO: remove visitor related method definition from declaration
        # to avoid this include
        self.writer.write_line()
        self.writer.write_line("#include <visitors/visitor.hpp>", newline=2)

        self.writer.write_line("/* Define all AST nodes */", newline=2)

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

            self.writer.write_line(class_decl + " {", post_gutter=1)
            self.writer.write_line("public:", post_gutter=1)

            members = []
            members_with_types = []

            for child in node.children:
                members.append(child.get_typename() + " " + child.varname)
                members_with_types.append(child.get_typename_as_member() + " " + child.varname)

            if members:
                self.writer.write_line("/* member variables */")

            # todo : need a way to setup location
            # self.writer.write_line("void setLocation(nmodl::location loc) {}")

            for member in members_with_types:
                self.writer.write_line(member + ";")

            if node.has_token:
                self.writer.write_line("std::shared_ptr<ModToken> token;")


            if node.is_symtab_needed():
                self.writer.write_line("void* symtab = nullptr;", newline=2)

            if members:
                self.writer.write_line("/* constructors */")
                self.writer.write_line(node.class_name + "(" + (", ".join(members)) + ");")
                self.writer.write_line(node.class_name + "(const " + node.class_name + "& obj);")

            # program node holds statements and blocks and we instantiate it in driver
            # also other nodes which we use as value type, parsor needs to return them
            # as a value. And instantiation of those causes error if default constructor not there.
            if node.is_program_node() or node.is_ptr_excluded_node():
                self.writer.write_line( node.class_name + "() {}", newline=2)

            # Todo : We need virtual destructor otherwise there will be memory leaks.
            #        But we need define which are virtual base classes that needs virtual function.
            self.writer.write_line("virtual ~" + node.class_name + "() {}")
            self.writer.write_line()

            get_method_added = False

            for child in node.children:
                class_name = child.class_name
                varname = child.varname
                if child.add_method:
                    self.writer.write_line("void add" + class_name + "(" + class_name + " *s) {")
                    self.writer.write_line("    " + varname + ".push_back(std::shared_ptr<" + class_name + ">(s));")
                    self.writer.write_line("}")

                if child.getname_method:
                    # string node should be evaluated and hence eval() method
                    if child.is_string_node():
                        method = "eval"
                    else:
                        method = "getName"

                    self.writer.write_line("virtual std::string getName() override {")
                    self.writer.write_line("    return " + varname + "->" + method + "();")
                    self.writer.write_line("}")

                    if not node.has_token:
                        self.writer.write_line(virtual + "ModToken* getToken() override {")
                        self.writer.write_line("    return " + varname + "->getToken();")
                        self.writer.write_line("}")

                    get_method_added = True

                if node.is_prime_node() and child.varname == ORDER_VAR_NAME:
                    self.writer.write_line("int getOrder() " + " { return " + ORDER_VAR_NAME + "->eval(); }")

            # TODO: returning typename for name? check the usage of this and fix in better way
            self.writer.write_line("virtual std::string getTypeName() override { return \"" + node.class_name + "\"; }")

            # all member functions
            self.writer.write_line(virtual + "void visitChildren (Visitor* v) override;")
            self.writer.write_line(virtual + "void accept(Visitor* v) override { v->visit" + node.class_name + "(this); }")

            # TODO: type should declared as enum class
            typename = node_ast_type(node.class_name)
            self.writer.write_line(virtual + "Type getType() override { return Type::" + typename + "; }")
            self.writer.write_line(virtual + node.class_name + "* clone() override { return new " + node.class_name + "(*this); }")

            if node.has_token:
                self.writer.write_line(virtual + "ModToken* getToken() " + override + " { return token.get(); }")
                self.writer.write_line("void setToken(ModToken& tok) " + " { token = std::shared_ptr<ModToken>(new ModToken(tok)); }")

            if node.is_symtab_needed():
                self.writer.write_line("void setBlockSymbolTable(void *s) " + " { symtab = s; }")
                self.writer.write_line("void* getBlockSymbolTable() " + " { return symtab; }")

            if node.is_number_node():
                self.writer.write_line(virtual + "void negate()" + override + " { std::cout << \"ERROR : negate() not implemented! \"; abort(); } ")

            if node.is_base_class_number_node():
                if node.is_boolean_node():
                    self.writer.write_line("void negate() override { value = !value; }")
                else:
                    self.writer.write_line("void negate() override { value = -value; }")

            if node.is_identifier_node():
                self.writer.write_line(virtual + "void setName(std::string name)" + override + " { std::cout << \"ERROR : setName() not implemented! \"; abort(); }")

            if node.is_name_node():
                self.writer.write_line(virtual + "void setName(std::string name)" + override + " { value->set(name); }")

            # if node is of enum type then return enum value
            # TODO: hardcoded Names
            if node.is_data_type_node():
                data_type = node.get_data_type_name()
                if node.is_enum_node():
                    self.writer.write_line("std::string " + " eval() { return " + data_type + "Names[value]; }")
                # But if basic data type then eval return their value
                # TODO: value member is hardcoded
                else:
                    self.writer.write_line(data_type + " eval() { return value; }")
                    self.writer.write_line("void set(" + data_type + " _value) " + " { value = _value; }")

            self.writer.write_line("};", pre_gutter=-2, newline=2)

    def class_name_declaration(self):
        self.writer.write_line("namespace ast {", newline=2, post_gutter=1)
        self.forward_declarations()
        self.ast_classes_declaration()

    def declaration_end(self):
        self.writer.write_line(pre_gutter=-1)

    def private_declaration(self):
        pass

    def public_declaration(self):
        pass

    def post_declaration(self):
        self.writer.write_line("} // namespace ast", pre_gutter=-1)


class AstDefinitionPrinter(DefinitionPrinter):
    """Prints AST class definitions"""

    def headers(self):
        self.writer.write_line('#include "ast/ast.hpp"', newline=2)

    def definitions(self):
        self.writer.write_line("namespace ast {", post_gutter=1)

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
                    self.writer.add_line("for(auto& item : this->" + child.varname + ") {")
                    #self.writer.add_line("    iter != this->" + child.varname + "->end(); iter++) {")
                    self.writer.add_line("        item->accept(v);")
                    self.writer.add_line("}")
                elif child.optional or child.is_statement_block_node():
                    self.writer.add_line("if (this->" + child.varname + ") {")
                    self.writer.add_line("    this->" + child.varname + "->accept(v);")
                    self.writer.add_line("}")
                elif not child.is_pointer_node():
                    self.writer.add_line(child.varname + ".accept(v);")
                else:
                    self.writer.add_line(child.varname + "->accept(v);")
                self.writer.decrease_gutter()

            if self.writer.num_buffered_lines():
                self.writer.write_line("/* visit method for " + node.class_name + " ast node */")
                self.writer.write_line("void " + node.class_name + "::visitChildren(Visitor* v) {", post_gutter=1)
                self.writer.flush_buffered_lines()
                self.writer.write_line("}", pre_gutter=-1, newline=2)
            else:
                self.writer.write_line("void " + node.class_name + "::visitChildren(Visitor* v) {}")

            if members:
                # TODO : constructor definition : remove this with C++11 style
                self.writer.write_line("/* constructor for " + node.class_name + " ast node */")
                arguments = (", ".join(map(lambda x: x[0] + " " + x[1], members)))
                self.writer.write_line(node.class_name + "::" + node.class_name + "(" + arguments + ")")

                if non_ptr_members:
                    self.writer.write_line(":", newline=0)
                    self.writer.write_line(",\n".join(map(lambda x: x[1] + "(" + x[1] + ")", non_ptr_members)))

                self.writer.write_line("{")

                for member in ptr_members:
                    # bit hack here : remove pointer because we are creating smart pointer
                    typename = member[0].replace("*", "")
                    self.writer.write_line("    this->" + member[1] + " = std::shared_ptr<" + typename + ">(" + member[1] + ");")

                self.writer.write_line("}", newline=2)

                # copy construcotr definition : remove this with C++11 style
                self.writer.write_line("/* copy constructor for " + node.class_name + " ast node */")
                self.writer.write_line(node.class_name + "::" + node.class_name + "(const " + node.class_name + "& obj) ")

                self.writer.write_line("{", post_gutter=1)

                # TODO : more cleanup
                for child in node.children:
                    if child.is_vector:
                        self.writer.write_line("for(auto& item : obj." + child.varname + ") {")
                        self.writer.write_line("    this->" + child.varname + ".push_back(std::shared_ptr< " + child.class_name + ">(item->clone()));")
                        self.writer.write_line("}")
                    else:
                        if child.is_pointer_node():
                            self.writer.write_line("if(obj." + child.varname + ")")
                            self.writer.write_line("    this->" + child.varname + " = std::shared_ptr<" + child.class_name + ">(obj." + child.varname + "->clone());")
                        else:
                            self.writer.write_line("this->" + child.varname + " = obj." + child.varname + ";")

                if node.has_token:
                    self.writer.write_line("if(obj.token)")
                    self.writer.write_line("    this->token = std::shared_ptr<ModToken>(obj.token->clone());")

                self.writer.write_line("}", pre_gutter=-1, newline=2)

        self.writer.write_line("} // namespace ast", pre_gutter=-1)
