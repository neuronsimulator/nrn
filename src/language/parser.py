"""Parser for parsing abstract NMODL language definition file

Abstract definition file of NMODL has custom syntax for defining language
constructs. This module provides basic YAML parsing utility and creates
AST tree nodes.

"""

import sys
import yaml
from argument import Argument
from nodes import Node
from node_info import *


class LanguageParser:
    """class to parse abstract language definition YAML file"""

    def __init__(self, filename, debug=False):
        self.filename = filename
        self.debug = debug

    @classmethod
    def is_token(self, name):
        """check if the name (i.e. class) is a token type in lexer

        Lexims returned from Lexer have position and hence token object.
        Return True if this node is returned by lexer otherwise False
        """
        if name in LEXER_DATA_TYPES or name in SYMBOL_BLOCK_TYPES or name in ADDITIONAL_TOKEN_BLOCKS:
            return True
        else:
            return False

    def parse_child_rule(self, child):
        """parse child specification and return argument as properties

        Child specification has additional option like list, optional,
        getName method etc.
        """

        # there is only one key and it has one value
        varname = next(iter(list(child.keys())))
        properties = next(iter(list(child.values())))

        # arguments holder for creating tree node
        args = Argument()

        # node of the variable
        args.varname = varname

        # type i.e. class of the variable
        args.class_name = properties['type']


        if self.debug:
            print(('Child {}, {}'.format(args.varname, args.class_name)))

        # if there is add method for member in the class
        if 'add' in properties:
            args.add_method = properties['add']

        # if variable is an optional member
        if 'optional' in properties:
            args.is_optional = properties['optional']

        # if variable is vector, the separator to use while
        # printing back to nmodl
        if 'separator' in properties:
            args.separator = properties['separator']

        # if variable if of vector type
        if 'vector' in properties:
            args.is_vector = properties['vector']

        # if getNmae method required
        if 'getname' in properties:
            args.getname_method = properties['getname']

        # if getter method required
        if 'getter' in properties:
            if 'name' in properties['getter']:
                args.getter_method = properties['getter']['name']
            if 'override' in properties['getter']:
                args.getter_override = properties['getter']['override']

        # if there is nmodl name
        if 'nmodl' in properties:
            args.nmodl_name = properties['nmodl']

        # prefix while printing back to NMODL
        if 'prefix' in properties:
            args.prefix = properties['prefix']['value']

            # if prefix is compulsory to print in NMODL then make suffix empty
            if 'force' in properties['prefix']:
                if properties['prefix']['force']:
                    args.force_prefix = args.prefix
                    args.prefix = ""

        # suffix while printing back to NMODL
        if 'suffix' in properties:
            args.suffix = properties['suffix']['value']

            # if suffix is compulsory to print in NMODL then make suffix empty
            if 'force' in properties['suffix']:
                if properties['suffix']['force']:
                    args.force_suffix = args.suffix
                    args.suffix = ""

        return args


    def parse_yaml_rules(self, nodelist, base_class=None):
        abstract_nodes = []
        nodes = []

        for node in nodelist:
            # name of the ast class and it's properties as dictionary
            class_name = next(iter(list(node.keys())))
            properties = next(iter(list(node.values())))

            # yaml file has abstract classes and their subclasses (i.e. children) as a list
            if isinstance(properties, list):
                # recursively parse all sub-classes of current abstract class
                child_abstract_nodes, child_nodes = self.parse_yaml_rules(properties, class_name)

                # append all parsed subclasses
                abstract_nodes.extend(child_abstract_nodes)
                nodes.extend(child_nodes)

                # classes like AST which don't have base class
                # are not added (we print AST class separately)
                if base_class:
                    args = Argument()
                    args.base_class = base_class
                    args.class_name = class_name
                    node = Node(args)
                    abstract_nodes.append(node)
                    nodes.insert(0, node)
                    if self.debug:
                        print(('Abstract {}, {}'.format(base_class, class_name)))
            else:
                # name of the node while printing back to NMODL
                nmodl_name = properties['nmodl'] if 'nmodl' in properties else None

                # check if we need token for the node
                # todo : we will have token for every node
                has_token = LanguageParser.is_token(class_name)

                args = Argument()
                args.base_class = base_class if base_class else 'AST'
                args.class_name = class_name
                args.nmodl_name = nmodl_name
                args.has_token = has_token

                # create tree node and add to the list
                node = Node(args)
                nodes.append(node)

                if self.debug:
                    print(('Class {}, {}, {}'.format(base_class, class_name, nmodl_name)))

                # now process all children specification
                childs = properties['members'] if 'members' in properties else []
                for child in childs:
                    args = self.parse_child_rule(child)
                    node.add_child(args)

        # update the abstract nodes
        for absnode in abstract_nodes:
            for node in nodes:
                if absnode.class_name == node.class_name:
                    node.is_abstract = True
                    break

        return abstract_nodes, nodes

    def parse_file(self):
        """ parse nmodl YAML specification file for AST creation """

        with open(self.filename, 'r') as stream:
            try:
                rules = yaml.load(stream)
                # abstract nodes are not used though
                abstract_nodes, nodes = self.parse_yaml_rules(rules)
            except yaml.YAMLError as e:
                print(("Error while parsing YAML definition file {0} : {1}".format(self.filename, e.strerror)))
                sys.exit(1)

        return nodes
