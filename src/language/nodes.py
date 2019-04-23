# ***********************************************************************
# Copyright (C) 2018-2019 Blue Brain Project
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

    def get_add_methods(self):
        s = ''
        if self.add_method:
            method = f"""
                         /**
                          * \\brief Add member to {self.varname} by raw pointer
                          */
                         void add{self.class_name}({self.class_name} *n) {{
                             {self.varname}.emplace_back(n);
                         }}

                         /**
                          * \\brief Add member to {self.varname} by shared_ptr
                          */
                         void add{self.class_name}(std::shared_ptr<{self.class_name}> n) {{
                             {self.varname}.push_back(n);
                         }}
                    """
            s = textwrap.dedent(method)
        return s

    def get_node_name_method(self):
        s = ''
        if self.get_node_name:
            # string node should be evaluated and hence eval() method
            method_name = "eval" if self.is_string_node else "get_node_name"
            method = f"""
                         /**
                          * \\brief Return name of of the node
                          *
                          * Some ast nodes have a member marked designated as node name. For example,
                          * in case of this ast::{self.class_name} has {self.varname} designated as a
                          * node name.
                          *
                          * @return name of the node as std::string
                          *
                          * \\sa Ast::get_node_type_name
                          */
                         virtual std::string get_node_name() override {{
                             return {self.varname}->{method_name}();
                         }}"""
            s = textwrap.dedent(method)
        return s

    def get_getter_method(self, class_name):
        getter_method = self.getter_method if self.getter_method else "get_" + to_snake_case(self.varname)
        getter_override = " override" if self.getter_override else ""
        return_type = self.member_typename
        return f"""
                   /**
                    * \\brief Getter for member variable \\ref {class_name}.{self.varname}
                    */
                   {return_type} {getter_method}(){getter_override}{{
                       return {self.varname};
                   }}"""

    def get_setter_method(self, class_name):
        setter_method = "set_" + to_snake_case(self.varname)
        setter_type = self.member_typename
        reference = "" if self.is_base_type_node else "&&"
        return f"""
                   /**
                    * \\brief Setter for member variable \\ref {class_name}.{self.varname}
                    */
                   void {setter_method}({setter_type}{reference} {self.varname}) {{
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
        args = [f'{c.get_typename()} {c.varname}' for c in self.children]
        return f"{self.class_name}({', '.join(args)});"

    def ctor_definition(self):
        args = [f'{c.get_typename()} {c.varname}' for c in self.children]
        initlist = [f'{c.varname}({c.varname})' for c in self.children]

        s = f"""{self.class_name}::{self.class_name}({', '.join(args)})
                : {', '.join(initlist)} {{}}
        """
        return textwrap.dedent(s)

    def ctor_shrptr_declaration(self):
        args = [f'{c.member_typename} {c.varname}' for c in self.children]
        return f"{self.class_name}({', '.join(args)});"

    def ctor_shrptr_definition(self):
        args = [f'{c.member_typename} {c.varname}' for c in self.children]
        initlist = [f'{c.varname}({c.varname})' for c in self.children]

        s = f"""{self.class_name}::{self.class_name}({', '.join(args)})
                : {', '.join(initlist)} {{}}
        """
        return textwrap.dedent(s)

    def has_ptr_children(self):
        return any(not (c.is_vector or c.is_base_type_node or c.is_ptr_excluded_node)
                   for c in self.children)

    def public_members(self):
        """
        Return public members of the node
        """
        members = [[child.member_typename, child.varname, child.brief]
                   for child in self.children
                   if child.is_public]

        return members

    def private_members(self):
        """
        Return private members of the node
        """
        members = [[child.member_typename, child.varname, child.brief]
                   for child in self.children
                   if not child.is_public]

        if self.has_token:
            members.append(["std::shared_ptr<ModToken>", "token", "token with location information"])

        if self.is_symtab_needed:
            members.append(["symtab::SymbolTable*", "symtab = nullptr", "symbol table for a block"])

        if self.is_program_node:
            members.append(["symtab::ModelSymbolTable", "model_symtab", "global symbol table for model"])

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
