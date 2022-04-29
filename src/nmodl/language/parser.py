# ***********************************************************************
# Copyright (C) 2018-2022 Blue Brain Project
#
# This file is part of NMODL distributed under the terms of the GNU
# Lesser General Public License. See top-level LICENSE file for details.
# ***********************************************************************

"""Parser for parsing abstract NMODL language definition file

Abstract definition file of NMODL has custom syntax for defining language
constructs. This module provides basic YAML parsing utility and creates
AST tree nodes.

"""

import sys
import yaml

from argument import Argument
from nodes import Node
import node_info


class LanguageParser:
    """class to parse abstract language definition YAML file"""

    def __init__(self, filename, debug=False):
        self.filename = filename
        self.debug = debug


    def parse_child_rule(self, child):
        """parse child specification and return argument as properties

        Child specification has additional option like list, optional,
        getName method etc.
        """
        # there is only one key and it has one value
        varname, properties = next(iter(child.items()))

        args = Argument()
        args.varname = varname

        # type i.e. class of the variable
        args.class_name = properties['type']

        if self.debug:
            print('Child {}, {}'.format(args.varname, args.class_name))

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

        # if variable is public member
        if 'public' in properties:
            args.is_public = properties['public']

        # if variable if of vector type
        if 'vector' in properties:
            args.is_vector = properties['vector']

        # if get_node_name method required
        if 'node_name' in properties:
            args.get_node_name = properties['node_name']

        # brief description of member variable
        if 'brief' in properties:
            args.brief = properties['brief']

        # description of member variable
        if 'description' in properties:
            args.description = properties['description']

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
            class_name, properties = next(iter(node.items()))

            # no need to process empty nodes
            if properties is None:
                continue

            args = Argument()
            args.url = properties.get('url', None)
            args.class_name = class_name
            args.brief = properties.get('brief', '')
            args.description = properties.get('description', '')

            # yaml file has abstract classes and their subclasses with children as a property
            if 'children' in properties:
                # recursively parse all sub-classes of current abstract class
                child_abstract_nodes, child_nodes = self.parse_yaml_rules(properties['children'], class_name)

                # append all parsed subclasses
                abstract_nodes.extend(child_abstract_nodes)
                nodes.extend(child_nodes)

                # classes like Ast which don't have base class
                # are not added (we print Ast class separately)
                if base_class is not None:
                    args.base_class = base_class
                    node = Node(args)
                    abstract_nodes.append(node)
                    nodes.insert(0, node)
                    if self.debug:
                        print('Abstract {}'.format(node))
            else:
                args.base_class = base_class if base_class else 'Ast'

                # store token in every node
                args.has_token = True

                # name of the node while printing back to NMODL
                args.nmodl_name = properties['nmodl'] if 'nmodl' in properties else None

                # create tree node and add to the list
                node = Node(args)
                nodes.append(node)

                if self.debug:
                    print('Class {}'.format(node))

                # now process all children specification
                if 'members' in properties:
                    for child in properties['members']:
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
                rules = yaml.safe_load(stream)
                _, nodes = self.parse_yaml_rules(rules)
            except yaml.YAMLError as e:
                print("Error while parsing YAML definition file {0} : {1}".format(
                    self.filename, e.strerror))
                sys.exit(1)

        return nodes
