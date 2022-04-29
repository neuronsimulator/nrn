# ***********************************************************************
# Copyright (C) 2018-2022 Blue Brain Project
#
# This file is part of NMODL distributed under the terms of the GNU
# Lesser General Public License. See top-level LICENSE file for details.
# ***********************************************************************

"""Define basic classes used from parser

Node is used to represent a node parsed from every rule
in language definition file. Each member variable of Node
is represented by ChildNode.
"""

import textwrap

import node_info
from utils import to_snake_case


class BaseNode:
    """base class for all node types (parent + child) """

    def __init__(self, args):
        self.class_name = args.class_name
        self.nmodl_name = args.nmodl_name
        self.prefix = args.prefix
        self.suffix = args.suffix
        self.separator = args.separator
        self.force_prefix = args.force_prefix
        self.force_suffix = args.force_suffix
        self.brief = args.brief
        self.description = args.description
        self.is_abstract = False

    def __lt__(self, other):
        return self.class_name < other.class_name

    def get_data_type_name(self):
        """ return type name for the node """
        return node_info.DATA_TYPES[self.class_name]

    @property
    def cpp_header(self):
        """Path to C++ header file of this class relative to BUILD_DIR"""
        return "ast/" + to_snake_case(self.class_name) + ".hpp"

    @property
    def is_statement_block_node(self):
        return self.class_name == node_info.STATEMENT_BLOCK_NODE

    @property
    def is_statement_node(self):
        return self.class_name in node_info.STATEMENT_TYPES

    @property
    def is_global_block_node(self):
        return self.class_name in node_info.GLOBAL_BLOCKS

    @property
    def is_prime_node(self):
        return self.class_name == node_info.PRIME_NAME_NODE

    @property
    def is_program_node(self):
        """
        check if current node is main program container node
        :return: True if main program block otherwise False
        """
        return self.class_name == node_info.PROGRAM_BLOCK

    @property
    def is_block_node(self):
        """
        Check if current node is of type Block

        While translating AST to NMODL, we handle block nodes for printing.

        :return: True or False
        """
        return self.class_name in node_info.BLOCK_TYPES

    @property
    def is_unit_block(self):
        return self.class_name == node_info.UNIT_BLOCK

    @property
    def is_data_type_node(self):
        return self.class_name in node_info.DATA_TYPES

    @property
    def is_symbol_var_node(self):
        return self.class_name in node_info.SYMBOL_VAR_TYPES

    @property
    def is_symbol_helper_node(self):
        return self.class_name in node_info.SYMBOL_TABLE_HELPER_NODES

    @property
    def is_symbol_block_node(self):
        return self.class_name in node_info.SYMBOL_BLOCK_TYPES

    @property
    def is_base_type_node(self):
        return self.class_name in node_info.BASE_TYPES

    @property
    def is_integral_type_node(self):
        return self.class_name in node_info.INTEGRAL_TYPES

    @property
    def is_string_node(self):
        return self.class_name == node_info.STRING_NODE

    @property
    def is_integer_node(self):
        return self.class_name == node_info.INTEGER_NODE

    @property
    def is_float_node(self):
        return self.class_name == node_info.FLOAT_NODE

    @property
    def is_double_node(self):
        return self.class_name == node_info.DOUBLE_NODE

    @property
    def is_number_node(self):
        return self.class_name == node_info.NUMBER_NODE

    @property
    def is_boolean_node(self):
        return self.class_name == node_info.BOOLEAN_NODE

    @property
    def is_name_node(self):
        return self.class_name == node_info.NAME_NODE

    @property
    def is_enum_node(self):
        data_type = node_info.DATA_TYPES[self.class_name]
        return data_type in node_info.ENUM_BASE_TYPES

    @property
    def is_pointer_node(self):
        return not (self.class_name in node_info.PTR_EXCLUDE_TYPES or
                    self.is_base_type_node)

    @property
    def is_ptr_excluded_node(self):
        return self.class_name in node_info.PTR_EXCLUDE_TYPES

    @property
    def requires_default_constructor(self):
        return (self.class_name in node_info.LEXER_DATA_TYPES or
                self.is_program_node or
                self.is_ptr_excluded_node)

    @property
    def has_template_methods(self):
        """returns True if the corresponding C++ class has at least
        one template member method, False otherwise"""
        for child in self.children:
            if child.add_method:
                return True
        return False


