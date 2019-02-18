# ***********************************************************************
# Copyright (C) 2018-2019 Blue Brain Project
#
# This file is part of NMODL distributed under the terms of the GNU
# Lesser General Public License. See top-level LICENSE file for details.
# ***********************************************************************

class Argument:
    """Utility class for holding all arguments for node classes"""

    def __init__(self):
        self.base_class = ""
        self.class_name = ""
        self.description = ""
        self.nmodl_name = ""
        self.prefix = ""
        self.suffix = ""
        self.force_prefix = ""
        self.force_suffix = ""
        self.separator = ""
        self.typename = ""
        self.varname = ""
        self.is_public = False
        self.is_vector = False
        self.is_optional = False
        self.add_method = False
        self.get_node_name = False
        self.has_token = False
        self.getter_method = False
        self.getter_override = False
        self.url = None
