"""Define basic classes used from parser

Node is used to represent a node parsed from every rule
in language definition file. Each member variable of Node
is represented by ChildNode.
"""

from argument import Argument
from node_info import *


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
        self.is_abstract = False

    def __cmp__(self, other):
        return cmp(self.class_name, other.class_name)

    def get_data_type_name(self):
        """ return type name for the node """
        return DATA_TYPES[self.class_name]

    def get_typename(self):
        """returns type of the node for declaration

        When node is of base type then it is returned as it is. Depending
        on pointer or list, appropriate suffix is added. Some of the examples
        of typename are Expression, ExpressionList, Expression* etc.

        This is different than child node because childs are typically
        class members and need to be
        """

        type_name = self.class_name

        if self.class_name not in BASE_TYPES and self.class_name not in PTR_EXCLUDE_TYPES:
            type_name += "*"

        return type_name

    def is_expression_node(self):
        return True if self.class_name == EXPRESSION_NODE else False

    def is_statement_block_node(self):
        return True if self.class_name == STATEMENT_BLOCK_NODE else False

    def is_statement_node(self):
        return True if self.class_name in STATEMENT_TYPES else False

    def is_global_block_node(self):
        return True if self.class_name in GLOBAL_BLOCKS else False

    def is_prime_node(self):
        return True if self.class_name == PRIME_NAME_NODE else False

    def is_program_node(self):
        """
        check if current node is main program container node
        :return: True if main program block otherwise False
        """
        return True if self.class_name == PROGRAM_BLOCK else False

    def is_block_node(self):
        """
        Check if current node is of type Block

        While translating AST to NMODL, we handle block nodes for printing.

        :return: True or False
        """
        return True if self.class_name in BLOCK_TYPES else False

    def is_reaction_statement_node(self):
        return True if self.class_name == REACTION_STATEMENT_NODE else False

    def is_conserve_node(self):
        return True if self.class_name == CONSERVE_NODE else False

    def is_binary_expression_node(self):
        return True if self.class_name == BINARY_EXPRESSION_NODE else False

    def is_unary_expression_node(self):
        return True if self.class_name == UNARY_EXPRESSION_NODE else False

    def is_verbatim_node(self):
        return True if self.class_name == VERBATIM_NODE else False

    def is_comment_node(self):
        return True if self.class_name == COMMENT_NODE else False

    def is_independentdef_node(self):
        return True if self.class_name == INDEPENDENTDEF_NODE else False

    def is_unit_block(self):
        return True if self.class_name == UNIT_BLOCK else False

    def is_data_type_node(self):
        return True if self.class_name in DATA_TYPES.keys() else False

    def is_symbol_var_node(self):
        return True if self.class_name in SYMBOL_VAR_TYPES else False

    def is_symbol_helper_node(self):
        return True if self.class_name in SYMBOL_TABLE_HELPER_NODES else False

    def is_symbol_block_node(self):
        return True if self.class_name in SYMBOL_BLOCK_TYPES else False

    def is_base_type_node(self):
        return True if self.class_name in BASE_TYPES else False

    def is_string_node(self):
        return True if self.class_name == STRING_NODE else False

    def is_integer_node(self):
        return True if self.class_name == INTEGER_NODE else False

    def is_number_node(self):
        return True if self.class_name == NUMBER_NODE else False

    def is_boolean_node(self):
        return True if self.class_name == BOOLEAN_NODE else False

    def is_identifier_node(self):
        return True if self.class_name == IDENTIFIER_NODE else False

    def is_name_node(self):
        return True if self.class_name == NAME_NODE else False

    def is_enum_node(self):
        data_type = DATA_TYPES[self.class_name]
        return True if data_type in ENUM_BASE_TYPES else False

    def is_pointer_node(self):
        if self.class_name in PTR_EXCLUDE_TYPES:
            return False
        if self.is_base_type_node():
            return False
        return True

    def is_ptr_excluded_node(self):
        if self.class_name in PTR_EXCLUDE_TYPES:
            return True
        return False


class ChildNode(BaseNode):
    """represent member variable for a Node"""

    def __init__(self, args):
        BaseNode.__init__(self, args)
        self.typename = args.typename
        self.varname = args.varname
        self.is_vector = args.is_vector
        self.optional = args.is_optional
        self.add_method = args.add_method
        self.getname_method = args.getname_method
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
        elif not self.is_base_type_node() and not self.is_ptr_excluded_node():
            type_name += "*"

        return type_name

    @property
    def member_typename(self):
        """returns type when used as a member of the class

        todo: Check how to refactor this
        """

        type_name = self.class_name

        if self.is_vector:
            type_name += "Vector"
        elif not self.is_base_type_node() and not self.is_ptr_excluded_node():
            type_name = "std::shared_ptr<" + type_name + ">"

        return type_name

    @property
    def return_typename(self):
        """returns type when returned from getter method

        todo: Check how to refactor this
        """
        type_name = "std::shared_ptr<" + self.class_name + ">"

        if self.is_vector:
            type_name = self.class_name + "Vector"
        elif self.is_ptr_excluded_node() or self.is_base_type_node():
            type_name =  self.class_name

        return type_name


class Node(BaseNode):
    """represent a class for every rule in language specification"""

    def __init__(self, args):
        BaseNode.__init__(self, args)
        self.base_class = args.base_class
        self.has_token = args.has_token
        self.children = []

    def add_child(self, args):
        """add new child i.e. member to the class"""
        node = ChildNode(args)
        self.children.append(node)

    def has_children(self):
        return True if self.children else False

    def is_block_scoped_node(self):
        """
        Check if node is derived from BASE_BLOCK
        :return: True / False
        """
        return True if self.base_class == BASE_BLOCK else False

    def has_parent_block_node(self):
        """
        check is base or parent is structured base block
        :return: True if parent is BASE_BLOCK otherwise False
        """
        return True if self.base_class == BASE_BLOCK else False

    def is_base_block_node(self):
        """
        check if node is Block
        :return: True if node type/name is BASE_BLOCK
        """
        return True if self.class_name == BASE_BLOCK else False

    def is_symtab_needed(self):
        """
        Check if symbol tabel needed for current node

        Only main program node and block scoped nodes needed Symbol table
        :return: True or False
        """
        # block scope nodes have symtab pointer
        if self.is_program_node() or self.is_block_scoped_node():
            return True
        return False

    def is_symtab_method_required(self):
        """
        Symbols are not present in all block nodes e.g. Integer node
        and may other nodes has standard "node->visitChildren(this);"

        As we are inheriting from AstVisitor, we don't have write empty
        methods. Only override those where symbols are contained.

        :return: True if need to print visit method for node in symtabjsonvisitor
                 otherwise False
        """
        method_required = False

        if self.has_children():

            if self.class_name in SYMBOL_VAR_TYPES or self.class_name in SYMBOL_BLOCK_TYPES:
                method_required = True

            if self.is_program_node() or self.has_parent_block_node():
                method_required = True

            if self.class_name in SYMBOL_TABLE_HELPER_NODES:
                method_required = True

        return method_required

    def is_base_class_number_node(self):
        return True if self.base_class == NUMBER_NODE else False
