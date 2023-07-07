# Copyright 2023 Blue Brain Project, EPFL.
# See the top-level LICENSE file for details.
#
# SPDX-License-Identifier: Apache-2.0

class Argument:
    """Utility class for holding all arguments for node classes"""

    def __init__(self):
        # BaseNode
        self.class_name = ""
        self.nmodl_name = ""
        self.prefix = ""
        self.suffix = ""
        self.force_prefix = ""
        self.force_suffix = ""
        self.separator = ""
        self.brief = ""
        self.description = ""

        # ChildNode
        self.typename = ""
        self.varname = ""
        self.is_public = False
        self.is_vector = False
        self.is_optional = False
        self.add_method = False
        self.get_node_name = False
        self.getter_method = False
        self.getter_override = False

        # Node
        self.base_class = ""
        self.has_token = False
        self.url = None
