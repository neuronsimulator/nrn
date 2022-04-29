# ***********************************************************************
# Copyright (C) 2018-2022 Blue Brain Project
#
# This file is part of NMODL distributed under the terms of the GNU
# Lesser General Public License. See top-level LICENSE file for details.
# ***********************************************************************

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