class ChildNode(BaseNode):
    """represent member variable for a Node"""

    def __init__(self, args):
        BaseNode.__init__(self, args)
        self.typename = args.typename
        self.varname = args.varname
        self.is_public = args.is_public
        self.is_vector = args.is_vector
        self.optional = args.is_optional
        self.add_method = args.add_method
        self.get_node_name = args.get_node_name
        self.getter_method = args.getter_method
        self.getter_override = args.getter_override

    def get_typename(self):
        """returns type of the node for declaration

        When node is of base type then it is returned as it is. Depending
        on pointer or list, appropriate suffix is added. Some of the examples
        of typename are Expression, ExpressionList, Expression* etc.
        """

        type_name = self.class_name

        if self.is_vector:
            type_name += "Vector"
        elif not self.is_base_type_node and not self.is_ptr_excluded_node:
            type_name += "*"

        return type_name

    def get_rvalue_typename(self):
        """returns rvalue reference type of the node"""
        typename = self.get_typename()
        if self.is_base_type_node and not self.is_integral_type_node or self.is_ptr_excluded_node:
            return "const " + typename + "&"
        return typename

    def get_shared_typename(self):
        """returns the shared pointer type of the node for declaration

        When node is of base type then it is returned as it is. Depending
        on pointer or list, appropriate suffix is added. Some of the examples
        of typename are Expression, ExpressionVector,
        std::shared_ptr<Expression> etc.
        """

        type_name = self.class_name

        if self.is_vector:
            type_name += "Vector"
        elif not self.is_base_type_node and not self.is_ptr_excluded_node:
            type_name = "std::shared_ptr<" + type_name + ">"

        return type_name

    @property
    def member_typename(self):
        """returns type when used as a member of the class"""

        if self.is_vector:
            type_name = self.class_name + "Vector"
        elif self.is_base_type_node or self.is_ptr_excluded_node:
            type_name = self.class_name
        else:
            type_name = f"std::shared_ptr<{self.class_name}>"

        return type_name

    @property
    def member_rvalue_typename(self):
        """returns rvalue reference type when used as returned or parameter type"""
        typename = self.member_typename
        if not self.is_integral_type_node:
            return "const " + typename + "&"
        return typename

    def get_add_methods_declaration(self):
        s = ''
        if self.add_method:
            method = f"""
                         /**
                          * \\brief Add member to {self.varname} by raw pointer
                          */
                         void emplace_back_{to_snake_case(self.class_name)}({self.class_name} *n);

                         /**
                          * \\brief Add member to {self.varname} by shared_ptr
                          */
                         void emplace_back_{to_snake_case(self.class_name)}(std::shared_ptr<{self.class_name}> n);

                         /**
                          * \\brief Erase member to {self.varname}
                          */
                         {self.class_name}Vector::const_iterator erase_{to_snake_case(self.class_name)}({self.class_name}Vector::const_iterator first);

                         /**
                          * \\brief Erase members to {self.varname}
                          */
                         {self.class_name}Vector::const_iterator erase_{to_snake_case(self.class_name)}({self.class_name}Vector::const_iterator first, {self.class_name}Vector::const_iterator last);

                         /**
                          * \\brief Erase non-consecutive members to {self.varname}
                          *
                          * loosely following the cpp reference of remove_if
                          */
                         size_t erase_{to_snake_case(self.class_name)}(std::unordered_set<{self.class_name}*>& to_be_erased);

                         /**
                          * \\brief Insert member to {self.varname}
                          */
                         {self.class_name}Vector::const_iterator insert_{to_snake_case(self.class_name)}({self.class_name}Vector::const_iterator position, const std::shared_ptr<{self.class_name}>& n);

                         /**
                          * \\brief Insert members to {self.varname}
                          */
                         template <class NodeType, class InputIterator>
                         void insert_{to_snake_case(self.class_name)}({self.class_name}Vector::const_iterator position, NodeType& to, InputIterator first, InputIterator last);

                         /**
                          * \\brief Reset member to {self.varname}
                          */
                         void reset_{to_snake_case(self.class_name)}({self.class_name}Vector::const_iterator position, {self.class_name}* n);

                         /**
                          * \\brief Reset member to {self.varname}
                          */
                         void reset_{to_snake_case(self.class_name)}({self.class_name}Vector::const_iterator position, std::shared_ptr<{self.class_name}> n);
                    """
            s = textwrap.dedent(method)
        return s

    def get_add_methods_definition(self, parent):
        s = ''
        if self.add_method:
            set_parent = "n->set_parent(this); "
            if self.optional:
                set_parent = f"""
                        if (n) {{
                            n->set_parent(this);
                        }}
                        """
            method = f"""
                         /**
                          * \\brief Add member to {self.varname} by raw pointer
                          */
                         void {parent.class_name}::emplace_back_{to_snake_case(self.class_name)}({self.class_name} *n) {{
                            {self.varname}.emplace_back(n);

                             // set parents
                             {set_parent}
                         }}

                         /**
                          * \\brief Add member to {self.varname} by shared_ptr
                          */
                         void {parent.class_name}::emplace_back_{to_snake_case(self.class_name)}(std::shared_ptr<{self.class_name}> n) {{
                            {self.varname}.emplace_back(n);
                             // set parents
                             {set_parent}
                         }}

                         /**
                          * \\brief Erase member to {self.varname}
                          */
                         {self.class_name}Vector::const_iterator {parent.class_name}::erase_{to_snake_case(self.class_name)}({self.class_name}Vector::const_iterator first) {{
                            return {self.varname}.erase(first);
                         }}
                         /**
                          * \\brief Erase members to {self.varname}
                          */
                         {self.class_name}Vector::const_iterator {parent.class_name}::erase_{to_snake_case(self.class_name)}({self.class_name}Vector::const_iterator first, {self.class_name}Vector::const_iterator last) {{
                            return {self.varname}.erase(first, last);
                         }}
                         /**
                          * \\brief Erase non-consecutive members to {self.varname}
                          *
                          * loosely following the cpp reference of remove_if
                          */
                         size_t {parent.class_name}::erase_{to_snake_case(self.class_name)}(std::unordered_set<{self.class_name}*>& to_be_erased) {{
                            auto first = {self.varname}.begin();
                            auto last = {self.varname}.end();
                            auto result = first;

                            while (first != last) {{
                                    // automatically erase dangling pointers from the uset while
                                    // looking for them to erase them in the vector
                                    if (to_be_erased.erase(first->get()) == 0) {{
                                        reset_{to_snake_case(self.class_name)}(result, *first);
                                        ++result;
                                    }}
                                    ++first;
                                }}

                            size_t out = last - result;
                            erase_{to_snake_case(self.class_name)}(result, last);

                            return out;
                         }}

                         /**
                          * \\brief Insert member to {self.varname}
                          */
                         {self.class_name}Vector::const_iterator {parent.class_name}::insert_{to_snake_case(self.class_name)}({self.class_name}Vector::const_iterator position, const std::shared_ptr<{self.class_name}>& n) {{
                             {set_parent}
                            return {self.varname}.insert(position, n);
                         }}

                         /**
                          * \\brief Reset member to {self.varname}
                          */
                         void {parent.class_name}::reset_{to_snake_case(self.class_name)}({self.class_name}Vector::const_iterator position, {self.class_name}* n) {{
                             //set parents
                             {set_parent}

                            {self.varname}[position - {self.varname}.begin()].reset(n);
                         }}

                         /**
                          * \\brief Reset member to {self.varname}
                          */
                         void {parent.class_name}::reset_{to_snake_case(self.class_name)}({self.class_name}Vector::const_iterator position, std::shared_ptr<{self.class_name}> n) {{
                             //set parents
                             {set_parent}

                            {self.varname}[position - {self.varname}.begin()] = n;
                         }}
                    """
            s = textwrap.dedent(method)
        return s

    def get_add_methods_inline_definition(self, parent):
        s = ''
        if self.add_method:
            set_parent = "n->set_parent(this); "
            if self.optional:
                set_parent = f"""
                        if (n) {{
                            n->set_parent(this);
                        }}
                        """
            method = f"""
                     /**
                      * \\brief Insert members to {self.varname}
                      */
                     template <class NodeType, class InputIterator>
                     void {parent.class_name}::insert_{to_snake_case(self.class_name)}({self.class_name}Vector::const_iterator position, NodeType& to, InputIterator first, InputIterator last) {{

                         for (auto it = first; it != last; ++it) {{
                             auto& n = *it;
                             //set parents
                             {set_parent}
                          }}
                         {self.varname}.insert(position, first, last);
                     }}
                  """
            s = textwrap.dedent(method)
        return s

    def get_node_name_method_declaration(self):
        s = ''
        if self.get_node_name:
            # string node should be evaluated and hence eval() method
            method = f"""
                 /**
                  * \\brief Return name of the node
                  *
                  * Some ast nodes have a member marked designated as node name. For example,
                  * in case of this ast::{self.class_name} has {self.varname} designated as a
                  * node name.
                  *
                  * @return name of the node as std::string
                  *
                  * \\sa Ast::get_node_type_name
                  */
                 std::string get_node_name() const override;"""
            s = textwrap.dedent(method)
        return s

    def get_node_name_method_definition(self, parent):
        s = ''
        if self.get_node_name:
            # string node should be evaluated and hence eval() method
            method_name = "eval" if self.is_string_node else "get_node_name"
            method = f"""std::string {parent.class_name}::get_node_name() const {{
                             return {self.varname}->{method_name}();
                         }}"""
            s = textwrap.dedent(method)
        return s

    def get_getter_method(self, class_name):
        getter_method = self.getter_method if self.getter_method else "get_" + to_snake_case(self.varname)
        getter_override = " override" if self.getter_override else ""
        return_type = self.member_rvalue_typename
        return f"""
                   /**
                    * \\brief Getter for member variable \\ref {class_name}.{self.varname}
                    */
                   {return_type} {getter_method}() const noexcept {getter_override} {{
                       return {self.varname};
                   }}
                """

    def get_setter_method_declaration(self, class_name):
        setter_method = "set_" + to_snake_case(self.varname)
        setter_type = self.member_typename
        reference = "" if self.is_base_type_node else "&&"
        if self.is_base_type_node:
            return f"""
                       /**
                        * \\brief Setter for member variable \\ref {class_name}.{self.varname}
                        */
                       void {setter_method}({setter_type} {self.varname});
                    """
        else:
            return f"""
                       /**
                        * \\brief Setter for member variable \\ref {class_name}.{self.varname} (rvalue reference)
                        */
                       void {setter_method}({setter_type}&& {self.varname});

                       /**
                        * \\brief Setter for member variable \\ref {class_name}.{self.varname}
                        */
                       void {setter_method}(const {setter_type}& {self.varname});
                    """



    def get_setter_method_definition(self, class_name):
        setter_method = "set_" + to_snake_case(self.varname)
        setter_type = self.member_typename
        reference = "" if self.is_base_type_node else "&&"


        if self.is_base_type_node:
            return f"""
                       void {class_name}::{setter_method}({setter_type} {self.varname}) {{
                           // why don't we use a coding convention instead of this workaround for
                           // variable shadowing?
                           this->{self.varname} = {self.varname};
                       }}
                    """
        elif self.is_vector:
            return f"""
                       void {class_name}::{setter_method}({setter_type}&& {self.varname}) {{
                           this->{self.varname} = {self.varname};
                           // set parents
                           for (auto& ii : {self.varname}) {{
                                   ii->set_parent(this);
                            }}
                       }}

                       void {class_name}::{setter_method}(const {setter_type}& {self.varname}) {{
                           this->{self.varname} = {self.varname};
                           // set parents
                           for (auto& ii : {self.varname}) {{
                                   ii->set_parent(this);
                            }}
                       }}
                    """
        elif self.is_pointer_node or self.optional:
            return f"""
                       void {class_name}::{setter_method}({setter_type}&& {self.varname}) {{
                           this->{self.varname} = {self.varname};
                           // set parents
                           if ({self.varname}) {{
                               {self.varname}->set_parent(this);
                           }}
                       }}

                       void {class_name}::{setter_method}(const {setter_type}& {self.varname}) {{
                           this->{self.varname} = {self.varname};
                           // set parents
                           if ({self.varname}) {{
                               {self.varname}->set_parent(this);
                           }}
                       }}
                    """
        else:
            return f"""
                       void {class_name}::{setter_method}({setter_type}&& {self.varname}) {{
                           this->{self.varname} = {self.varname};
                       }}

                       void {class_name}::{setter_method}(const {setter_type}& {self.varname}) {{
                           this->{self.varname} = {self.varname};
                       }}
                    """




    def __repr__(self):
        return "ChildNode(class_name='{}', nmodl_name='{}')".format(
            self.class_name, self.nmodl_name)

    __str__ = __repr__


