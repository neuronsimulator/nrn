class Argument:
    """Utility class for holding all arguments for node classes"""

    def __init__(self):
        self.base_class = ""
        self.class_name = ""
        self.nmodl_name = ""
        self.prefix = ""
        self.suffix = ""
        self.force_prefix = ""
        self.force_suffix = ""
        self.separator = ""
        self.typename = ""
        self.varname = ""
        self.is_vector = False
        self.is_optional = False
        self.add_method = False
        self.getname_method = False
        self.has_token = False
        self.getter_method = False
        self.getter_override = False