class Node(BaseNode):
    """represent a class for every rule in language specification"""

    def __init__(self, args):
        BaseNode.__init__(self, args)
        self.base_class = args.base_class
        self.has_token = args.has_token
        self.url = args.url
        self.children = []

    def cpp_header_deps(self):
        """Get list of C++ headers required by the C++ declaration of this class

        Returns:
            string list of paths to C++ headers relative to BUILD_DIR
        """
        dependent_classes = set()
        if self.base_class:
            dependent_classes.add(self.base_class)
        for child in self.children:
            if child.is_ptr_excluded_node or child.is_vector:
                dependent_classes.add(child.class_name)
        return sorted(["ast/{}.hpp".format(to_snake_case(clazz)) for clazz in dependent_classes])

    @property
    def ast_enum_name(self):
        return to_snake_case(self.class_name).upper()

    @property
    def negation(self):
        return "!" if self.is_boolean_node else "-"

    def add_child(self, args):
        """add new child i.e. member to the class"""
        node = ChildNode(args)
        self.children.append(node)

    def has_children(self):
        return bool(self.children)

    @property
    def is_block_scoped_node(self):
        """
        Check if node is derived from BASE_BLOCK
        :return: True / False
        """
        return self.base_class == node_info.BASE_BLOCK

    def has_parent_block_node(self):
        """
        check is base or parent is structured base block
        :return: True if parent is BASE_BLOCK otherwise False
        """
        return self.base_class == node_info.BASE_BLOCK

    @property
    def has_setters(self):
        """returns True if the class has at least one setter member method"""
        return any([
            self.is_name_node,
            self.has_token,
            self.is_symtab_needed,
            self.is_data_type_node and not self.is_enum_node
        ])

    @property
    def is_base_block_node(self):
        """
        check if node is Block
        :return: True if node type/name is BASE_BLOCK
        """
        return self.class_name == node_info.BASE_BLOCK

    @property
    def is_symtab_needed(self):
        """
        Check if symbol tabel needed for current node

        Only main program node and block scoped nodes needed Symbol table
        :return: True or False
        """
        # block scope nodes have symtab pointer
        return self.is_program_node or self.is_block_scoped_node

    @property
    def is_symtab_method_required(self):
        """
        Symbols are not present in all block nodes e.g. Integer node
        and may other nodes has standard "node->visitChildren(this);"

        As we are inheriting from AstVisitor, we don't have write empty
        methods. Only override those where symbols are contained.

        :return: True if need to print visit method for node in symtabjsonvisitor
                 otherwise False
        """
        return (self.has_children() and
                (self.is_symbol_var_node or
                 self.is_symbol_block_node or
                 self.is_symbol_helper_node or
                 self.is_program_node or
                 self.has_parent_block_node()
                 ))

    @property
    def is_base_class_number_node(self):
        """
        Check if node is of type Number
        """
        return self.base_class == node_info.NUMBER_NODE

    def ctor_declaration(self):
        args = [f'{c.get_rvalue_typename()} {c.varname}' for c in self.children]
        return f"explicit {self.class_name}({', '.join(args)});"

    def ctor_definition(self):
        args = [f'{c.get_rvalue_typename()} {c.varname}' for c in self.children]
        initlist = [f'{c.varname}({c.varname})' for c in self.children]

        s = f"""{self.class_name}::{self.class_name}({', '.join(args)})
                : {', '.join(initlist)} {{ set_parent_in_children(); }}
        """
        return textwrap.dedent(s)

    def ctor_shrptr_declaration(self):
        args = [f'{c.member_rvalue_typename} {c.varname}' for c in self.children]
        return f"explicit {self.class_name}({', '.join(args)});"

    def ctor_shrptr_definition(self):
        args = [f'{c.member_rvalue_typename} {c.varname}' for c in self.children]
        initlist = [f'{c.varname}({c.varname})' for c in self.children]

        s = f"""{self.class_name}::{self.class_name}({', '.join(args)})
                : {', '.join(initlist)} {{ set_parent_in_children(); }}
        """
        return textwrap.dedent(s)

    def has_ptr_children(self):
        return any(not (c.is_vector or c.is_base_type_node or c.is_ptr_excluded_node)
                   for c in self.children)

    def public_members(self):
        """
        Return public members of the node
        """
        members = [[child.member_typename, child.varname, None, child.brief]
                   for child in self.children
                   if child.is_public]

        return members

    def private_members(self):
        """
        Return private members of the node
        """
        members = [[child.member_typename, child.varname, None, child.brief]
                   for child in self.children
                   if not child.is_public]

        if self.has_token:
            members.append(["std::shared_ptr<ModToken>", "token", None, "token with location information"])

        if self.is_symtab_needed:
            members.append(["symtab::SymbolTable*", "symtab",  "nullptr", "symbol table for a block"])

        if self.is_program_node:
            members.append(["symtab::ModelSymbolTable", "model_symtab", None, "global symbol table for model"])

        return members

    def properties(self):
        """
        Return private members of the node destined to be pybind properties
        """
        members = [[child.member_typename, child.varname, child.is_base_type_node, None, child.brief]
                   for child in self.children
                   if not child.is_public]

        if self.has_token:
            members.append(["std::shared_ptr<ModToken>", "token", True, None, "token with location information"])

        return members

    @property
    def non_base_members(self):
        return [child for child in self.children if not child.is_base_type_node]

    def get_description(self):
        """
        Return description for the node in doxygen form
        """
        lines = self.description.split('\n')
        description = ""
        for i, line in enumerate(lines):
            if i == 0:
                description = ' ' + line + '\n'
            else:
                description += '  * ' + line + '\n'
        return description

    def __repr__(self):
        return "Node(class_name='{}', base_class='{}', nmodl_name='{}')".format(
            self.class_name, self.base_class, self.nmodl_name)

    def __eq__(self, other):
        """
        AST node name (i.e. class_name) is supposed to be unique, just compare it for equality
        """
        return self.class_name == other.class_name

    __str__ = __repr__
